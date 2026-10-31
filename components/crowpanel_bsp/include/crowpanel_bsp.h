#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

typedef struct {
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t panel_io;
    esp_lcd_touch_handle_t touch;
} crowpanel_bsp_handles_t;

/* Single owner: display_port. No concurrent lifecycle calls. */
esp_err_t crowpanel_bsp_init(uint8_t frame_buffers, crowpanel_bsp_handles_t *out);
/* Adapter must be fully detached before releasing hardware. */
esp_err_t crowpanel_bsp_deinit(void);
/* Runtime caller holds display_port's lock; no brightness writes from ISR. */
esp_err_t crowpanel_bsp_set_brightness(uint8_t percent);
