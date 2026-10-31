#pragma once
#include <stdint.h>
#include "esp_err.h"

/* Standalone-only startup overlay. These calls acquire the display lock. */
esp_err_t startup_screen_show(void);
esp_err_t startup_screen_progress(uint8_t percent, const char *status);
esp_err_t startup_screen_finish(void);
esp_err_t startup_screen_destroy(void);
