#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "kitchen_settings.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { KITCHEN_NAV_HOME, KITCHEN_NAV_HIDE } kitchen_nav_t;
typedef struct {
    void *context;
    /* esp_err_t, milliseconds; 0 means wait forever, never bool/ESP_OK mixing.
     * Lifecycle callers are outside the GUI lock; create_content/events are inside.
     * Model/view mutations are synchronous; storage owns copied byte records. */
    esp_err_t (*lock)(void *context, uint32_t timeout_ms);
    void (*unlock)(void *context);
    uint64_t (*now_ms)(void *context);
    /* Nonblocking enqueue only. Never call synchronous manager transitions here. */
    esp_err_t (*navigate)(void *context, int32_t app_id, kitchen_nav_t intent);
    /* Optional pair with set_volume. Nonblocking, GUI-safe, no retained pointers.
     * Called every service cycle, including empty events: synchronize validity
     * before enqueueing. Coalesce equal types ONLY within this batch; reject
     * unstarted stale sessions/deadlines. Host owns mixing and hardware lifetime. */
    esp_err_t (*submit_audio)(void *context, const kitchen_audio_batch_t *batch);
    esp_err_t (*set_volume)(void *context, uint8_t percent);
    /* Optional, nonblocking preview at the current volume, independent of chime. */
    esp_err_t (*play_volume_preview)(void *context);
    /* Optional as a complete group. The backend/context outlive all instances.
     * Only load may block, and it is called outside the GUI lock during create. */
    kitchen_storage_backend_t storage;
} kitchen_platform_t;
esp_err_t kitchen_platform_validate(const kitchen_platform_t *platform);
#ifdef __cplusplus
}
#endif
