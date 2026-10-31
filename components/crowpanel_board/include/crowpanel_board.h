#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "sdkconfig.h"

#define CROWPANEL_LCD_WIDTH 1024
#define CROWPANEL_LCD_HEIGHT 600
#define CROWPANEL_FRAME_BYTES (1024U * 600U * 2U)
#define CROWPANEL_I2C_SDA GPIO_NUM_45
#define CROWPANEL_I2C_SCL GPIO_NUM_46
#define CROWPANEL_TOUCH_RESET GPIO_NUM_40
#define CROWPANEL_TOUCH_INT GPIO_NUM_42
#define CROWPANEL_BACKLIGHT GPIO_NUM_31
#if CONFIG_KITCHEN_LCD_V13
#define CROWPANEL_LCD_RESET GPIO_NUM_41
#else
#define CROWPANEL_LCD_RESET GPIO_NUM_NC
#endif

typedef struct {
    const char *board_name;
    const char *lcd_name;
    const char *silicon_name;
    bool candidate;
    bool sdio_known;
    gpio_num_t sdio_data[4];
    gpio_num_t sdio_clk, sdio_cmd, c6_reset;
    gpio_num_t wireless_a, wireless_b;
    bool wireless_confirmed;
    gpio_num_t lcd_reset;
    unsigned lane_mbps;
    unsigned hbp, hpw, hfp, vbp, vpw, vfp;
    uint8_t touch_address;
} crowpanel_board_config_t;

const crowpanel_board_config_t *crowpanel_board_get_config(void);
esp_err_t crowpanel_board_validate(void);
void crowpanel_board_log(void);
