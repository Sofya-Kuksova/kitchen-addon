#include "standalone_runner.h"
#include "standalone_audio.h"
#include <stdlib.h>
#include <string.h>
#include "display_port.h"
#include "kitchen_app.h"
#include "esp_log.h"
#include "esp_timer.h"

static app_base_t *s_app;
static lv_obj_t *s_parent;
static esp_err_t lock(void *ctx, uint32_t ms) { (void)ctx; return display_port_lock(ms ? (int32_t)ms : -1); }
static void unlock(void *ctx) { (void)ctx; display_port_unlock(); }
static uint64_t now_ms(void *ctx) { (void)ctx; return (uint64_t)esp_timer_get_time()/1000; }
static esp_err_t navigate(void *ctx, int32_t id, kitchen_nav_t nav)
{
    (void)ctx; (void)id; (void)nav;
    /* Standalone has no host home screen. Kitchen screen navigation stays in its UI. */
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t standalone_runner_run(void)
{
    if (s_app) return ESP_ERR_INVALID_STATE;
    esp_err_t audio = standalone_audio_init();
    if (audio != ESP_OK) ESP_LOGW("kitchen", "sound unavailable: %s", esp_err_to_name(audio));
    const kitchen_platform_t platform = {.lock = lock, .unlock = unlock, .now_ms = now_ms, .navigate = navigate,
        .play_chime = audio == ESP_OK ? standalone_audio_chime : NULL,
        .set_volume = audio == ESP_OK ? standalone_audio_volume : NULL,
        .play_volume_preview = audio == ESP_OK ? standalone_audio_preview : NULL};
    esp_err_t ret = kitchen_app_configure(&platform);
    if (ret != ESP_OK) return ret;
    const app_descriptor_t *descriptor = get_kitchen_app_desc();
    s_app = calloc(1, descriptor->instance_size);
    if (!s_app) return ESP_ERR_NO_MEM;
    const app_base_t base = {.desc = descriptor, .id = 0};
    memcpy(s_app, &base, sizeof(base));
    ret = APP_VCALL(s_app, create);
    if (ret != ESP_OK) { free(s_app); s_app = NULL; return ret; }
    ret = APP_VCALL(s_app, start, 0);
    if (ret != ESP_OK) goto fail;
    ret = display_port_lock(-1);
    if (ret != ESP_OK) goto fail;
    s_parent = lv_obj_create(lv_disp_get_scr_act(display_port_get_display()));
    if (s_parent) {
        lv_obj_remove_style_all(s_parent);
        lv_obj_set_size(s_parent, 1024, 600);
        lv_obj_clear_flag(s_parent, LV_OBJ_FLAG_SCROLLABLE);
        ret = APP_VCALL(s_app, create_content, s_parent, APP_VIEW_FULLSCREEN);
    } else ret = ESP_ERR_NO_MEM;
    display_port_unlock();
    if (ret != ESP_OK) goto fail;
    ret = APP_VCALL(s_app, on_view_change, APP_VIEW_NONE, APP_VIEW_FULLSCREEN);
    if (ret != ESP_OK) goto fail;
    display_port_log_memory("kitchen fullscreen");
    return ESP_OK;
fail:
    /* Release the owned application before the borrowed parent. No hardware teardown. */
    {
        esp_err_t cleanup = APP_VCALL(s_app, stop, 0);
        if (cleanup == ESP_OK) cleanup = APP_VCALL(s_app, destroy);
        if (cleanup == ESP_OK) {
            free(s_app); s_app = NULL;
            cleanup = display_port_lock(-1);
            if (cleanup == ESP_OK) {
                if (s_parent) lv_obj_del(s_parent);
                s_parent = NULL;
                display_port_unlock();
            }
        }
        if (cleanup != ESP_OK) ESP_LOGE("kitchen", "startup cleanup: %s", esp_err_to_name(cleanup));
    }
    return ret;
}
