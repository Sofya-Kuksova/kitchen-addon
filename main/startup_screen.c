#include "startup_screen.h"
#include "display_port.h"
#include "lvgl.h"

LV_FONT_DECLARE(kitchen_text_32)
LV_FONT_DECLARE(kitchen_text_24)

static struct {
    lv_obj_t *root, *status, *fill;
    uint32_t shown_at;
} s;

static void fade(void *object, int32_t opacity)
{
    lv_obj_set_style_opa(object, opacity, 0);
}
static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) != s.root) return;
    lv_anim_del(s.root, fade);
    s.root = s.status = s.fill = NULL;
}
static void fade_finished(lv_anim_t *animation)
{
    if (animation->var == s.root) lv_obj_del(s.root);
}
static lv_obj_t *plain(lv_obj_t *parent, int width, int height, uint32_t color)
{
    lv_obj_t *object = lv_obj_create(parent);
    if (!object) return NULL;
    lv_obj_remove_style_all(object);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return object;
}
esp_err_t startup_screen_show(void)
{
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    if (s.root) { display_port_unlock(); return ESP_ERR_INVALID_STATE; }
    lv_disp_t *display = display_port_get_display();
    if (!display) { display_port_unlock(); return ESP_ERR_INVALID_STATE; }
    s.root = plain(lv_disp_get_layer_top(display), 1024, 600, 0x000000);
    if (!s.root) goto no_mem;
    lv_obj_add_flag(s.root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s.root, deleted, LV_EVENT_DELETE, NULL);
    lv_obj_t *title = lv_label_create(s.root);
    s.status = lv_label_create(s.root);
    lv_obj_t *track = plain(s.root, 420, 18, 0x2c2c2c);
    if (!title || !s.status || !track) goto no_mem;
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &kitchen_text_32, 0);
    lv_label_set_text(title, "Loading...");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -70);
    lv_obj_set_style_text_color(s.status, lv_color_hex(0xbfc0c5), 0);
    lv_obj_set_style_text_font(s.status, &kitchen_text_24, 0);
    lv_label_set_text(s.status, "Starting kitchen timers");
    lv_obj_align(s.status, LV_ALIGN_CENTER, 0, 18);
    lv_obj_set_style_radius(track, 8, 0);
    lv_obj_set_style_border_width(track, 1, 0);
    lv_obj_set_style_border_color(track, lv_color_hex(0xe8e8e8), 0);
    lv_obj_align(track, LV_ALIGN_CENTER, 0, 56);
    s.fill = plain(track, 16, 14, 0xffffff);
    if (!s.fill) goto no_mem;
    lv_obj_set_pos(s.fill, 1, 1);
    lv_obj_set_style_radius(s.fill, 6, 0);
    s.shown_at = lv_tick_get();
    lv_refr_now(display);
    display_port_unlock();
    return ESP_OK;
no_mem:
    if (s.root) lv_obj_del(s.root);
    display_port_unlock();
    return ESP_ERR_NO_MEM;
}
esp_err_t startup_screen_progress(uint8_t percent, const char *status)
{
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    if (!s.root) { display_port_unlock(); return ESP_ERR_INVALID_STATE; }
    if (percent > 100) percent = 100;
    int width = 416 * percent / 100;
    lv_obj_set_width(s.fill, width > 1 ? width : 1);
    if (status) lv_label_set_text(s.status, status);
    lv_obj_align(s.status, LV_ALIGN_CENTER, 0, 18);
    lv_refr_now(display_port_get_display());
    display_port_unlock();
    return ESP_OK;
}
esp_err_t startup_screen_finish(void)
{
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    if (!s.root) { display_port_unlock(); return ESP_ERR_INVALID_STATE; }
    uint32_t elapsed = lv_tick_elaps(s.shown_at);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, s.root);
    lv_anim_set_exec_cb(&animation, fade);
    lv_anim_set_values(&animation, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_delay(&animation, elapsed < 900 ? 900 - elapsed : 0);
    lv_anim_set_time(&animation, 260);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_set_ready_cb(&animation, fade_finished);
    lv_anim_start(&animation);
    display_port_unlock();
    return ESP_OK;
}
esp_err_t startup_screen_destroy(void)
{
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    if (s.root) lv_obj_del(s.root);
    display_port_unlock();
    return ESP_OK;
}
