#pragma once
#include <string.h>
#include "sdkconfig.h"
#include "nvs.h"
#include "esp_log.h"

/* Explicit development reset, after NVS init and before Kitchen opens storage.
 * Marker is outside the erased namespace. No app/UI instances may exist yet. */
static esp_err_t kitchen_reset_once(void)
{
    const char *token = CONFIG_KITCHEN_RESET_TOKEN;
    if (!token[0]) return ESP_OK;
    char previous[64];
    if (strlen(token) >= sizeof(previous)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t marker;
    esp_err_t err = nvs_open("kitchen_reset", NVS_READWRITE, &marker);
    if (err != ESP_OK) return err;
    size_t length = sizeof(previous);
    err = nvs_get_str(marker, "token", previous, &length);
    if (err == ESP_OK && strcmp(previous, token) == 0) {
        nvs_close(marker);
        return ESP_OK;
    }
    /* Refuse unexpected marker corruption rather than erasing silently. */
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(marker);
        return err;
    }
    nvs_handle_t timers;
    err = nvs_open("kitchen", NVS_READWRITE, &timers);
    if (err == ESP_OK) {
        err = nvs_erase_all(timers);
        if (err == ESP_OK) err = nvs_commit(timers);
        nvs_close(timers);
    }
    if (err == ESP_OK) err = nvs_set_str(marker, "token", token);
    if (err == ESP_OK) err = nvs_commit(marker);
    nvs_close(marker);
    if (err == ESP_OK) ESP_LOGW("kitchen_storage", "One-time reset of Kitchen settings completed");
    return err;
}
