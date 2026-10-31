#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Future host-main bridge; not linked by standalone and not applied to host. */
esp_err_t kitchen_host_configure(void);
#ifdef __cplusplus
}
#endif
