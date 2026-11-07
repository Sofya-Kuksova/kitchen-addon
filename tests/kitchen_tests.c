/* Host-only acceptance scenarios. The owner compiles/runs this explicitly;
 * this file is not registered with the ESP-IDF project. */
#include "kitchen_settings.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool apply(kitchen_state_t *state, kitchen_command_kind_t kind, unsigned id, int seconds, uint64_t time)
{
    kitchen_command_t cmd = {.kind = kind, .id = id, .seconds = seconds, .name = "", .icon = KITCHEN_ICON_NONE};
    return kitchen_core_apply(state, &cmd, time);
}
static void model(void)
{
    kitchen_state_t state;
    kitchen_core_init(&state);
    assert(state.volume_percent == 40);
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        assert(state.timers[i].id == i + 1 && state.timers[i].status == KITCHEN_EMPTY);
        assert(state.timers[i].color == i % 5 && !state.timers[i].name[0]);
    }
    assert(!apply(&state, KITCHEN_CREATE, 0, 180, 0));
    assert(!apply(&state, KITCHEN_CREATE, 6, 180, 0));
    assert(kitchen_core_find(&state, 6) == NULL);
    assert(!apply(&state, KITCHEN_CREATE, 1, 0, 0));
    assert(!apply(&state, KITCHEN_CREATE, 1, 86400, 0));
    assert(apply(&state, KITCHEN_CREATE, 1, 180, 100));
    assert(!apply(&state, KITCHEN_CREATE, 1, 180, 100));
    assert(apply(&state, KITCHEN_PAUSE, 1, 0, 1111));
    assert(state.timers[0].remaining_ms == 178989);
    assert(apply(&state, KITCHEN_RESUME, 1, 0, 99000));
    assert(state.timers[0].deadline_ms == 277989);
    assert(apply(&state, KITCHEN_ADJUST, 1, 60, 99000));
    assert(state.timers[0].deadline_ms == 337989 && state.timers[0].duration_s == 180);
    assert(state.timers[0].span_ms == 238989);
    kitchen_core_update(&state, 338999);
    assert(state.timers[0].status == KITCHEN_DONE && state.pending_chimes == 1);
    assert(kitchen_core_elapsed(&state.timers[0], 338999) == 1010);
    kitchen_core_update(&state, 400000);
    assert(state.pending_chimes == 1);
    assert(kitchen_core_elapsed(&state.timers[0], 337989 + UINT64_C(172800123)) == UINT64_C(172800123));
    kitchen_command_t edit = {.kind = KITCHEN_EDIT, .id = 1, .name = "Tea", .icon = KITCHEN_ICON_CUP};
    assert(kitchen_core_apply(&state, &edit, 400000));
    assert(state.timers[0].status == KITCHEN_DONE);
    assert(apply(&state, KITCHEN_ACKNOWLEDGE, 1, 0, 400000));
    assert(state.timers[0].status == KITCHEN_READY && state.timers[0].duration_s == 180);
    assert(state.timers[0].id == 1 && state.timers[0].color == 0 && !strcmp(state.timers[0].name, "Tea"));
    assert(apply(&state, KITCHEN_SET_TIME, 1, 270, 400000));
    assert(apply(&state, KITCHEN_START, 1, 0, 400000));
    assert(state.timers[0].deadline_ms == 670000);
    assert(apply(&state, KITCHEN_SET_TIME, 1, 10, 500000));
    assert(state.timers[0].deadline_ms == 510000 && state.timers[0].span_ms == 10000);
    assert(state.timers[0].duration_s == 270);
    kitchen_core_update(&state, 510000);
    assert(apply(&state, KITCHEN_EXTEND, 1, 60, 510100));
    assert(state.timers[0].deadline_ms == 570100 && state.timers[0].span_ms == 60000);
    kitchen_core_update(&state, 570100);
    assert(apply(&state, KITCHEN_REPEAT, 1, 0, 570100));
    assert(state.timers[0].deadline_ms == 840100);
    assert(apply(&state, KITCHEN_CANCEL, 1, 0, 570100));
    assert(!apply(&state, KITCHEN_SET_TIME, 1, 0, 570100));
    assert(apply(&state, KITCHEN_SET_TIME, 1, 86399, 570100));
    assert(!apply(&state, KITCHEN_ADJUST, 1, 60, 570100));
    assert(!apply(&state, KITCHEN_ADJUST, 1, 180, 570100));
    assert(apply(&state, KITCHEN_SET_TIME, 1, 61, 570100));
    assert(apply(&state, KITCHEN_START, 1, 0, 570100));
    /* +/- one minute must preserve fractional milliseconds. */
    assert(apply(&state, KITCHEN_ADJUST, 1, -60, 570600));
    assert(kitchen_core_remaining(&state.timers[0], 570600) == 500);
    assert(apply(&state, KITCHEN_PAUSE, 1, 0, 570600));
    assert(apply(&state, KITCHEN_SET_TIME, 1, 1, 570600));
    assert(state.timers[0].status == KITCHEN_PAUSED && state.timers[0].remaining_ms == 1000);
    assert(!apply(&state, KITCHEN_RESUME, 1, 0, UINT64_MAX - 1));

    kitchen_core_init(&state);
    for (unsigned id = 1; id <= KITCHEN_MAX_TIMERS; ++id) assert(apply(&state, KITCHEN_CREATE, id, 1, 10));
    kitchen_core_update(&state, 1010);
    assert(state.pending_chimes == KITCHEN_MAX_TIMERS);
    state.pending_chimes = 0;
    kitchen_core_update(&state, 999999);
    assert(state.pending_chimes == 0);
    for (unsigned id = 1; id <= KITCHEN_MAX_TIMERS; ++id) {
        assert(state.timers[id - 1].status == KITCHEN_DONE);
        assert(apply(&state, KITCHEN_EXTEND, id, 1, 999999));
    }
    assert(apply(&state, KITCHEN_SET_VOLUME, 0, 0, 999999));
    kitchen_core_update(&state, 1000999);
    assert(state.pending_chimes == 0);
    assert(apply(&state, KITCHEN_SET_VOLUME, 0, 100, 1000999));
    kitchen_core_update(&state, 1001000);
    assert(state.pending_chimes == 0); /* unmute must not replay old DONE */
}

typedef struct {
    uint8_t committed[KITCHEN_SETTINGS_KEYS][KITCHEN_RECORD_MAX];
    uint8_t pending[KITCHEN_SETTINGS_KEYS][KITCHEN_RECORD_MAX];
    size_t length[KITCHEN_SETTINGS_KEYS], pending_length[KITCHEN_SETTINGS_KEYS];
    uint64_t tickets[KITCHEN_SETTINGS_KEYS], sequence;
    kitchen_storage_result_t results[KITCHEN_SETTINGS_KEYS];
    unsigned writes;
    bool reject;
} fake_t;
static kitchen_storage_result_t load(void *context, unsigned key, uint8_t *data, size_t *length)
{
    fake_t *fake = context;
    if (!fake->length[key]) return KITCHEN_STORAGE_MISSING;
    assert(*length >= fake->length[key]);
    *length = fake->length[key];
    memcpy(data, fake->committed[key], *length);
    return KITCHEN_STORAGE_OK;
}
static kitchen_storage_result_t submit(void *context, unsigned key, const uint8_t *data, size_t length, uint64_t *ticket)
{
    fake_t *fake = context;
    if (fake->reject) return KITCHEN_STORAGE_ERROR;
    memcpy(fake->pending[key], data, length);
    fake->pending_length[key] = length;
    fake->tickets[key] = *ticket = ++fake->sequence;
    fake->results[key] = KITCHEN_STORAGE_PENDING;
    ++fake->writes;
    return KITCHEN_STORAGE_PENDING;
}
static kitchen_storage_result_t poll(void *context, unsigned key, uint64_t ticket)
{
    fake_t *fake = context;
    return fake->tickets[key] == ticket ? fake->results[key] : KITCHEN_STORAGE_ERROR;
}
static void complete(fake_t *fake, unsigned key, bool success)
{
    fake->results[key] = success ? KITCHEN_STORAGE_OK : KITCHEN_STORAGE_ERROR;
    if (success) {
        fake->length[key] = fake->pending_length[key];
        memcpy(fake->committed[key], fake->pending[key], fake->length[key]);
    }
}
static void persistence(void)
{
    fake_t fake = {0};
    kitchen_storage_backend_t backend = {.context = &fake, .load = load, .submit = submit, .poll = poll};
    kitchen_state_t state;
    kitchen_settings_t settings;
    kitchen_core_init(&state);
    kitchen_settings_init(&settings, &state, &backend);
    assert(fake.writes == 0 && state.timers[0].status == KITCHEN_EMPTY);
    assert(apply(&state, KITCHEN_CREATE, 1, 180, 0));
    kitchen_settings_request(&settings, &state, 1);
    uint64_t first = settings.entries[1].ticket;
    assert(settings.entries[1].pending && !kitchen_settings_failed(&settings));
    assert(apply(&state, KITCHEN_CANCEL, 1, 0, 0));
    assert(apply(&state, KITCHEN_SET_TIME, 1, 240, 0));
    kitchen_settings_request(&settings, &state, 1);
    assert(settings.entries[1].ticket > first);
    complete(&fake, 1, true);
    kitchen_settings_poll(&settings);
    assert(!settings.entries[1].pending);
    kitchen_settings_request(&settings, &state, 1);
    assert(fake.writes == 2); /* unchanged confirmed settings do not write */

    assert(apply(&state, KITCHEN_SET_TIME, 1, 300, 0));
    kitchen_settings_request(&settings, &state, 1);
    complete(&fake, 1, false);
    kitchen_settings_poll(&settings);
    assert(kitchen_settings_failed(&settings));
    kitchen_settings_retry(&settings, &state);
    complete(&fake, 1, true);
    kitchen_settings_poll(&settings);
    assert(!kitchen_settings_failed(&settings));

    /* A newer enqueue failure must survive the older successful completion. */
    assert(apply(&state, KITCHEN_SET_TIME, 1, 360, 0));
    kitchen_settings_request(&settings, &state, 1);
    assert(apply(&state, KITCHEN_SET_TIME, 1, 420, 0));
    fake.reject = true;
    kitchen_settings_request(&settings, &state, 1);
    complete(&fake, 1, true);
    kitchen_settings_poll(&settings);
    assert(kitchen_settings_failed(&settings));
    fake.reject = false;
    kitchen_settings_retry(&settings, &state);
    complete(&fake, 1, true);
    kitchen_settings_poll(&settings);
    assert(!kitchen_settings_failed(&settings));

    /* Reverting to a committed value must supersede an in-flight edit too. */
    assert(apply(&state, KITCHEN_SET_TIME, 1, 480, 0));
    kitchen_settings_request(&settings, &state, 1);
    assert(apply(&state, KITCHEN_SET_TIME, 1, 420, 0));
    kitchen_settings_request(&settings, &state, 1);
    complete(&fake, 1, true);
    kitchen_settings_poll(&settings);
    assert(fake.committed[1][1] == (420 & 255));

    assert(apply(&state, KITCHEN_SET_VOLUME, 0, 0, 0));
    unsigned writes = fake.writes;
    kitchen_settings_poll(&settings);
    assert(fake.writes == writes); /* dragging/tick never writes volume */
    kitchen_settings_request(&settings, &state, 0);
    complete(&fake, 0, true);
    kitchen_settings_poll(&settings);
    assert(fake.committed[0][1] == 0);

    /* All five records, all runtime states, including the largest ASCII name. */
    for (unsigned id = 1; id <= KITCHEN_MAX_TIMERS; ++id) {
        if (id > 1) assert(apply(&state, KITCHEN_CREATE, id, id * 10, 0));
        kitchen_command_t edit = {.kind = KITCHEN_EDIT, .id = id,
            .name = "12345678901234567890123456789012", .icon = KITCHEN_ICON_OVEN};
        assert(kitchen_core_apply(&state, &edit, 0));
        kitchen_settings_request(&settings, &state, id);
        complete(&fake, id, true);
    }
    kitchen_settings_poll(&settings);
    assert(apply(&state, KITCHEN_START, 1, 0, 0));
    assert(apply(&state, KITCHEN_PAUSE, 2, 0, 1));
    kitchen_core_update(&state, 30000); /* slot 3 DONE, slot 4 still RUNNING */
    kitchen_state_t reboot;
    kitchen_settings_t loaded;
    kitchen_core_init(&reboot);
    kitchen_settings_init(&loaded, &reboot, &backend);
    assert(reboot.volume_percent == 0 && reboot.pending_chimes == 0);
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        assert(reboot.timers[i].status == KITCHEN_READY);
        assert(!reboot.timers[i].deadline_ms && !reboot.timers[i].remaining_ms);
        assert(reboot.timers[i].icon == KITCHEN_ICON_OVEN && strlen(reboot.timers[i].name) == 32);
    }
    fake.committed[3][5] = 255; /* one corrupt icon must not clear other keys */
    kitchen_core_init(&reboot);
    kitchen_settings_init(&loaded, &reboot, &backend);
    assert(reboot.timers[2].status == KITCHEN_EMPTY && reboot.timers[1].status == KITCHEN_READY);
    assert(reboot.volume_percent == 0);
    fake.committed[4][0] = 2;
    kitchen_core_init(&reboot);
    kitchen_settings_init(&loaded, &reboot, &backend);
    writes = fake.writes;
    assert(kitchen_settings_protected(&loaded));
    assert(apply(&reboot, KITCHEN_CREATE, 4, 10, 0));
    kitchen_settings_request(&loaded, &reboot, 4);
    assert(fake.writes == writes && fake.committed[4][0] == 2);
    kitchen_settings_init(&loaded, &reboot, NULL);
    assert(!loaded.available);
}
static void records(void)
{
    kitchen_state_t state;
    kitchen_core_init(&state);
    uint8_t data[KITCHEN_RECORD_MAX] = {1, 0x7f, 0x51, 1, 0, 4, 0}; /* 86399 s */
    assert(kitchen_settings_encode(&state, 6, data) == 0);
    assert(kitchen_settings_decode(&state, 6, data, 7) == KITCHEN_STORAGE_ERROR);
    assert(kitchen_settings_decode(&state, 1, data, 7) == KITCHEN_STORAGE_OK);
    assert(state.timers[0].duration_s == 86399);
    data[1] = 0x80; /* 86400 s */
    assert(kitchen_settings_decode(&state, 1, data, 7) == KITCHEN_STORAGE_ERROR);
    assert(state.timers[0].duration_s == 86399); /* failed decode never partly applies */
    data[0] = 2;
    assert(kitchen_settings_decode(&state, 1, data, 7) == KITCHEN_STORAGE_PROTECTED);
    data[0] = 1; data[1] = 1; data[2] = data[3] = 0; data[6] = 1; data[7] = 31;
    assert(kitchen_settings_decode(&state, 1, data, 8) == KITCHEN_STORAGE_ERROR);
    data[7] = 127;
    assert(kitchen_settings_decode(&state, 1, data, 8) == KITCHEN_STORAGE_ERROR);
    data[7] = 'A';
    assert(kitchen_settings_decode(&state, 1, data, 8) == KITCHEN_STORAGE_OK);
    assert(kitchen_settings_decode(&state, 1, data, 7) == KITCHEN_STORAGE_ERROR);
    const uint8_t invalid_volume[] = {1, 101};
    assert(kitchen_settings_decode(&state, 0, invalid_volume, 2) == KITCHEN_STORAGE_ERROR);
    for (unsigned value = 0; value <= 100; value += value ? 60 : 40) {
        uint8_t volume[] = {1, (uint8_t)value};
        assert(kitchen_settings_decode(&state, 0, volume, 2) == KITCHEN_STORAGE_OK);
        assert(state.volume_percent == value);
    }
}
int main(void)
{
    model(); records(); persistence();
    puts("Kitchen core/settings acceptance: PASS");
    return 0;
}
