#include "standalone_audio.h"
#include <stdatomic.h>
#include "audio_mixer.h"
#include "esp_timer.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "crowpanel_board.h"

/* Mapping verified against the product-explainer's speaker BSP. PA is active low. */
#define CHIME_BCLK GPIO_NUM_22
#define CHIME_WS GPIO_NUM_21
#define CHIME_DOUT GPIO_NUM_23
#define CHIME_PA GPIO_NUM_30
#define FRAMES 256
#define RATE 44100
extern const uint8_t warning_start[] asm("_binary_timer_warning_5s_double_bell_wav_start");
extern const uint8_t warning_end[] asm("_binary_timer_warning_5s_double_bell_wav_end");
extern const uint8_t done_start[] asm("_binary_timer_done_single_bell_wav_start");
extern const uint8_t done_end[] asm("_binary_timer_done_single_bell_wav_end");
extern const uint8_t preview_start[] asm("_binary_volume_release_pcm_start");
extern const uint8_t preview_end[] asm("_binary_volume_release_pcm_end");
#define EVENT_BATCHES 8
#define WARNING_MAX_AGE_MS 150
#define DONE_MAX_AGE_MS 500
static i2s_chan_handle_t s_tx;
static TaskHandle_t s_worker;
static audio_clip_t s_warning, s_done, s_preview;
static atomic_uint s_volume = 40;
static bool s_ready;
/* GUI only copies bounded scalar snapshots under this lock. Worker never takes
 * GUI-lock; clips and all mixer state belong exclusively to worker. */
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
typedef struct { kitchen_audio_batch_t batch; uint64_t queued_ms; } queued_t;
static queued_t s_events[EVENT_BATCHES];
static kitchen_audio_batch_t s_latest;
static unsigned s_count, s_mute_epoch, s_dropped;
static bool s_preview_pending;
static uint64_t s_preview_ms;
static uint64_t clock_ms(void) { return (uint64_t)esp_timer_get_time() / 1000; }

static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)p[1] << 8; }
static uint32_t le32(const uint8_t *p) { return (uint32_t)le16(p) | (uint32_t)le16(p + 2) << 16; }
static esp_err_t parse_wav(const uint8_t *wav_start, const uint8_t *wav_end, audio_clip_t *clip)
{
    size_t length = (size_t)(wav_end - wav_start);
    if (length < 12 || memcmp(wav_start, "RIFF", 4) || memcmp(wav_start + 8, "WAVE", 4)) return ESP_ERR_INVALID_ARG;
    bool format = false;
    for (size_t pos = 12; pos + 8 <= length;) {
        const uint8_t *chunk = wav_start + pos;
        uint32_t bytes = le32(chunk + 4);
        pos += 8;
        if (bytes > length - pos) return ESP_ERR_INVALID_SIZE;
        if (!memcmp(chunk, "fmt ", 4)) {
            if (bytes < 16 || le16(chunk + 8) != 1 || le16(chunk + 10) != 1 ||
                le32(chunk + 12) != RATE || le16(chunk + 20) != 2 || le16(chunk + 22) != 16)
                return ESP_ERR_NOT_SUPPORTED;
            format = true;
        } else if (!memcmp(chunk, "data", 4)) {
            clip->pcm = wav_start + pos; clip->samples = bytes / 2;
            if (bytes & 1) return ESP_ERR_INVALID_SIZE;
        }
        pos += bytes;
        if (bytes & 1) { if (pos == length) return ESP_ERR_INVALID_SIZE; ++pos; }
    }
    clip->rate = RATE;
    return format && clip->pcm && clip->samples ? ESP_OK : ESP_ERR_INVALID_ARG;
}
static esp_err_t write_all(const int16_t *samples, size_t bytes)
{
    const uint8_t *data = (const uint8_t *)samples;
    while (bytes) {
        size_t written = 0;
        esp_err_t ret = i2s_channel_write(s_tx, data, bytes, &written, 100);
        if (ret != ESP_OK) return ret;
        if (!written) return ESP_ERR_TIMEOUT;
        data += written; bytes -= written;
    }
    return ESP_OK;
}
/* Called under s_mux. Remove invalid members, preserving coalesced events for
 * the other timers. A later batch is never merged with an earlier batch. */
static void prune(queued_t *event, uint64_t now)
{
    kitchen_audio_batch_t *b = &event->batch;
    uint64_t age = now >= event->queued_ms ? now - event->queued_ms : 0;
    if (age > WARNING_MAX_AGE_MS) b->warnings = 0;
    if (age > DONE_MAX_AGE_MS) b->done = 0;
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        uint8_t bit = 1U << i;
        if (b->session[i] != s_latest.session[i]) { b->warnings &= ~bit; b->done &= ~bit; }
        if (!(s_latest.finished & bit)) b->done &= ~bit;
        if (!(s_latest.running & bit) || b->deadline_ms[i] != s_latest.deadline_ms[i] ||
            now >= b->deadline_ms[i]) b->warnings &= ~bit;
    }
}
static void compact(uint64_t now)
{
    unsigned count = 0;
    for (unsigned i = 0; i < s_count; ++i) {
        prune(&s_events[i], now);
        if (s_events[i].batch.warnings || s_events[i].batch.done) s_events[count++] = s_events[i];
        else ++s_dropped;
    }
    s_count = count;
}
static void worker(void *context)
{
    (void)context;
    audio_mixer_t mixer = {0};
    int16_t output[FRAMES * 2];
    unsigned mute_epoch = 0, drain = 0;
    bool amplifier = false;
    for (;;) {
        uint64_t now = clock_ms();
        portENTER_CRITICAL(&s_mux);
        if (mute_epoch != s_mute_epoch) {
            memset(&mixer, 0, sizeof(mixer));
            mute_epoch = s_mute_epoch;
        }
        compact(now);
        /* Admission and validity checks are serialized with producer updates:
         * an event either starts here or a preceding cancel excludes it. */
        for (unsigned i = 0; i < s_count; ++i) {
            kitchen_audio_batch_t *b = &s_events[i].batch;
            if (b->done && !audio_mixer_start(&mixer, AUDIO_DONE, s_done)) ++s_dropped;
            if (b->warnings && !audio_mixer_start(&mixer, AUDIO_WARNING, s_warning)) ++s_dropped;
        }
        s_count = 0;
        if (s_preview_pending && (now < s_preview_ms || now - s_preview_ms <= WARNING_MAX_AGE_MS) &&
            atomic_load(&s_volume)) {
            /* Preview is discarded if any timer voice is active. */
            (void)audio_mixer_start(&mixer, AUDIO_PREVIEW, s_preview);
        }
        s_preview_pending = false;
        unsigned dropped = s_dropped;
        s_dropped = 0;
        portEXIT_CRITICAL(&s_mux);
        if (dropped) ESP_LOGW("chime", "discarded %u stale/overflow/voice-limit audio batches", dropped);

        bool active = audio_mixer_active(&mixer);
        if (!active && !amplifier) {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }
        esp_err_t ret = ESP_OK;
        if (active) {
            drain = 8; /* Greater than six DMA descriptors; check new events each block. */
            if (!amplifier) ret = gpio_set_level(CHIME_PA, 0);
            amplifier = true;
        }
        audio_mixer_render(&mixer, output, FRAMES, atomic_load(&s_volume));
        if (ret == ESP_OK) ret = write_all(output, sizeof(output));
        if (!active && drain) --drain;
        if (ret != ESP_OK || (!active && !drain)) {
            esp_err_t mute = gpio_set_level(CHIME_PA, 1);
            amplifier = false;
            if (ret != ESP_OK || mute != ESP_OK) {
                ESP_LOGW("chime", "playback=%s mute=%s", esp_err_to_name(ret), esp_err_to_name(mute));
                memset(&mixer, 0, sizeof(mixer));
                portENTER_CRITICAL(&s_mux);
                s_count = 0; s_preview_pending = false;
                portEXIT_CRITICAL(&s_mux);
            }
        }
    }
}
esp_err_t standalone_audio_init(void)
{
    if (s_ready) return ESP_OK;
    size_t preview_bytes = (size_t)(preview_end - preview_start);
    if (!preview_bytes || (preview_bytes & 1)) return ESP_ERR_INVALID_SIZE;
    esp_err_t ret = parse_wav(warning_start, warning_end, &s_warning);
    if (ret != ESP_OK) return ret;
    ret = parse_wav(done_start, done_end, &s_done);
    if (ret != ESP_OK) return ret;
    s_preview = (audio_clip_t){.pcm = preview_start, .samples = preview_bytes / 2, .rate = 16000};
    const gpio_num_t used[] = {CROWPANEL_I2C_SDA, CROWPANEL_I2C_SCL, CROWPANEL_TOUCH_RESET,
        CROWPANEL_TOUCH_INT, CROWPANEL_BACKLIGHT, crowpanel_board_get_config()->lcd_reset};
    const gpio_num_t audio[] = {CHIME_BCLK, CHIME_WS, CHIME_DOUT, CHIME_PA};
    for (unsigned i = 0; i < sizeof(audio) / sizeof(audio[0]); ++i)
        for (unsigned j = 0; j < sizeof(used) / sizeof(used[0]); ++j)
            if (audio[i] == used[j]) return ESP_ERR_INVALID_ARG;
    const gpio_config_t pa = {.pin_bit_mask = 1ULL << CHIME_PA, .mode = GPIO_MODE_OUTPUT};
    /* Preload mute before enabling the active-low amplifier output. */
    ret = gpio_set_level(CHIME_PA, 1);
    if (ret != ESP_OK) return ret;
    ret = gpio_config(&pa);
    if (ret != ESP_OK) return ret;
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    channel.dma_desc_num = 6; channel.dma_frame_num = FRAMES;
    channel.auto_clear_after_cb = true;
    ret = i2s_new_channel(&channel, &s_tx, NULL);
    bool enabled = false;
    if (ret != ESP_OK) goto fail;
    const i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = CHIME_BCLK, .ws = CHIME_WS,
                     .dout = CHIME_DOUT, .din = I2S_GPIO_UNUSED},
    };
    ret = i2s_channel_init_std_mode(s_tx, &config);
    if (ret != ESP_OK) goto fail;
    ret = i2s_channel_enable(s_tx);
    if (ret != ESP_OK) goto fail;
    enabled = true;
    if (xTaskCreate(worker, "kitchen_chime", 4096, NULL, 4, &s_worker) != pdPASS) {
        ret = ESP_ERR_NO_MEM; goto fail;
    }
    s_ready = true;
    ESP_LOGI("chime", "11 voices, PCM 44100 Hz; I2S stereo 22/21/23, PA30 active low");
    return ESP_OK;
fail:
    if (s_tx) {
        if (enabled) (void)i2s_channel_disable(s_tx);
        (void)i2s_del_channel(s_tx); s_tx = NULL;
    }
    return ret;
}
esp_err_t standalone_audio_submit(void *context, const kitchen_audio_batch_t *batch)
{
    (void)context;
    if (!s_ready || !batch) return ESP_ERR_INVALID_STATE;
    uint64_t now = clock_ms();
    esp_err_t ret = ESP_OK;
    portENTER_CRITICAL(&s_mux);
    s_latest = *batch;
    compact(now); /* Invalidate immediately, including before a new app instance. */
    if (atomic_load(&s_volume) && (batch->warnings || batch->done)) {
        if (s_count == EVENT_BATCHES && batch->done) {
            /* Completion may evict a warning-only batch, never the reverse. */
            for (unsigned i = 0; i < s_count; ++i) {
                if (!s_events[i].batch.done) {
                    memmove(&s_events[i], &s_events[i + 1], (s_count - i - 1) * sizeof(s_events[0]));
                    --s_count; ++s_dropped; break;
                }
            }
        }
        if (s_count < EVENT_BATCHES) s_events[s_count++] = (queued_t){.batch = *batch, .queued_ms = now};
        else { ++s_dropped; ret = ESP_ERR_TIMEOUT; }
    }
    portEXIT_CRITICAL(&s_mux);
    xTaskNotifyGive(s_worker);
    return ret;
}
esp_err_t standalone_audio_volume(void *context, uint8_t percent)
{
    (void)context;
    if (!s_ready || percent > 100) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_mux);
    atomic_store(&s_volume, percent);
    if (!percent) {
        ++s_mute_epoch;
        s_count = 0; s_preview_pending = false;
    }
    portEXIT_CRITICAL(&s_mux);
    xTaskNotifyGive(s_worker);
    return ESP_OK;
}
esp_err_t standalone_audio_preview(void *context)
{
    (void)context;
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    uint64_t now = clock_ms();
    portENTER_CRITICAL(&s_mux);
    s_preview_pending = atomic_load(&s_volume) != 0;
    s_preview_ms = now;
    portEXIT_CRITICAL(&s_mux);
    xTaskNotifyGive(s_worker);
    return ESP_OK;
}
