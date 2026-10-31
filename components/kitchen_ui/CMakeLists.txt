idf_component_register(SRCS "kitchen_ui.c" "kitchen_fonts.c" INCLUDE_DIRS "include"
    REQUIRES kitchen_core lvgl__lvgl esp_common)
