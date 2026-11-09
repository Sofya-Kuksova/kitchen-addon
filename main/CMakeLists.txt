idf_component_register(SRCS "app_main.c" "standalone_runner.c" "startup_screen.c" "standalone_audio.c" "audio_mixer.c" "standalone_storage.c" INCLUDE_DIRS "."
    PRIV_REQUIRES crowpanel_board display_port kitchen_app kitchen_ui kitchen_core esp_timer esp_system esp_driver_i2s esp_driver_gpio nvs_flash
    EMBED_FILES "assets/timer_warning_5s_double_bell.wav" "assets/timer_done_single_bell.wav" "assets/volume_release.pcm")
