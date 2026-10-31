#pragma once
#include "app_base.h"
#include "kitchen_platform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bind once before registration; operations/context must outlive every instance. */
esp_err_t kitchen_app_configure(const kitchen_platform_t *platform);
const app_descriptor_t *get_kitchen_app_desc(void);
#ifdef __cplusplus
}
#endif
