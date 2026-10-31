#include "standalone_audio.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "crowpanel_board.h"

/* Mapping verified against the product-explainer's speaker BSP. PA is active low. */
#define CHIME_BCLK GPIO_NUM_22
#define CHIME_WS GPIO_NUM_21
#define CHIME_DOUT GPIO_NUM_23
#define CHIME_PA GPIO_NUM_30
#define FRAMES 256
#define RATE 44100
extern const uint8_t wav_start[] asm("_binary_kitchen_timer_chime_wav_start");
extern const uint8_t wav_end[] asm("_binary_kitchen_timer_chime_wav_end");
extern const uint8_t preview_start[] asm("_binary_volume_release_pcm_start");
extern const uint8_t preview_end[] asm("_binary_volume_release_pcm_end");
static i2s_chan_handle_t s_tx;
static QueueHandle_t s_events;
static const uint8_t *s_pcm;
static size_t s_pcm_bytes;
static atomic_uint s_volume = 40;
static bool s_ready;

static uint16_t le16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)p[1] << 8; }
static uint32_t le32(const uint8_t *p) { return (uint32_t)le16(p) | (uint32_t)le16(p + 2) << 16; }
static esp_err_t parse_wav(void)
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
            s_pcm = wav_start + pos; s_pcm_bytes = bytes;
        }
        pos += bytes;
        if (bytes & 1) { if (pos == length) return ESP_ERR_INVALID_SIZE; ++pos; }
    }
    return format && s_pcm && s_pcm_bytes && !(s_pcm_bytes & 1) ? ESP_OK : ESP_ERR_INVALID_ARG;
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
static void worker(void *context)
{
    (void)context;
    int16_t output[FRAMES * 2];
    uint8_t token;
    for (;;) {
        if (xQueueReceive(s_events, &token, portMAX_DELAY) != pdTRUE) continue;
        bool preview = token == 2;
        if (!atomic_load(&s_volume)) {
            continue;
        }
        const uint8_t *pcm = preview ? preview_start : s_pcm;
        size_t samples = preview ? (size_t)(preview_end - preview_start) / 2 : s_pcm_bytes / 2;
        unsigned rate = preview ? 16000 : RATE;
        size_t total = (samples * (uint64_t)RATE + rate - 1) / rate;
        esp_err_t ret = gpio_set_level(CHIME_PA, 0);
        for (size_t pos = 0; ret == ESP_OK && pos < total;) {
            if (preview) {
                uint8_t next;
                if (xQueuePeek(s_events, &next, 0) == pdTRUE) {
                    if (next == 1) break; /* Completion has priority over preview. */
                    if (xQueueReceive(s_events, &next, 0) == pdTRUE) pos = 0;
                }
            }
            unsigned volume = atomic_load(&s_volume);
            size_t frames = total - pos;
            if (frames > FRAMES) frames = FRAMES;
            for (size_t i = 0; i < frames; ++i) {
                uint64_t phase = (uint64_t)(pos + i) * rate;
                size_t index = phase / RATE;
                unsigned fraction = phase % RATE;
                int32_t sample = (int16_t)le16(pcm + index * 2);
                if (preview && index + 1 < samples) {
                    int32_t next = (int16_t)le16(pcm + (index + 1) * 2);
                    sample += (int32_t)((int64_t)(next - sample) * fraction / RATE);
                }
                /* Reference preview gain=0.40; timer chime keeps unity gain. */
                int16_t value = (int16_t)(sample * (int32_t)volume / (preview ? 250 : 100));
                output[i * 2] = output[i * 2 + 1] = value;
            }
            ret = write_all(output, frames * 2 * sizeof(*output));
            pos += frames;
        }
        /* More silence than the six DMA buffers drains the last audible samples. */
        memset(output, 0, sizeof(output));
        for (unsigned i = 0; ret == ESP_OK && i < 8; ++i) ret = write_all(output, sizeof(output));
        esp_err_t mute = gpio_set_level(CHIME_PA, 1);
        if (ret != ESP_OK || mute != ESP_OK)
            ESP_LOGW("chime", "playback=%s mute=%s", esp_err_to_name(ret), esp_err_to_name(mute));
    }
}
esp_err_t standalone_audio_init(void)
{
    if (s_ready) return ESP_OK;
    size_t preview_bytes = (size_t)(preview_end - preview_start);
    if (!preview_bytes || (preview_bytes & 1)) return ESP_ERR_INVALID_SIZE;
    esp_err_t ret = parse_wav();
    if (ret != ESP_OK) return ret;
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
    s_events = xQueueCreate(20, sizeof(uint8_t));
    if (!s_events) return ESP_ERR_NO_MEM;
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
    if (xTaskCreate(worker, "kitchen_chime", 4096, NULL, 4, NULL) != pdPASS) {
        ret = ESP_ERR_NO_MEM; goto fail;
    }
    s_ready = true;
    ESP_LOGI("chime", "PCM mono 44100 Hz, %u samples; I2S stereo 22/21/23, PA30 active low", (unsigned)(s_pcm_bytes / 2));
    return ESP_OK;
fail:
    if (s_tx) {
        if (enabled) (void)i2s_channel_disable(s_tx);
        (void)i2s_del_channel(s_tx); s_tx = NULL;
    }
    vQueueDelete(s_events); s_events = NULL;
    return ret;
}
esp_err_t standalone_audio_chime(void *context)
{
    (void)context;
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    const uint8_t token = 1;
    return xQueueSend(s_events, &token, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}
esp_err_t standalone_audio_volume(void *context, uint8_t percent)
{
    (void)context;
    if (!s_ready || percent > 100) return ESP_ERR_INVALID_ARG;
    atomic_store(&s_volume, percent);
    return ESP_OK;
}

esp_err_t standalone_audio_preview(void *context)
{
    (void)context;
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    const uint8_t token = 2;
    if (xQueueSend(s_events, &token, 0) == pdTRUE) return ESP_OK;
    return ESP_ERR_TIMEOUT;
}
