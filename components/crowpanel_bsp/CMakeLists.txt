idf_component_register(SRCS "crowpanel_bsp.c" INCLUDE_DIRS "include"
    REQUIRES crowpanel_board esp_lcd espressif__esp_lcd_touch_gt911
    PRIV_REQUIRES espressif__esp_lcd_ek79007 esp_driver_gpio esp_driver_i2c esp_driver_ledc esp_hw_support esp_mm)
