idf_component_register(SRCS "kitchen_app.c" INCLUDE_DIRS "include"
    REQUIRES app_manager kitchen_platform
    PRIV_REQUIRES kitchen_core kitchen_ui)
