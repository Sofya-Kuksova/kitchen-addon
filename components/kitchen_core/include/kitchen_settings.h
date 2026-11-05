#pragma once
#include <stddef.h>
#include "kitchen_core.h"

#define KITCHEN_SETTINGS_KEYS (KITCHEN_MAX_TIMERS + 1)
#define KITCHEN_RECORD_MAX (7 + KITCHEN_NAME_MAX)
/* Key 0 is volume; keys 1..6 are stable slot IDs. */
typedef enum {
    KITCHEN_STORAGE_OK, KITCHEN_STORAGE_PENDING, KITCHEN_STORAGE_MISSING,
    KITCHEN_STORAGE_ERROR, KITCHEN_STORAGE_PROTECTED
} kitchen_storage_result_t;
typedef struct {
    void *context;
    /* load runs only at app creation, outside GUI-lock. It must wait for all
     * previously accepted submissions before returning committed records.
     * Oversized/newer records return PROTECTED; they must never be overwritten. */
    kitchen_storage_result_t (*load)(void *, unsigned key, uint8_t *, size_t *length);
    /* Nonblocking; copy bytes before return. PENDING means accepted. Tickets are
     * nonzero, backend-wide and never reused across app destruction/recreation.
     * A newer submission for a key may replace an unstarted older submission. */
    kitchen_storage_result_t (*submit)(void *, unsigned key, const uint8_t *, size_t length, uint64_t *ticket);
    /* Nonblocking; OK only after set+commit succeed. Older tickets superseded
     * by another submission return ERROR. No callbacks or app/UI pointers. */
    kitchen_storage_result_t (*poll)(void *, unsigned key, uint64_t ticket);
} kitchen_storage_backend_t;
typedef struct {
    uint8_t desired[KITCHEN_RECORD_MAX], sent[KITCHEN_RECORD_MAX], saved[KITCHEN_RECORD_MAX];
    size_t desired_len, sent_len, saved_len;
    uint64_t ticket;
    kitchen_storage_result_t loaded;
    const char *load_reason; /* Static diagnostic string, never serialized. */
    bool pending, failed, protected_record;
} kitchen_settings_entry_t;
typedef struct {
    kitchen_storage_backend_t backend;
    kitchen_settings_entry_t entries[KITCHEN_SETTINGS_KEYS];
    bool available;
} kitchen_settings_t;
#ifdef __cplusplus
extern "C" {
#endif
/* out has at least KITCHEN_RECORD_MAX bytes. Init expects a core-init state. */
size_t kitchen_settings_encode(const kitchen_state_t *, unsigned key, uint8_t *out);
kitchen_storage_result_t kitchen_settings_decode(kitchen_state_t *, unsigned key, const uint8_t *, size_t);
void kitchen_settings_init(kitchen_settings_t *, kitchen_state_t *, const kitchen_storage_backend_t *);
void kitchen_settings_request(kitchen_settings_t *, const kitchen_state_t *, unsigned key);
void kitchen_settings_poll(kitchen_settings_t *);
void kitchen_settings_retry(kitchen_settings_t *, const kitchen_state_t *);
bool kitchen_settings_failed(const kitchen_settings_t *);
bool kitchen_settings_protected(const kitchen_settings_t *);
#ifdef __cplusplus
}
#endif
