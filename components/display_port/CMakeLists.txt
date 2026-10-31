idf_component_register(SRCS "display_port.c" INCLUDE_DIRS "include"
    REQUIRES lvgl__lvgl esp_common
    PRIV_REQUIRES crowpanel_bsp espressif__esp_lvgl_port crowpanel_board heap esp_system esp_psram)
if(NOT CONFIG_LV_COLOR_DEPTH EQUAL 16 OR CONFIG_LV_COLOR_16_SWAP)
    message(FATAL_ERROR "Standalone requires native RGB565 LVGL8")
endif()
