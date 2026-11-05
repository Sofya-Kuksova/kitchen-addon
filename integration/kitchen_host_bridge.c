#include "kitchen_host_bridge.h"
#include "kitchen_app.h"
#include "app_manager.h"
#include "bsp/esp32_p4_function_ev_board.h"
#include "esp_timer.h"

static esp_err_t lock(void *ctx, uint32_t timeout_ms)
{
    (void)ctx;
    return bsp_display_lock(timeout_ms) ? ESP_OK : ESP_ERR_TIMEOUT;
}
static void unlock(void *ctx) { (void)ctx; bsp_display_unlock(); }
static uint64_t now_ms(void *ctx) { (void)ctx; return (uint64_t)esp_timer_get_time()/1000; }
static esp_err_t navigate(void *ctx, int32_t id, kitchen_nav_t intent)
{
    (void)ctx;
    app_nav_intent_t nav;
    switch (intent) {
    case KITCHEN_NAV_HOME: nav = APP_NAV_INTENT_BACK_TO_HOMESCREEN; break;
    case KITCHEN_NAV_HIDE: nav = APP_NAV_INTENT_HIDE; break;
    default: return ESP_ERR_INVALID_ARG;
    }
    /* Public intent queue, 0 ticks: never wait for app_manager's mutex in GUI. */
    return app_manager_send_navigation_event(id, nav, 0);
}
esp_err_t kitchen_host_configure(void)
{
    /* No host storage API is assumed. Without this optional group Kitchen
     * works in RAM and displays a persistence-unavailable notice. */
    const kitchen_platform_t p = {.lock = lock, .unlock = unlock, .now_ms = now_ms, .navigate = navigate,
        .storage = {0}};
    return kitchen_app_configure(&p);
}
