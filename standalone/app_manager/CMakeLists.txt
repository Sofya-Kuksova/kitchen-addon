# Header-only ABI fixture, never supplied to the real host.
# IDF scans requirements in a separate script without project cache variables.
# Validate the supplied path/hash only during normal component registration.
if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register(REQUIRES lvgl__lvgl esp_common)
    return()
endif()

if(NOT KITCHEN_HOST_CONTRACT_DIR OR NOT EXISTS "${KITCHEN_HOST_CONTRACT_DIR}/app_base.h")
    message(FATAL_ERROR "Supply -D KITCHEN_HOST_CONTRACT_DIR=<prepared-contract-dir>; see docs/BUILDING.md")
endif()
file(SHA256 "${KITCHEN_HOST_CONTRACT_DIR}/app_base.h" KITCHEN_ACTUAL_CONTRACT_HASH)
if(NOT KITCHEN_ACTUAL_CONTRACT_HASH STREQUAL "b105002ee0833cb2ed10314ee12cc5ace698a410a5638d86b30b9c81e5c5b75f")
    message(FATAL_ERROR "app_base.h differs from reviewed host snapshot")
endif()
idf_component_register(INCLUDE_DIRS "${KITCHEN_HOST_CONTRACT_DIR}" REQUIRES lvgl__lvgl esp_common)
