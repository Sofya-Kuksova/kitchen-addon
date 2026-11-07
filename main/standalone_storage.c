#include "standalone_storage.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"

typedef struct {
    uint8_t data[KITCHEN_RECORD_MAX];
    size_t length;
    uint64_t issued, pending, completed;
    kitchen_storage_result_t result;
    bool protected_record;
} storage_entry_t;
static storage_entry_t s_entries[KITCHEN_SETTINGS_KEYS];
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t s_io;
static TaskHandle_t s_worker;
static nvs_handle_t s_handle;
static bool s_open, s_ready;
static uint64_t s_ticket;
static const char *TAG = "kitchen_storage";
static const char *keys[] = {"volume", "slot1", "slot2", "slot3", "slot4", "slot5"};

/* Caller owns s_io. Never erase NVS to recover an initialization failure. */
static esp_err_t open_storage(void)
{
    if (s_open) return ESP_OK;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_OK) ret = nvs_open("kitchen", NVS_READWRITE, &s_handle);
    if (ret == ESP_OK) s_open = true;
    else ESP_LOGW(TAG, "NVS unavailable (no erase): %s", esp_err_to_name(ret));
    return ret;
}
static void worker(void *argument)
{
    (void)argument;
    /* Explicit five-slot migration requested by the owner. Only this retired
     * key is removed, including any old schema; other records stay intact.
     * Keep erase/commit on the sole writer, before load is allowed to proceed. */
    xSemaphoreTake(s_io, portMAX_DELAY);
    esp_err_t cleanup = open_storage();
    if (cleanup == ESP_OK) {
        cleanup = nvs_erase_key(s_handle, "slot6");
        if (cleanup == ESP_OK) cleanup = nvs_commit(s_handle);
        else if (cleanup == ESP_ERR_NVS_NOT_FOUND) cleanup = ESP_OK;
    }
    if (cleanup != ESP_OK)
        ESP_LOGW(TAG, "retired slot6 cleanup failed (retry next boot): %s", esp_err_to_name(cleanup));
    xSemaphoreGive(s_io);
    portENTER_CRITICAL(&s_mux);
    s_ready = true;
    portEXIT_CRITICAL(&s_mux);
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        bool work;
        do {
            work = false;
            for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key) {
                uint8_t data[KITCHEN_RECORD_MAX];
                size_t length = 0;
                uint64_t ticket;
                portENTER_CRITICAL(&s_mux);
                storage_entry_t *entry = &s_entries[key];
                ticket = entry->pending;
                if (ticket) {
                    length = entry->length;
                    memcpy(data, entry->data, length);
                    entry->pending = 0;
                }
                portEXIT_CRITICAL(&s_mux);
                if (!ticket) continue;
                work = true;
                xSemaphoreTake(s_io, portMAX_DELAY);
                esp_err_t ret = open_storage();
                if (ret == ESP_OK) ret = nvs_set_blob(s_handle, keys[key], data, length);
                if (ret == ESP_OK) ret = nvs_commit(s_handle);
                xSemaphoreGive(s_io);
                if (ret != ESP_OK) ESP_LOGW(TAG, "%s ticket %llu: %s", keys[key],
                    (unsigned long long)ticket, esp_err_to_name(ret));
                portENTER_CRITICAL(&s_mux);
                entry->completed = ticket;
                entry->result = ret == ESP_OK ? KITCHEN_STORAGE_OK : KITCHEN_STORAGE_ERROR;
                portEXIT_CRITICAL(&s_mux);
            }
        } while (work);
    }
}
static kitchen_storage_result_t load(void *context, unsigned key, uint8_t *data, size_t *length)
{
    (void)context;
    if (!s_worker || key >= KITCHEN_SETTINGS_KEYS || !data || !length) return KITCHEN_STORAGE_ERROR;
    /* A recreated app must not load behind already accepted writes. This wait
     * is permitted only in create, outside GUI-lock. No app lifetime is retained. */
    uint64_t barrier[KITCHEN_SETTINGS_KEYS];
    portENTER_CRITICAL(&s_mux);
    for (unsigned i = 0; i < KITCHEN_SETTINGS_KEYS; ++i) barrier[i] = s_entries[i].issued;
    portEXIT_CRITICAL(&s_mux);
    for (;;) {
        portENTER_CRITICAL(&s_mux);
        bool ready = s_ready;
        for (unsigned i = 0; i < KITCHEN_SETTINGS_KEYS; ++i)
            if (s_entries[i].completed < barrier[i]) ready = false;
        portEXIT_CRITICAL(&s_mux);
        if (ready) break;
        vTaskDelay(1);
    }
    xSemaphoreTake(s_io, portMAX_DELAY);
    esp_err_t ret = open_storage();
    size_t needed = 0;
    if (ret == ESP_OK) ret = nvs_get_blob(s_handle, keys[key], NULL, &needed);
    kitchen_storage_result_t result = KITCHEN_STORAGE_ERROR;
    if (ret == ESP_ERR_NVS_NOT_FOUND) result = KITCHEN_STORAGE_MISSING;
    else if (ret == ESP_OK && needed > *length) result = KITCHEN_STORAGE_PROTECTED;
    else if (ret == ESP_OK && needed) {
        ret = nvs_get_blob(s_handle, keys[key], data, &needed);
        if (ret == ESP_OK) {
            *length = needed;
            result = data[0] > 1 ? KITCHEN_STORAGE_PROTECTED : KITCHEN_STORAGE_OK;
        }
    }
    xSemaphoreGive(s_io);
    if (result == KITCHEN_STORAGE_PROTECTED) {
        portENTER_CRITICAL(&s_mux);
        s_entries[key].protected_record = true;
        portEXIT_CRITICAL(&s_mux);
        ESP_LOGW(TAG, "%s has newer/oversized schema; preserving record", keys[key]);
    } else if (result == KITCHEN_STORAGE_ERROR) ESP_LOGW(TAG, "%s read failed: %s", keys[key],
        ret == ESP_OK ? "zero-length record" : esp_err_to_name(ret));
    return result;
}
static kitchen_storage_result_t submit(void *context, unsigned key, const uint8_t *data, size_t length, uint64_t *ticket)
{
    (void)context;
    if (!s_worker || key >= KITCHEN_SETTINGS_KEYS || !data || !length || length > KITCHEN_RECORD_MAX || !ticket)
        return KITCHEN_STORAGE_ERROR;
    kitchen_storage_result_t result = KITCHEN_STORAGE_PENDING;
    portENTER_CRITICAL(&s_mux);
    storage_entry_t *entry = &s_entries[key];
    if (entry->protected_record) result = KITCHEN_STORAGE_PROTECTED;
    else if (s_ticket == UINT64_MAX) result = KITCHEN_STORAGE_ERROR;
    else {
        entry->length = length;
        memcpy(entry->data, data, length);
        entry->issued = entry->pending = *ticket = ++s_ticket;
    }
    portEXIT_CRITICAL(&s_mux);
    if (result == KITCHEN_STORAGE_PENDING) xTaskNotifyGive(s_worker);
    return result;
}
static kitchen_storage_result_t poll(void *context, unsigned key, uint64_t ticket)
{
    (void)context;
    if (key >= KITCHEN_SETTINGS_KEYS || !ticket) return KITCHEN_STORAGE_ERROR;
    portENTER_CRITICAL(&s_mux);
    storage_entry_t *entry = &s_entries[key];
    kitchen_storage_result_t result = entry->issued != ticket ? KITCHEN_STORAGE_ERROR :
        entry->completed == ticket ? entry->result : KITCHEN_STORAGE_PENDING;
    portEXIT_CRITICAL(&s_mux);
    return result;
}
kitchen_storage_backend_t standalone_storage_init(void)
{
    if (!s_io) s_io = xSemaphoreCreateMutex();
    if (s_io && !s_worker && xTaskCreate(worker, "kitchen_store", 3072, NULL, 2, &s_worker) != pdPASS) {
        s_worker = NULL;
        ESP_LOGE(TAG, "cannot create storage worker");
    }
    return (kitchen_storage_backend_t){.load = load, .submit = submit, .poll = poll};
}
