#pragma once
#include "esp_err.h"
#include "lvgl.h"

/* Standalone diagnostics/brightness only; never import into embedded kitchen. */
esp_err_t baseline_ui_create(const char *identity);
/* Must complete before display_port_deinit; serialized by app_main. */
esp_err_t baseline_ui_destroy(void);

/* Standalone fixture only; caller holds the display lock. */
lv_obj_t *baseline_ui_get_root(void);
