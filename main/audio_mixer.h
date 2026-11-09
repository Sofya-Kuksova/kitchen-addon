#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Portable, single-owner PCM engine; no SDK/RTOS/UI dependencies. */
#define AUDIO_MIXER_VOICES 11 /* two signals per timer plus volume preview */
#define AUDIO_MIXER_RATE 44100U
#define AUDIO_MIXER_FADE 128U

typedef enum { AUDIO_PREVIEW, AUDIO_WARNING, AUDIO_DONE } audio_kind_t;
typedef struct {
    const uint8_t *pcm;
    size_t samples;
    unsigned rate;
} audio_clip_t;
typedef struct {
    audio_clip_t clip;
    size_t position; /* Output frames, not source samples. */
    audio_kind_t kind;
    unsigned fade;
    bool active;
} audio_voice_t;
typedef struct {
    audio_voice_t voices[AUDIO_MIXER_VOICES];
    uint32_t limiter_gain; /* Q15, zero means uninitialized. */
} audio_mixer_t;

bool audio_mixer_start(audio_mixer_t *, audio_kind_t, audio_clip_t);
bool audio_mixer_active(const audio_mixer_t *);
/* Interleaved stereo; always writes frames, zero-filling the end. Fixed -6 dB
 * timer headroom, original preview gain 0.40, master gain, common peak limiter and PCM16 saturation guard. */
void audio_mixer_render(audio_mixer_t *, int16_t *output, size_t frames, unsigned volume);
