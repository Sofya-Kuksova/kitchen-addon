#include "crowpanel_board.h"
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_log.h"

#if CONFIG_KITCHEN_BOARD_UNSELECTED || CONFIG_KITCHEN_LCD_UNSELECTED || CONFIG_KITCHEN_SILICON_UNSELECTED
#error "Explicit board/LCD/silicon selection is required"
#endif
#if CONFIG_KITCHEN_BOARD_V13 != CONFIG_KITCHEN_LCD_V13
#error "Incompatible board and LCD profiles"
#endif
#if CONFIG_KITCHEN_SILICON_LEGACY
#if !CONFIG_ESP32P4_SELECTS_REV_LESS_V3 || CONFIG_ESP_REV_MIN_FULL != 0
#error "Legacy silicon branch/min revision mismatch"
#endif
#else
#if CONFIG_ESP32P4_SELECTS_REV_LESS_V3 || CONFIG_ESP_REV_MIN_FULL != 300
#error "V3 silicon branch/min revision mismatch"
#endif
#endif

_Static_assert(CROWPANEL_BACKLIGHT != CROWPANEL_TOUCH_RESET, "GPIO conflict");
_Static_assert(CROWPANEL_I2C_SDA != CROWPANEL_I2C_SCL, "GPIO conflict");
_Static_assert(CROWPANEL_FRAME_BYTES == 1228800U, "RGB565 frame budget");

#define PIN_BIT(pin) (1ULL << (pin))
#define COMMON_ACTIVE_MASK (PIN_BIT(CROWPANEL_I2C_SDA) | PIN_BIT(CROWPANEL_I2C_SCL) | \
    PIN_BIT(CROWPANEL_TOUCH_RESET) | PIN_BIT(CROWPANEL_TOUCH_INT) | PIN_BIT(CROWPANEL_BACKLIGHT))
_Static_assert(__builtin_popcountll(COMMON_ACTIVE_MASK) == 5, "Active GPIO conflict");
#if CONFIG_KITCHEN_LCD_V13
_Static_assert((COMMON_ACTIVE_MASK & PIN_BIT(CROWPANEL_LCD_RESET)) == 0, "LCD reset GPIO conflict");
#endif

static const crowpanel_board_config_t s_config = {
#if CONFIG_KITCHEN_BOARD_V10
    .board_name = "HW V1.0", .sdio_known = true,
    .sdio_data = {14, 15, 16, 17},
#elif CONFIG_KITCHEN_BOARD_V11
    .board_name = "HW V1.1", .sdio_known = true,
    .sdio_data = {17, 16, 15, 14},
#elif CONFIG_KITCHEN_BOARD_V12
    .board_name = "HW V1.2", .sdio_known = true,
    .sdio_data = {17, 16, 15, 14},
#else
    .board_name = "HW V1.3 candidate", .candidate = true,
    .sdio_known = false,
    .sdio_data = {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC},
#endif
#if CONFIG_KITCHEN_BOARD_V13
    .sdio_clk = GPIO_NUM_NC, .sdio_cmd = GPIO_NUM_NC, .c6_reset = GPIO_NUM_NC,
#else
    .sdio_clk = 18, .sdio_cmd = 19, .c6_reset = 32,
#endif
#if CONFIG_KITCHEN_BOARD_V10 || CONFIG_KITCHEN_BOARD_V11
    .wireless_a = 53, .wireless_b = 54, .wireless_confirmed = true,
#else
    .wireless_a = 27, .wireless_b = 28,
#if CONFIG_KITCHEN_BOARD_V13
    .wireless_confirmed = false,
#else
    .wireless_confirmed = true,
#endif
#endif
#if CONFIG_KITCHEN_LCD_V13
    .lcd_name = "EK79007 V1.3 candidate", .lcd_reset = CROWPANEL_LCD_RESET,
    .lane_mbps = 900, .hbp = 90, .hpw = 70, .hfp = 160,
    .vbp = 13, .vpw = 10, .vfp = 21,
#else
    .lcd_name = "EK79007 legacy", .lcd_reset = CROWPANEL_LCD_RESET,
    .lane_mbps = 1000, .hbp = 160, .hpw = 70, .hfp = 160,
    .vbp = 23, .vpw = 10, .vfp = 12,
#endif
#if CONFIG_KITCHEN_SILICON_LEGACY
    .silicon_name = "ESP32-P4 rev 1.x (0.x compatible)",
#else
    .silicon_name = "ESP32-P4 rev 3.x",
#endif
#if CONFIG_KITCHEN_TOUCH_ADDRESS_14
    .touch_address = 0x14,
#else
    .touch_address = 0x5d,
#endif
};

const crowpanel_board_config_t *crowpanel_board_get_config(void) { return &s_config; }

esp_err_t crowpanel_board_validate(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    if (chip.model != CHIP_ESP32P4 ||
        chip.revision > CONFIG_ESP_REV_MAX_FULL) return ESP_ERR_NOT_SUPPORTED;
#if CONFIG_ESP_REV_MIN_FULL > 0
    if (chip.revision < CONFIG_ESP_REV_MIN_FULL) return ESP_ERR_NOT_SUPPORTED;
#endif
    const gpio_num_t active[] = {CROWPANEL_I2C_SDA, CROWPANEL_I2C_SCL,
        CROWPANEL_TOUCH_RESET, CROWPANEL_TOUCH_INT, CROWPANEL_BACKLIGHT, s_config.lcd_reset};
    for (unsigned i = 0; i < sizeof(active) / sizeof(active[0]); ++i) {
        if (active[i] == GPIO_NUM_NC) continue;
        if (!GPIO_IS_VALID_OUTPUT_GPIO(active[i])) return ESP_ERR_INVALID_ARG;
        for (unsigned j = i + 1; j < sizeof(active) / sizeof(active[0]); ++j) {
            if (active[i] == active[j]) return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_OK;
}

void crowpanel_board_log(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI("board", "%s | %s | %s | actual chip=%u.%u | build/device=UNTESTED",
        s_config.board_name, s_config.lcd_name, s_config.silicon_name,
        (unsigned)(chip.revision / 100), (unsigned)(chip.revision % 100));
    ESP_LOGI("board", "DSI=2x%uMbps DPI=51MHz 1024x600 RGB565 reset=%d H=%u/%u/%u V=%u/%u/%u",
        s_config.lane_mbps, s_config.lcd_reset, s_config.hbp, s_config.hpw, s_config.hfp,
        s_config.vbp, s_config.vpw, s_config.vfp);
    ESP_LOGI("board", "CPU=%dMHz PSRAM=%dMHz HEX; LDO3=2500mV GT911=0x%02x; SDIO/slot not initialized",
        CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, CONFIG_SPIRAM_SPEED, s_config.touch_address);
}
