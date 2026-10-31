#pragma once
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { KITCHEN_NAV_HOME, KITCHEN_NAV_HIDE } kitchen_nav_t;
typedef struct {
    void *context;
    /* esp_err_t, milliseconds; 0 means wait forever, never bool/ESP_OK mixing.
     * Lifecycle callers are outside the GUI lock; create_content/events are inside.
     * No dispatch/queue exists in v1: all model/view mutations are synchronous. */
    esp_err_t (*lock)(void *context, uint32_t timeout_ms);
    void (*unlock)(void *context);
    uint64_t (*now_ms)(void *context);
    /* Nonblocking enqueue only. Never call synchronous manager transitions here. */
    esp_err_t (*navigate)(void *context, int32_t app_id, kitchen_nav_t intent);
    /* Optional audio service. Both callbacks must be nonblocking, GUI-safe and
     * retain no app/UI pointers. The host owns hardware and playback lifetime. */
    esp_err_t (*play_chime)(void *context);
    esp_err_t (*set_volume)(void *context, uint8_t percent);
    /* Optional, nonblocking preview at the current volume, independent of chime. */
    esp_err_t (*play_volume_preview)(void *context);
} kitchen_platform_t;
esp_err_t kitchen_platform_validate(const kitchen_platform_t *platform);
#ifdef __cplusplus
}
#endif
