#include "kitchen_app.h"
#include "kitchen_core.h"
#include "kitchen_ui.h"
#include <stdlib.h>
#include "esp_log.h"

typedef struct {
    app_base_t base;
    kitchen_platform_t platform;
    kitchen_state_t state;
    kitchen_settings_t settings;
    kitchen_ui_t ui;
    lv_timer_t *heartbeat;
    bool running, paused, suspended, fullscreen;
} kitchen_app_t;
static kitchen_platform_t s_platform;
static unsigned s_instances;
static bool s_configured;

esp_err_t kitchen_app_configure(const kitchen_platform_t *p)
{
    if (s_instances) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = kitchen_platform_validate(p);
    if (ret != ESP_OK) return ret;
    s_platform = *p; s_configured = true;
    return ESP_OK;
}
static void update_view(kitchen_app_t *app)
{
    kitchen_ui_set_active(&app->ui, app->running && !app->paused && !app->suspended && app->fullscreen);
}
static uint64_t now_ms(void *ctx)
{
    kitchen_app_t *app = ctx;
    return app->platform.now_ms(app->platform.context);
}
static void service(kitchen_app_t *app)
{
    kitchen_settings_poll(&app->settings);
    kitchen_core_update(&app->state, now_ms(app));
    kitchen_audio_batch_t batch;
    kitchen_core_take_audio(&app->state, &batch);
    if (app->platform.submit_audio) {
        esp_err_t ret = app->platform.submit_audio(app->platform.context, &batch);
        if (ret != ESP_OK) ESP_LOGW("kitchen", "audio enqueue failed: %s", esp_err_to_name(ret));
    }
}
static void heartbeat(lv_timer_t *timer)
{
    kitchen_app_t *app = timer->user_data;
    if (app->running) service(app);
}
static bool command(void *ctx, const kitchen_command_t *cmd)
{
    kitchen_app_t *app = ctx;
    if (cmd->kind == KITCHEN_RETRY_SAVE || cmd->kind == KITCHEN_SAVE_VOLUME) {
        if (cmd->kind == KITCHEN_RETRY_SAVE) kitchen_settings_retry(&app->settings, &app->state);
        else kitchen_settings_request(&app->settings, &app->state, 0);
        service(app);
        return true;
    }
    if (cmd->kind == KITCHEN_PREVIEW_VOLUME) {
        service(app);
        if (!app->state.volume_percent) return true;
        if (!app->platform.play_volume_preview) return false;
        esp_err_t ret = app->platform.play_volume_preview(app->platform.context);
        if (ret != ESP_OK) ESP_LOGW("kitchen", "volume preview failed: %s", esp_err_to_name(ret));
        return ret == ESP_OK;
    }
    uint8_t previous_volume = app->state.volume_percent;
    kitchen_timer_t *timer = kitchen_core_find(&app->state, cmd->id);
    kitchen_status_t previous_status = timer ? timer->status : KITCHEN_EMPTY;
    bool changed = kitchen_core_apply(&app->state, cmd, now_ms(app));
    if (changed && cmd->kind == KITCHEN_SET_VOLUME && app->platform.set_volume) {
        esp_err_t ret = app->platform.set_volume(app->platform.context, app->state.volume_percent);
        if (ret != ESP_OK) {
            app->state.volume_percent = previous_volume;
            changed = false;
            ESP_LOGW("kitchen", "volume update failed: %s", esp_err_to_name(ret));
        }
    }
    if (changed && (cmd->kind == KITCHEN_CREATE || cmd->kind == KITCHEN_EDIT ||
        (previous_status == KITCHEN_READY && (cmd->kind == KITCHEN_ADJUST || cmd->kind == KITCHEN_SET_TIME))))
        kitchen_settings_request(&app->settings, &app->state, cmd->id);
    service(app);
    return changed;
}
static void back(void *ctx)
{
    kitchen_app_t *app = ctx;
    esp_err_t ret = app->platform.navigate(app->platform.context, app->base.id, KITCHEN_NAV_HOME);
    if (ret != ESP_OK) ESP_LOGW("kitchen", "navigation enqueue failed: %s", esp_err_to_name(ret));
}
static esp_err_t create(app_base_t *self)
{
    if (!s_configured) return ESP_ERR_INVALID_STATE;
    kitchen_app_t *app = (kitchen_app_t *)self;
    app->platform = s_platform;
    kitchen_core_init(&app->state);
    /* create is outside GUI-lock; load also orders earlier accepted writes. */
    kitchen_settings_init(&app->settings, &app->state, &app->platform.storage);
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key) {
        kitchen_storage_result_t result = app->settings.entries[key].loaded;
        if (result != KITCHEN_STORAGE_OK && result != KITCHEN_STORAGE_MISSING)
            ESP_LOGW("kitchen", "settings key %u: %s (result %d); using safe defaults", key,
                app->settings.entries[key].load_reason ? app->settings.entries[key].load_reason : "invalid backend result", (int)result);
    }
    if (app->platform.set_volume) {
        esp_err_t ret = app->platform.set_volume(app->platform.context, app->state.volume_percent);
        if (ret != ESP_OK) return ret;
    }
    ++s_instances;
    return ESP_OK;
}
static void invalidate_audio(kitchen_app_t *app)
{
    app->state.pending_warnings = app->state.pending_done = 0;
    if (app->platform.submit_audio) {
        const kitchen_audio_batch_t empty = {0};
        (void)app->platform.submit_audio(app->platform.context, &empty);
    }
}
static esp_err_t destroy(app_base_t *self)
{
    kitchen_app_t *app = (kitchen_app_t *)self;
    /* Manager frees self regardless of return: cleanup must finish synchronously.
     * No worker/join/queued payload can wait for the manager mutex here. */
    esp_err_t ret = app->platform.lock(app->platform.context, 0);
    if (ret != ESP_OK) { ESP_LOGE("kitchen", "platform violated infinite-lock contract"); abort(); }
    if (app->heartbeat) { lv_timer_del(app->heartbeat); app->heartbeat = NULL; }
    kitchen_ui_destroy(&app->ui);
    invalidate_audio(app);
    app->running = false;
    app->platform.unlock(app->platform.context);
    --s_instances;
    return ESP_OK;
}
static esp_err_t lifecycle(app_base_t *self, unsigned operation)
{
    kitchen_app_t *app = (kitchen_app_t *)self;
    esp_err_t ret = app->platform.lock(app->platform.context, 1000);
    if (ret != ESP_OK) return ret;
    if (operation == 0) {
        if (!app->heartbeat) app->heartbeat = lv_timer_create(heartbeat, 50, app);
        if (!app->heartbeat) { app->platform.unlock(app->platform.context); return ESP_ERR_NO_MEM; }
        lv_timer_resume(app->heartbeat);
        lv_timer_ready(app->heartbeat);
    }
    switch (operation) {
    case 0: app->running = true; app->paused = false; app->suspended = false; break;
    case 1:
        invalidate_audio(app);
        app->running = false; app->fullscreen = false;
        if (app->heartbeat) lv_timer_pause(app->heartbeat);
        kitchen_ui_destroy(&app->ui); break;
    case 2: app->paused = true; break;
    case 3: app->paused = false; break;
    case 4: app->fullscreen = false; break;
    case 5: app->fullscreen = true; break;
    case 6: app->suspended = true; break;
    case 7: app->suspended = false; break;
    }
    update_view(app);
    app->platform.unlock(app->platform.context);
    return ESP_OK;
}
#define LIFECYCLE(name, op) static esp_err_t name(app_base_t *self, size_t timeout) \
    { (void)timeout; return lifecycle(self, op); }
LIFECYCLE(start, 0)
LIFECYCLE(stop, 1)
LIFECYCLE(pause_app, 2)
LIFECYCLE(resume_app, 3)
LIFECYCLE(minimize, 4)
LIFECYCLE(restore, 5)
static esp_err_t before_suspend(app_base_t *self) { return lifecycle(self, 6); }
static esp_err_t before_resume(app_base_t *self) { return lifecycle(self, 7); }
static esp_err_t before_close(app_base_t *self, bool *allow)
{
    (void)self;
    if (!allow) return ESP_ERR_INVALID_ARG;
    *allow = true;
    return ESP_OK;
}
static esp_err_t before_back(app_base_t *self) { (void)self; return ESP_OK; }
static esp_err_t view_change(app_base_t *self, app_view_t from, app_view_t to)
{
    (void)from;
    kitchen_app_t *app = (kitchen_app_t *)self;
    esp_err_t ret = app->platform.lock(app->platform.context, 1000);
    if (ret != ESP_OK) return ret;
    app->fullscreen = to == APP_VIEW_FULLSCREEN;
    update_view(app);
    app->platform.unlock(app->platform.context);
    return ESP_OK;
}
static esp_err_t create_content(app_base_t *self, lv_obj_t *container, app_view_t view)
{
    /* Called in GUI job context: do not acquire a possibly nonrecursive lock. */
    if (view != APP_VIEW_FULLSCREEN) return ESP_ERR_NOT_SUPPORTED;
    kitchen_app_t *app = (kitchen_app_t *)self;
    esp_err_t ret = kitchen_ui_create(&app->ui, container, &app->state, &app->settings, command, now_ms, back,
        app->platform.submit_audio && app->platform.set_volume, app);
    if (ret == ESP_OK) update_view(app);
    return ret;
}
const app_descriptor_t *get_kitchen_app_desc(void)
{
    static const app_descriptor_t descriptor = {
        .name = "kitchen", .instance_size = sizeof(kitchen_app_t),
        .launch_policy = APP_LAUNCH_POLICY_NONE,
        .caps = {.ui_caps = {.can_fullscreen = 1, .can_widget = 0},
                 .run_caps = {.can_background = 0, .can_suspend = 1}, .priority = 0},
        .iface = {.create = create, .destroy = destroy, .start = start, .stop = stop,
            .pause = pause_app, .resume = resume_app, .minimize = minimize, .restore = restore,
            .create_content = create_content, .on_before_suspend = before_suspend,
            .on_before_resume = before_resume, .on_before_close = before_close,
            .on_before_back = before_back, .on_view_change = view_change},
    };
    return &descriptor;
}
