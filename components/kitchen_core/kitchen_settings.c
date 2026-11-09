#include "kitchen_settings.h"
#include <string.h>

size_t kitchen_settings_encode(const kitchen_state_t *state, unsigned key, uint8_t *out)
{
    if (!state || !out || key >= KITCHEN_SETTINGS_KEYS) return 0;
    out[0] = 1;
    if (!key) { out[1] = state->volume_percent; return 2; }
    const kitchen_timer_t *timer = &state->timers[key - 1];
    size_t n = 0;
    while (n <= KITCHEN_NAME_MAX && timer->name[n]) ++n;
    if (n > KITCHEN_NAME_MAX) return 0;
    for (unsigned i = 0; i < 4; ++i) out[1 + i] = (uint8_t)(timer->duration_s >> (8 * i));
    out[5] = (uint8_t)timer->icon;
    out[6] = (uint8_t)n;
    memcpy(out + 7, timer->name, n);
    return 7 + n;
}
static const char *record_error(unsigned key, const uint8_t *data, size_t length)
{
    if (!data || key >= KITCHEN_SETTINGS_KEYS || !length) return "empty record or invalid key";
    if (length > KITCHEN_RECORD_MAX) return "oversized record";
    if (data[0] > 1) return "newer schema";
    if (data[0] != 1) return "invalid schema version";
    if (!key) {
        if (length != 2) return "invalid volume length";
        return data[1] > 100 ? "volume outside 0..100" : NULL;
    }
    if (length < 7 || data[6] > KITCHEN_NAME_MAX || length != (size_t)7 + data[6]) return "invalid slot/name length";
    if (data[5] >= KITCHEN_ICON_COUNT) return "invalid icon ID";
    uint32_t seconds = 0;
    for (unsigned i = 0; i < 4; ++i) seconds |= (uint32_t)data[1 + i] << (8 * i);
    if (seconds > KITCHEN_MAX_SECONDS) return "duration exceeds 86399 seconds";
    if (!seconds && (data[5] || data[6])) return "nonempty metadata in EMPTY slot";
    for (unsigned i = 0; i < data[6]; ++i)
        if (data[7 + i] < 32 || data[7 + i] > 126) return "name contains nonprintable/non-ASCII bytes";
    return NULL;
}
kitchen_storage_result_t kitchen_settings_decode(kitchen_state_t *state, unsigned key, const uint8_t *data, size_t length)
{
    if (!state) return KITCHEN_STORAGE_ERROR;
    if (record_error(key, data, length))
        return data && length && (length > KITCHEN_RECORD_MAX || data[0] > 1) ? KITCHEN_STORAGE_PROTECTED : KITCHEN_STORAGE_ERROR;
    if (!key) {
        state->volume_percent = data[1];
        return KITCHEN_STORAGE_OK;
    }
    uint32_t seconds = 0;
    for (unsigned i = 0; i < 4; ++i) seconds |= (uint32_t)data[1 + i] << (8 * i);
    kitchen_timer_t *timer = &state->timers[key - 1];
    timer->duration_s = seconds;
    timer->icon = (kitchen_icon_t)data[5];
    memcpy(timer->name, data + 7, data[6]);
    timer->name[data[6]] = 0;
    timer->status = seconds ? KITCHEN_READY : KITCHEN_EMPTY;
    timer->remaining_ms = timer->deadline_ms = timer->span_ms = 0;
    timer->session = timer->time_revision = 0;
    timer->warning_armed = false;
    return KITCHEN_STORAGE_OK;
}
static bool equal(const uint8_t *a, size_t na, const uint8_t *b, size_t nb)
{
    return na == nb && !memcmp(a, b, na);
}
void kitchen_settings_init(kitchen_settings_t *settings, kitchen_state_t *state, const kitchen_storage_backend_t *backend)
{
    memset(settings, 0, sizeof(*settings));
    if (backend) settings->backend = *backend;
    settings->available = backend && backend->load && backend->submit && backend->poll;
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key) {
        kitchen_settings_entry_t *entry = &settings->entries[key];
        uint8_t data[KITCHEN_RECORD_MAX];
        size_t length = sizeof(data);
        entry->loaded = settings->available ? backend->load(backend->context, key, data, &length) : KITCHEN_STORAGE_MISSING;
        if (entry->loaded == KITCHEN_STORAGE_OK) {
            entry->load_reason = record_error(key, data, length);
            entry->loaded = kitchen_settings_decode(state, key, data, length);
        } else if (entry->loaded == KITCHEN_STORAGE_PROTECTED) entry->load_reason = "newer or oversized record";
        else if (entry->loaded == KITCHEN_STORAGE_ERROR) entry->load_reason = "backend read unavailable";
        entry->protected_record = entry->loaded == KITCHEN_STORAGE_PROTECTED;
        /* A corrupt/missing record starts safely, without an automatic repair write. */
        entry->failed = entry->loaded == KITCHEN_STORAGE_ERROR || entry->protected_record;
        entry->desired_len = kitchen_settings_encode(state, key, entry->desired);
        entry->saved_len = entry->desired_len;
        memcpy(entry->saved, entry->desired, entry->saved_len);
    }
}
static void submit(kitchen_settings_t *settings, unsigned key)
{
    kitchen_settings_entry_t *entry = &settings->entries[key];
    if (!settings->available || entry->protected_record) return;
    uint64_t ticket = 0;
    kitchen_storage_result_t ret = settings->backend.submit(settings->backend.context, key,
        entry->desired, entry->desired_len, &ticket);
    if (ret != KITCHEN_STORAGE_PENDING || !ticket) {
        entry->failed = true;
        if (ret == KITCHEN_STORAGE_PROTECTED) entry->protected_record = true;
        return;
    }
    entry->ticket = ticket;
    entry->sent_len = entry->desired_len;
    memcpy(entry->sent, entry->desired, entry->sent_len);
    entry->pending = true;
    entry->failed = false;
}
void kitchen_settings_request(kitchen_settings_t *settings, const kitchen_state_t *state, unsigned key)
{
    if (key >= KITCHEN_SETTINGS_KEYS) return;
    kitchen_settings_entry_t *entry = &settings->entries[key];
    entry->desired_len = kitchen_settings_encode(state, key, entry->desired);
    if (entry->pending && equal(entry->desired, entry->desired_len, entry->sent, entry->sent_len)) return;
    if (!entry->pending && !entry->failed && equal(entry->desired, entry->desired_len, entry->saved, entry->saved_len)) return;
    submit(settings, key);
}
void kitchen_settings_poll(kitchen_settings_t *settings)
{
    if (!settings->available) return;
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key) {
        kitchen_settings_entry_t *entry = &settings->entries[key];
        if (!entry->pending) continue;
        kitchen_storage_result_t ret = settings->backend.poll(settings->backend.context, key, entry->ticket);
        if (ret == KITCHEN_STORAGE_PENDING) continue;
        entry->pending = false;
        if (ret == KITCHEN_STORAGE_OK) {
            entry->saved_len = entry->sent_len;
            memcpy(entry->saved, entry->sent, entry->saved_len);
            /* Do not let an old completion clear a failed newer submission. */
            if (equal(entry->desired, entry->desired_len, entry->sent, entry->sent_len)) entry->failed = false;
        } else entry->failed = true;
    }
}
void kitchen_settings_retry(kitchen_settings_t *settings, const kitchen_state_t *state)
{
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key)
        if (settings->entries[key].failed) kitchen_settings_request(settings, state, key);
}
bool kitchen_settings_failed(const kitchen_settings_t *settings)
{
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key)
        if (settings->entries[key].failed && !settings->entries[key].protected_record) return true;
    return false;
}
bool kitchen_settings_protected(const kitchen_settings_t *settings)
{
    for (unsigned key = 0; key < KITCHEN_SETTINGS_KEYS; ++key)
        if (settings->entries[key].protected_record) return true;
    return false;
}
