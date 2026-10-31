#include "display_port.h"
#include <stdlib.h>
#include "crowpanel_board.h"
#include "crowpanel_bsp.h"
#include "esp_lvgl_port.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if LVGL_VERSION_MAJOR != 8 || LVGL_VERSION_MINOR != 4
#error "Standalone requires LVGL8.4"
#endif
static const char *TAG = "display_port";
static lv_disp_t *s_display;
static lv_indev_t *s_touch;
static bool s_port, s_hw;
void display_port_log_memory(const char *stage)
{
    ESP_LOGI(TAG, "MEM %s internal free=%u largest=%u SPIRAM free=%u largest=%u",
        stage,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}


esp_err_t display_port_deinit(void)
{
    if (s_hw) (void)crowpanel_bsp_set_brightness(0);
    if (s_port) {
        /* LVGL8 port deinit only signals the worker; no public join exists.
         * Never free handles still referenced by its task/DSI ISR. */
        ESP_LOGW(TAG, "port/hardware retained until reset; no runtime hardware restart");
        return ESP_ERR_NOT_SUPPORTED;
    }
    esp_err_t ret = s_hw ? crowpanel_bsp_deinit() : ESP_OK;
    if (ret == ESP_OK) s_hw = false;
    return ret;
}
esp_err_t display_port_init(void)
{
    if (s_port || s_hw) return ESP_ERR_INVALID_STATE;
    crowpanel_bsp_handles_t hw;
    esp_err_t ret = crowpanel_bsp_init(2, &hw);
    if (ret != ESP_OK) return ret;
    s_hw = true;
    lvgl_port_cfg_t cfg = ESP_LVGL_PORT_INIT_CONFIG();
    cfg.task_stack = 8192; cfg.task_priority = 6; cfg.task_affinity = -1;
    cfg.task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    cfg.task_max_sleep_ms = 20;
    ret = lvgl_port_init(&cfg);
    if (ret != ESP_OK) { (void)display_port_deinit(); return ret; }
    s_port = true;
    ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    const lvgl_port_display_cfg_t display = {
        .io_handle = hw.panel_io, .panel_handle = hw.panel,
        .buffer_size = CROWPANEL_LCD_WIDTH * CROWPANEL_LCD_HEIGHT,
        .double_buffer = true, .hres = CROWPANEL_LCD_WIDTH, .vres = CROWPANEL_LCD_HEIGHT,
        .flags = {.buff_spiram = true, .buff_dma = false, .sw_rotate = false,
                  .direct_mode = true, .full_refresh = false},
    };
    /* With avoid_tearing the port borrows both existing LCD framebuffers. */
    const lvgl_port_display_dsi_cfg_t dsi = {.flags.avoid_tearing = true};
    s_display = lvgl_port_add_disp_dsi(&display, &dsi);
    if (!s_display) {
        /* Port 2.8.0~1 can free borrowed framebuffers on registration failure.
         * Do not continue with a panel that may reference released memory. */
        ESP_LOGE(TAG, "direct display registration failed; framebuffer ownership uncertain, aborting");
        abort();
    }
    if (s_display) {
        const lvgl_port_touch_cfg_t touch = {
            .disp = s_display, .handle = hw.touch, .scale = {.x = 1, .y = 1},
        };
        s_touch = lvgl_port_add_touch(&touch);
    }
    display_port_unlock();
    if (!s_display || !s_touch) {
        ESP_LOGE(TAG, "display/touch registration failed; retaining powered-off port resources");
        (void)display_port_deinit();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "IDF=%s LVGL=%u.%u.%u port=2.8.0~1 (pin) PSRAM=%u bytes",
        esp_get_idf_version(), LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
        (unsigned)esp_psram_get_size());
    ESP_LOGI(TAG, "standalone: direct_mode=on avoid_tearing=on; 2 borrowed LCD frames=%u bytes; no extra draw buffer; rotation=0 DMA2D=off",
        (unsigned)(2 * CROWPANEL_FRAME_BYTES));
    return ESP_OK;
}
esp_err_t display_port_start(void)
{
    if (!s_display) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = display_port_lock(-1);
    if (ret != ESP_OK) return ret;
    lv_refr_now(s_display);
    ret = crowpanel_bsp_set_brightness(70);
    display_port_unlock();
    return ret;
}
esp_err_t display_port_lock(int32_t timeout_ms)
{
    if (!s_port) return ESP_ERR_INVALID_STATE;
    if (timeout_ms == 0) return ESP_ERR_INVALID_ARG; /* port uses 0 for infinity */
    return lvgl_port_lock(timeout_ms < 0 ? 0 : (uint32_t)timeout_ms) ? ESP_OK : ESP_ERR_TIMEOUT;
}
void display_port_unlock(void) { lvgl_port_unlock(); }
lv_disp_t *display_port_get_display(void) { return s_display; }
lv_indev_t *display_port_get_touch(void) { return s_touch; }
esp_err_t display_port_set_brightness(uint8_t percent) { return crowpanel_bsp_set_brightness(percent); }
display_port_diagnostics_t display_port_sample_gui_diagnostics(void)
{
    display_port_diagnostics_t d = {
        .internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        .internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        .spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
        .spiram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
        .gui_stack_hwm_bytes = (unsigned)uxTaskGetStackHighWaterMark(NULL),
    };
    ESP_LOGI(TAG, "MEM periodic internal=%u/%u SPIRAM=%u/%u GUI_stack_hwm=%u bytes",
        (unsigned)d.internal_free, (unsigned)d.internal_largest,
        (unsigned)d.spiram_free, (unsigned)d.spiram_largest, d.gui_stack_hwm_bytes);
    return d;
}
