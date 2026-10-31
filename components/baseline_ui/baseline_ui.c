#include "baseline_ui.h"
#include <string.h>
#include "display_port.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "baseline_ui";
static struct {
    lv_obj_t *root, *slider, *brightness, *touch, *memory;
    lv_timer_t *touch_timer, *memory_timer, *restore_timer;
    bool restore_pending;
} s;

static void brightness_text(unsigned value)
{
    lv_label_set_text_fmt(s.brightness, "Brightness: %u%%  |  0%% restores to 70%% after 5 s", value);
}

static void restore_brightness(lv_timer_t *timer)
{
    (void)timer;
    lv_timer_pause(s.restore_timer);
    s.restore_pending = false;
    esp_err_t ret = display_port_set_brightness(70);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "brightness restore: %s", esp_err_to_name(ret));
        lv_label_set_text(s.brightness, "Brightness restore failed; see log");
        return;
    }
    lv_slider_set_value(s.slider, 70, LV_ANIM_OFF);
    brightness_text(70);
}

static void brightness_changed(lv_event_t *event)
{
    (void)event;
    const unsigned value = (unsigned)lv_slider_get_value(s.slider);
    esp_err_t ret = display_port_set_brightness(value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "brightness: %s", esp_err_to_name(ret));
        lv_label_set_text(s.brightness, "Brightness update failed; see log");
        return;
    }
    brightness_text(value);
    if (value == 0 && !s.restore_pending) {
        /* Continued finger motion at zero must not postpone recovery. */
        s.restore_pending = true;
        lv_timer_reset(s.restore_timer);
        lv_timer_resume(s.restore_timer);
    } else if (value != 0) {
        s.restore_pending = false;
        lv_timer_pause(s.restore_timer);
    }
}

static void sample_touch(lv_timer_t *timer)
{
    (void)timer;
    lv_indev_t *touch = display_port_get_touch();
    lv_point_t point;
    lv_indev_get_point(touch, &point);
    lv_label_set_text_fmt(s.touch, "Last touch point: x=%ld y=%ld", (long)point.x, (long)point.y);
}

static void sample_memory(lv_timer_t *timer)
{
    (void)timer;
    const display_port_diagnostics_t d = display_port_sample_gui_diagnostics();
    lv_label_set_text_fmt(s.memory, "Memory free / largest (bytes)\nInternal: %u / %u   SPIRAM: %u / %u\nGUI stack minimum free: %u bytes",
        (unsigned)d.internal_free, (unsigned)d.internal_largest,
        (unsigned)d.spiram_free, (unsigned)d.spiram_largest, d.gui_stack_hwm_bytes);
}


static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y)
{
    lv_obj_t *obj = lv_label_create(parent);
    if (obj) { lv_label_set_text(obj, text); lv_obj_set_pos(obj, x, y); }
    return obj;
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    if (obj) {
        lv_obj_remove_style_all(obj);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_size(obj, w, h);
        lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
    return obj;
}

static void destroy_locked(void)
{
    if (s.touch_timer) lv_timer_del(s.touch_timer);
    if (s.memory_timer) lv_timer_del(s.memory_timer);
    if (s.restore_timer) lv_timer_del(s.restore_timer);
    if (s.root) lv_obj_del(s.root);
    memset(&s, 0, sizeof(s));
}

esp_err_t baseline_ui_destroy(void)
{
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    destroy_locked();
    display_port_unlock();
    return ESP_OK;
}

esp_err_t baseline_ui_create(const char *identity)
{
    if (!identity || s.root) return ESP_ERR_INVALID_ARG;
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    lv_disp_t *display = display_port_get_display();
    if (!display) { display_port_unlock(); return ESP_ERR_INVALID_STATE; }
    /* All dimensions are scene geometry; no board-specific branches. */
    s.root = box(lv_disp_get_scr_act(display), 0, 0, 1024, 600, 0x152033);
    if (!s.root) goto no_mem;
    lv_obj_set_style_text_color(s.root, lv_color_hex(0xffffff), 0);
    lv_obj_t *title = label(s.root, identity, 12, 402);
    if (!title) goto no_mem;
    lv_obj_set_width(title, 1000);
    s.touch = label(s.root, "Touch: waiting", 12, 425);
    s.brightness = label(s.root, "Brightness: 70%", 12, 447);
    s.memory = label(s.root, "Memory: first sample after 10 s", 12, 497);
    s.slider = lv_slider_create(s.root);
    if (!s.touch || !s.brightness || !s.memory || !s.slider) goto no_mem;
    lv_obj_set_pos(s.slider, 25, 478);
    lv_obj_set_size(s.slider, 950, 12);
    lv_slider_set_range(s.slider, 0, 100);
    lv_slider_set_value(s.slider, 70, LV_ANIM_OFF);
    brightness_text(70);
    s.touch_timer = lv_timer_create(sample_touch, 100, NULL);
    s.memory_timer = lv_timer_create(sample_memory, 10000, NULL);
    s.restore_timer = lv_timer_create(restore_brightness, 5000, NULL);
    if (!s.touch_timer || !s.memory_timer || !s.restore_timer) goto no_mem;
    lv_timer_pause(s.restore_timer);
    lv_obj_add_event_cb(s.slider, brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);
    display_port_unlock();
    return ESP_OK;
no_mem:
    destroy_locked();
    display_port_unlock();
    return ESP_ERR_NO_MEM;
}

lv_obj_t *baseline_ui_get_root(void) { return s.root; }
