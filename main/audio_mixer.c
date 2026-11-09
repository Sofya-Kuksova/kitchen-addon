#include "audio_mixer.h"
#include <limits.h>

static int32_t sample_at(const audio_clip_t *clip, size_t position)
{
    uint64_t phase = (uint64_t)position * clip->rate;
    size_t index = phase / AUDIO_MIXER_RATE;
    const uint8_t *p = clip->pcm + index * 2;
    int32_t sample = (int16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
    if (clip->rate != AUDIO_MIXER_RATE && index + 1 < clip->samples) {
        int32_t next = (int16_t)((uint16_t)p[2] | (uint16_t)p[3] << 8);
        sample += (int32_t)((int64_t)(next - sample) * (int64_t)(phase % AUDIO_MIXER_RATE) / AUDIO_MIXER_RATE);
    }
    return sample;
}
bool audio_mixer_active(const audio_mixer_t *mixer)
{
    for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i)
        if (mixer->voices[i].active) return true;
    return false;
}
bool audio_mixer_start(audio_mixer_t *mixer, audio_kind_t kind, audio_clip_t clip)
{
    if (!clip.pcm || !clip.samples || !clip.rate || clip.rate > AUDIO_MIXER_RATE) return false;
    if (kind == AUDIO_PREVIEW) {
        for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i)
            if (mixer->voices[i].active && mixer->voices[i].kind != AUDIO_PREVIEW) return false;
    }
    if (!audio_mixer_active(mixer)) mixer->limiter_gain = 32768;
    int slot = -1, victim = -1;
    for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i) {
        audio_voice_t *voice = &mixer->voices[i];
        if (!voice->active) { if (slot < 0) slot = (int)i; continue; }
        if (voice->kind == AUDIO_PREVIEW) {
            if (kind == AUDIO_PREVIEW) { slot = (int)i; break; }
            if (!voice->fade) voice->fade = AUDIO_MIXER_FADE;
        }
        if (voice->kind < kind && (victim < 0 || voice->kind < mixer->voices[victim].kind)) victim = (int)i;
    }
    if (slot < 0) slot = victim;
    if (slot < 0) return false;
    mixer->voices[slot] = (audio_voice_t){.clip = clip, .kind = kind, .active = true};
    return true;
}
void audio_mixer_render(audio_mixer_t *mixer, int16_t *output, size_t frames, unsigned volume)
{
    if (volume > 100) volume = 100;
    if (!mixer->limiter_gain) mixer->limiter_gain = 32768;
    /* Fixed lookahead scratch: common gain preserves the summed waveform,
     * avoiding flat-topped clipping when several independent bells overlap. */
    int32_t mixed[64];
    for (size_t offset = 0; offset < frames;) {
        size_t count = frames - offset;
        if (count > 64) count = 64;
        uint32_t peak = 0;
        for (size_t frame = 0; frame < count; ++frame) {
            int32_t sum = 0; /* <= 11 * 32768; scaling uses int64_t. */
            for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i) {
                audio_voice_t *voice = &mixer->voices[i];
                if (!voice->active) continue;
                int32_t sample = sample_at(&voice->clip, voice->position);
                if (voice->position < AUDIO_MIXER_FADE)
                    sample = sample * (int32_t)voice->position / (int32_t)AUDIO_MIXER_FADE;
                if (voice->kind == AUDIO_PREVIEW) sample = sample * 4 / 5;
                if (voice->fade) sample = sample * (int32_t)voice->fade / (int32_t)AUDIO_MIXER_FADE;
                sum += sample;
                ++voice->position;
                if ((uint64_t)voice->position * voice->clip.rate >=
                    (uint64_t)voice->clip.samples * AUDIO_MIXER_RATE || (voice->fade && !--voice->fade))
                    voice->active = false;
            }
            mixed[frame] = (int32_t)((int64_t)sum * volume / 200);
            uint32_t magnitude = mixed[frame] < 0 ? (uint32_t)-mixed[frame] : (uint32_t)mixed[frame];
            if (magnitude > peak) peak = magnitude;
        }
        uint32_t target = peak > INT16_MAX ? (uint32_t)((uint64_t)INT16_MAX * 32768 / peak) : 32768;
        if (target < mixer->limiter_gain) mixer->limiter_gain = target; /* Immediate attack. */
        else {
            uint32_t release = (32768 - mixer->limiter_gain) * (uint32_t)count / 2205 + 1;
            uint32_t delta = target - mixer->limiter_gain;
            mixer->limiter_gain += delta < release ? delta : release; /* ~50 ms release. */
        }
        for (size_t frame = 0; frame < count; ++frame) {
            int32_t value = (int32_t)((int64_t)mixed[frame] * mixer->limiter_gain / 32768);
            if (value > INT16_MAX) value = INT16_MAX;
            if (value < INT16_MIN) value = INT16_MIN;
            output[(offset + frame) * 2] = output[(offset + frame) * 2 + 1] = (int16_t)value;
        }
        offset += count;
    }
}
