#pragma once
#include <stdint.h>
#include "esp_err.h"

/* Standalone-only I2S/PA owner, retained until reset. GUI callbacks never wait. */
esp_err_t standalone_audio_init(void);
esp_err_t standalone_audio_chime(void *context);
esp_err_t standalone_audio_volume(void *context, uint8_t percent);

esp_err_t standalone_audio_preview(void *context);
