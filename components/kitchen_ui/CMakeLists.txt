idf_component_register(SRCS "kitchen_ui.c" "kitchen_fonts.c" "kitchen_font_extensions.c" "kitchen_status_24.c" "kitchen_large_assets.c" "kitchen_overlay_fonts.c" INCLUDE_DIRS "include"
    REQUIRES kitchen_core lvgl__lvgl esp_common)
