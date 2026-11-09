/* Owner-run plain C scenarios, never registered in the firmware build. */
#include "audio_mixer.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static uint8_t positive[1024], negative[1024];
static int16_t output[1024];
static unsigned voices(const audio_mixer_t *mixer, audio_kind_t kind)
{
    unsigned count = 0;
    for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i)
        if (mixer->voices[i].active && mixer->voices[i].kind == kind) ++count;
    return count;
}
int main(void)
{
    for (unsigned i = 0; i < 512; ++i) {
        positive[i * 2] = 0xff; positive[i * 2 + 1] = 0x7f;
        negative[i * 2] = 0; negative[i * 2 + 1] = 0x80;
    }
    audio_clip_t plus = {.pcm = positive, .samples = 512, .rate = 44100};
    audio_clip_t minus = {.pcm = negative, .samples = 512, .rate = 44100};
    audio_mixer_t mixer = {0};
    assert(audio_mixer_start(&mixer, AUDIO_WARNING, plus));
    audio_mixer_render(&mixer, output, 128, 100);
    assert(output[0] == 0 && output[254] > 0); /* onset ramp */
    assert(audio_mixer_start(&mixer, AUDIO_DONE, minus));
    audio_mixer_render(&mixer, output, 256, 100);
    assert(mixer.voices[0].position == 384 && mixer.voices[1].position == 256);
    assert(output[510] == 0 && output[511] == 0); /* +32767 and -32768 cancel (round to zero) */
    assert(!audio_mixer_start(&mixer, AUDIO_PREVIEW, plus));
    audio_mixer_render(&mixer, output, 512, 100);
    assert(!audio_mixer_active(&mixer));
    for (unsigned i = 512; i < 1024; ++i) assert(output[i] == 0); /* zero-filled tail */

    memset(&mixer, 0, sizeof(mixer));
    for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i) assert(audio_mixer_start(&mixer, AUDIO_DONE, plus));
    assert(!audio_mixer_start(&mixer, AUDIO_WARNING, plus));
    assert(!audio_mixer_start(&mixer, AUDIO_DONE, plus));
    audio_mixer_render(&mixer, output, 256, 100);
    assert(output[510] > 32000 && output[511] == output[510]);
    assert(mixer.limiter_gain < 32768);
    uint32_t reduced_gain = mixer.limiter_gain;
    audio_mixer_render(&mixer, output, 128, 0);
    for (unsigned i = 0; i < 256; ++i) assert(output[i] == 0);
    assert(mixer.limiter_gain > reduced_gain && mixer.limiter_gain <= 32768);
    memset(&mixer, 0, sizeof(mixer));
    for (unsigned i = 0; i < AUDIO_MIXER_VOICES; ++i) assert(audio_mixer_start(&mixer, AUDIO_WARNING, minus));
    assert(audio_mixer_start(&mixer, AUDIO_DONE, plus));
    assert(voices(&mixer, AUDIO_DONE) == 1 && voices(&mixer, AUDIO_WARNING) == AUDIO_MIXER_VOICES - 1);
    audio_mixer_render(&mixer, output, 256, 100);
    assert(output[510] < -32000 && output[511] == output[510]);
    assert(mixer.limiter_gain < 32768);

    memset(&mixer, 0, sizeof(mixer));
    assert(audio_mixer_start(&mixer, AUDIO_PREVIEW, plus));
    for (unsigned i = 1; i < AUDIO_MIXER_VOICES; ++i) assert(audio_mixer_start(&mixer, AUDIO_WARNING, plus));
    assert(audio_mixer_start(&mixer, AUDIO_DONE, minus));
    assert(voices(&mixer, AUDIO_PREVIEW) == 0 && voices(&mixer, AUDIO_DONE) == 1);
    assert(voices(&mixer, AUDIO_WARNING) == AUDIO_MIXER_VOICES - 1);

    memset(&mixer, 0, sizeof(mixer));
    plus.rate = 16000;
    assert(audio_mixer_start(&mixer, AUDIO_PREVIEW, plus));
    audio_mixer_render(&mixer, output, 256, 100);
    assert(output[510] == 13106); /* preserved 0.40 preview gain */
    assert(audio_mixer_start(&mixer, AUDIO_WARNING, minus));
    audio_mixer_render(&mixer, output, 128, 100);
    assert(voices(&mixer, AUDIO_PREVIEW) == 0 && voices(&mixer, AUDIO_WARNING) == 1);
    while (audio_mixer_active(&mixer)) audio_mixer_render(&mixer, output, 256, 50);
    /* Unequal-rate preview ends at ceil(512 * 44100 / 16000) frames. */
    memset(&mixer, 0, sizeof(mixer));
    assert(audio_mixer_start(&mixer, AUDIO_PREVIEW, plus));
    audio_mixer_render(&mixer, output, 512, 100);
    audio_mixer_render(&mixer, output, 512, 100);
    audio_mixer_render(&mixer, output, 387, 100);
    assert(audio_mixer_active(&mixer));
    audio_mixer_render(&mixer, output, 1, 100);
    assert(!audio_mixer_active(&mixer));
    puts("Kitchen audio mixer acceptance: PASS");
    return 0;
}
