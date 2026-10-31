idf_component_register(SRCS "app_main.c" "standalone_runner.c" "startup_screen.c" "standalone_audio.c" INCLUDE_DIRS "."
    PRIV_REQUIRES crowpanel_board display_port kitchen_app kitchen_ui esp_timer esp_system esp_driver_i2s esp_driver_gpio
    EMBED_FILES "assets/kitchen_timer_chime.wav" "assets/volume_release.pcm")
