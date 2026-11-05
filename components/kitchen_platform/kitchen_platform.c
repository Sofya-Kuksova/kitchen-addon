#include "kitchen_platform.h"
esp_err_t kitchen_platform_validate(const kitchen_platform_t *p)
{
    return p && p->lock && p->unlock && p->now_ms && p->navigate &&
        (!!p->play_chime == !!p->set_volume) &&
        ((!p->storage.load && !p->storage.submit && !p->storage.poll) ||
         (p->storage.load && p->storage.submit && p->storage.poll)) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
