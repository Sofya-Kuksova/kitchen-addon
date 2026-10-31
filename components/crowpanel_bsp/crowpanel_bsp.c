#include "crowpanel_bsp.h"
#include <string.h>
#include "crowpanel_board.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_ldo_regulator.h"
#include "esp_memory_utils.h"
#include "esp_cache.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "bsp";
static struct {
    i2c_master_bus_handle_t i2c;
    esp_ldo_channel_handle_t ldo;
    esp_lcd_dsi_bus_handle_t dsi;
    esp_lcd_panel_io_handle_t lcd_io, touch_io;
    esp_lcd_panel_handle_t panel;
    esp_lcd_touch_handle_t touch;
    esp_lcd_touch_io_gt911_config_t touch_config;
    bool pwm_timer, pwm_channel, ready;
} s;

esp_err_t crowpanel_bsp_set_brightness(uint8_t percent)
{
    if (percent > 100) return ESP_ERR_INVALID_ARG;
    if (!s.pwm_channel) return ESP_ERR_INVALID_STATE;
    const uint32_t duty = (1023U * percent) / 100U;
    /* Caller serializes writes; immediate PWM does not need the fade service. */
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty), TAG, "backlight duty");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

esp_err_t crowpanel_bsp_deinit(void)
{
    /* Stop on a deletion failure: never free a resource still used by a child. */
    if (s.pwm_channel) ESP_RETURN_ON_ERROR(crowpanel_bsp_set_brightness(0), TAG, "backlight off");
    if (s.touch) {
        ESP_RETURN_ON_ERROR(esp_lcd_touch_del(s.touch), TAG, "delete touch");
        s.touch = NULL;
    }
    if (s.touch_io) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_del(s.touch_io), TAG, "delete touch IO");
        s.touch_io = NULL;
    }
    if (s.i2c) {
        ESP_RETURN_ON_ERROR(i2c_del_master_bus(s.i2c), TAG, "delete I2C");
        s.i2c = NULL;
    }
    if (s.panel) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_del(s.panel), TAG, "delete LCD");
        s.panel = NULL;
    }
    if (s.lcd_io) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_del(s.lcd_io), TAG, "delete DBI");
        s.lcd_io = NULL;
    }
    if (s.dsi) {
        ESP_RETURN_ON_ERROR(esp_lcd_del_dsi_bus(s.dsi), TAG, "delete DSI");
        s.dsi = NULL;
    }
    if (s.ldo) {
        ESP_RETURN_ON_ERROR(esp_ldo_release_channel(s.ldo), TAG, "release PHY power");
        s.ldo = NULL;
    }
    if (s.pwm_channel) {
        const ledc_channel_config_t off = {
            .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0,
            .deconfigure = true,
        };
        ESP_RETURN_ON_ERROR(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0), TAG, "stop PWM");
        ESP_RETURN_ON_ERROR(ledc_channel_config(&off), TAG, "release PWM channel");
        s.pwm_channel = false;
    }
    if (s.pwm_timer) {
        ESP_RETURN_ON_ERROR(ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1), TAG, "pause PWM");
        const ledc_timer_config_t off = {
            .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_1,
            .deconfigure = true,
        };
        ESP_RETURN_ON_ERROR(ledc_timer_config(&off), TAG, "release PWM timer");
        s.pwm_timer = false;
    }
    /* Keep the backlight pin actively low even after shutdown. */
    ESP_RETURN_ON_ERROR(gpio_set_direction(CROWPANEL_BACKLIGHT, GPIO_MODE_OUTPUT), TAG, "backlight GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(CROWPANEL_BACKLIGHT, 0), TAG, "backlight low");
    s.ready = false;
    return ESP_OK;
}

esp_err_t crowpanel_bsp_init(uint8_t frame_buffers, crowpanel_bsp_handles_t *out)
{
    if (!out || frame_buffers != 2) return ESP_ERR_INVALID_ARG;
    *out = (crowpanel_bsp_handles_t){0};
    if (s.ready || s.dsi || s.ldo || s.pwm_timer || s.i2c) return ESP_ERR_INVALID_STATE;
    const crowpanel_board_config_t *b = crowpanel_board_get_config();
    esp_err_t ret;
    ESP_RETURN_ON_ERROR(crowpanel_board_validate(), TAG, "hardware configuration");
    const gpio_config_t dark = {
        .pin_bit_mask = 1ULL << CROWPANEL_BACKLIGHT, .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&dark), TAG, "backlight GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(CROWPANEL_BACKLIGHT, 0), TAG, "backlight low");
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1, .freq_hz = 30000, .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_GOTO_ON_ERROR(ledc_timer_config(&timer), fail, TAG, "PWM timer");
    s.pwm_timer = true;
    const ledc_channel_config_t pwm = {
        .gpio_num = CROWPANEL_BACKLIGHT, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_1, .duty = 0,
    };
    ESP_GOTO_ON_ERROR(ledc_channel_config(&pwm), fail, TAG, "PWM channel");
    s.pwm_channel = true;
    ESP_LOGI(TAG, "stage=PHY power");
    const esp_ldo_channel_config_t ldo = {.chan_id = 3, .voltage_mv = 2500};
    ESP_GOTO_ON_ERROR(esp_ldo_acquire_channel(&ldo, &s.ldo), fail, TAG, "LDO3 2500mV");
    const esp_lcd_dsi_bus_config_t bus = {
        .bus_id = 0, .num_data_lanes = 2,
#if CONFIG_KITCHEN_SILICON_LEGACY
        .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT_LEGACY,
#else
        .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT,
#endif
        .lane_bit_rate_mbps = b->lane_mbps,
    };
    ESP_LOGI(TAG, "stage=DSI/DBI");
    ESP_GOTO_ON_ERROR(esp_lcd_new_dsi_bus(&bus, &s.dsi), fail, TAG, "DSI bus");
    const esp_lcd_dbi_io_config_t dbi = {.virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8};
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(s.dsi, &dbi, &s.lcd_io), fail, TAG, "DBI IO");
    const esp_lcd_dpi_panel_config_t dpi = {
        .virtual_channel = 0, .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 51, .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565, .num_fbs = frame_buffers,
        .video_timing = {
            .h_size = CROWPANEL_LCD_WIDTH, .v_size = CROWPANEL_LCD_HEIGHT,
            .hsync_back_porch = b->hbp, .hsync_pulse_width = b->hpw, .hsync_front_porch = b->hfp,
            .vsync_back_porch = b->vbp, .vsync_pulse_width = b->vpw, .vsync_front_porch = b->vfp,
        },
    };
    ek79007_vendor_config_t vendor = {
        .mipi_config = {.dsi_bus = s.dsi, .dpi_config = &dpi, .lane_num = 2},
    };
    const esp_lcd_panel_dev_config_t panel = {
        .reset_gpio_num = b->lcd_reset, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16, .vendor_config = &vendor,
    };
    ESP_LOGI(TAG, "stage=EK79007/framebuffers");
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ek79007(s.lcd_io, &panel, &s.panel), fail, TAG, "LCD creation");
    void *fb0 = NULL, *fb1 = NULL;
    ESP_GOTO_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(s.panel, 2, &fb0, &fb1), fail, TAG, "framebuffers");
    ESP_GOTO_ON_FALSE(fb0 && fb1 && fb0 != fb1 && esp_ptr_external_ram(fb0) && esp_ptr_external_ram(fb1),
        ESP_ERR_INVALID_STATE, fail, TAG, "two distinct PSRAM framebuffers required");
    memset(fb0, 0, CROWPANEL_FRAME_BYTES);
    memset(fb1, 0, CROWPANEL_FRAME_BYTES);
    ESP_GOTO_ON_ERROR(esp_cache_msync(fb0, CROWPANEL_FRAME_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M), fail, TAG, "sync FB0");
    ESP_GOTO_ON_ERROR(esp_cache_msync(fb1, CROWPANEL_FRAME_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M), fail, TAG, "sync FB1");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(s.panel), fail, TAG, "LCD reset");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(s.panel), fail, TAG, "LCD init");
    /* This exact driver has no disp_on_off hook; DPI init starts scanout. */
    ESP_LOGI(TAG, "stage=I2C/GT911");
    const i2c_master_bus_config_t i2c = {
        .i2c_port = 0, .sda_io_num = CROWPANEL_I2C_SDA, .scl_io_num = CROWPANEL_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_GOTO_ON_ERROR(i2c_new_master_bus(&i2c, &s.i2c), fail, TAG, "I2C master");
    esp_lcd_panel_io_i2c_config_t touch_io = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    touch_io.dev_addr = b->touch_address;
    touch_io.scl_speed_hz = 400000;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_i2c(s.i2c, &touch_io, &s.touch_io), fail, TAG, "touch IO");
    s.touch_config.dev_addr = b->touch_address;
    const esp_lcd_touch_config_t touch = {
        .x_max = CROWPANEL_LCD_WIDTH, .y_max = CROWPANEL_LCD_HEIGHT,
        .rst_gpio_num = CROWPANEL_TOUCH_RESET, .int_gpio_num = CROWPANEL_TOUCH_INT,
        .levels = {.reset = 0, .interrupt = 0}, .driver_data = &s.touch_config,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_touch_new_i2c_gt911(s.touch_io, &touch, &s.touch), fail, TAG, "GT911 reset/init");
    s.ready = true;
    *out = (crowpanel_bsp_handles_t){.panel = s.panel, .panel_io = s.lcd_io, .touch = s.touch};
    return ESP_OK;
fail:
    ESP_LOGE(TAG, "init failed: %s (0x%x); no retry", esp_err_to_name(ret), (unsigned)ret);
    esp_err_t cleanup = crowpanel_bsp_deinit();
    if (cleanup != ESP_OK) ESP_LOGE(TAG, "cleanup failed: %s; resources retained", esp_err_to_name(cleanup));
    return ret;
}
