#include "standalone_runner.h"
#include "startup_screen.h"
#include "crowpanel_board.h"
#include "display_port.h"
#include "esp_log.h"

void app_main(void)
{
    const char *TAG = "kitchen-addon";
    crowpanel_board_log();
    display_port_log_memory("before init");
    esp_err_t ret = crowpanel_board_validate();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "profile validation: %s", esp_err_to_name(ret));
        return;
    }
    ret = display_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "display initialization: %s; startup stopped", esp_err_to_name(ret));
        return;
    }
    ret = startup_screen_show();
    if (ret == ESP_OK) ret = display_port_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UI/start: %s; startup stopped", esp_err_to_name(ret));
        esp_err_t cleanup = startup_screen_destroy();
        if (cleanup != ESP_OK) ESP_LOGE(TAG, "loading cleanup: %s", esp_err_to_name(cleanup));
        cleanup = display_port_deinit();
        if (cleanup != ESP_OK) ESP_LOGE(TAG, "display cleanup: %s", esp_err_to_name(cleanup));
        return;
    }
    ret = startup_screen_progress(30, "Preparing timers");
    if (ret == ESP_OK) ret = standalone_runner_run();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "kitchen startup: %s", esp_err_to_name(ret));
        (void)startup_screen_progress(30, "Unable to start. Restart the panel.");
        return;
    }
    ret = startup_screen_progress(100, "Ready");
    if (ret == ESP_OK) ret = startup_screen_finish();
    if (ret != ESP_OK) ESP_LOGE(TAG, "loading finish: %s", esp_err_to_name(ret));
    display_port_log_memory("after UI/start");
}
