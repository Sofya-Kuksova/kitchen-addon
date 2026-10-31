#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"

typedef struct {
    size_t internal_free, internal_largest, spiram_free, spiram_largest;
    unsigned gui_stack_hwm_bytes;
} display_port_diagnostics_t;

/* Standalone only. Port/worker/hardware live until reset once initialized.
 * deinit darkens the display and returns NOT_SUPPORTED for a live port. */
esp_err_t display_port_init(void);
esp_err_t display_port_start(void);
esp_err_t display_port_deinit(void);
/* -1 = forever, >0 = milliseconds, 0 rejected (port itself uses 0 = forever). */
esp_err_t display_port_lock(int32_t timeout_ms);
void display_port_unlock(void);
/* Getters and runtime brightness calls require this lock or an LVGL callback. */
lv_disp_t *display_port_get_display(void);
lv_indev_t *display_port_get_touch(void);
esp_err_t display_port_set_brightness(uint8_t percent);
/* Invoke only from an LVGL timer: HWM then measures the GUI worker itself. */
display_port_diagnostics_t display_port_sample_gui_diagnostics(void);
void display_port_log_memory(const char *stage);
