#include "kitchen_core.h"
#include <string.h>
#include <stdio.h>

void kitchen_core_init(kitchen_state_t *state)
{
    memset(state, 0, sizeof(*state));
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        state->timers[i].id = i + 1;
        state->timers[i].color = i % 5;
        state->timers[i].status = KITCHEN_EMPTY;
    }
    state->volume_percent = 40;
}
kitchen_timer_t *kitchen_core_find(kitchen_state_t *state, uint32_t id)
{
    return state && id >= 1 && id <= KITCHEN_MAX_TIMERS ? &state->timers[id - 1] : NULL;
}
uint64_t kitchen_core_remaining(const kitchen_timer_t *timer, uint64_t now_ms)
{
    if (timer->status == KITCHEN_RUNNING)
        return timer->deadline_ms > now_ms ? timer->deadline_ms - now_ms : 0;
    if (timer->status == KITCHEN_PAUSED) return timer->remaining_ms;
    if (timer->status == KITCHEN_READY) return (uint64_t)timer->duration_s * 1000;
    return 0;
}
uint64_t kitchen_core_elapsed(const kitchen_timer_t *timer, uint64_t now_ms)
{
    return timer->status == KITCHEN_DONE && now_ms >= timer->deadline_ms ? now_ms - timer->deadline_ms : 0;
}
unsigned kitchen_core_progress(const kitchen_timer_t *timer, uint64_t now_ms)
{
    if (timer->status == KITCHEN_DONE) return 1000;
    if (timer->status != KITCHEN_RUNNING && timer->status != KITCHEN_PAUSED) return 0;
    uint64_t ms = kitchen_core_remaining(timer, now_ms);
    if (!timer->span_ms || ms >= timer->span_ms) return 0;
    /* Valid spans are bounded by KITCHEN_MAX_SECONDS * 1000. */
    return (unsigned)((timer->span_ms - ms) * 1000 / timer->span_ms);
}
void kitchen_core_format_time(char *buffer, unsigned size, const kitchen_timer_t *timer, uint64_t now_ms)
{
    bool done = timer->status == KITCHEN_DONE;
    uint64_t seconds = done ? kitchen_core_elapsed(timer, now_ms) / 1000 :
        (kitchen_core_remaining(timer, now_ms) + 999) / 1000;
    const char *sign = done && seconds ? "\xe2\x88\x92" : ""; /* U+2212 */
    if (seconds >= 3600) snprintf(buffer, size, "%s%llu:%02u:%02u", sign,
        (unsigned long long)(seconds / 3600), (unsigned)(seconds / 60 % 60), (unsigned)(seconds % 60));
    else snprintf(buffer, size, "%s%02u:%02u", sign, (unsigned)(seconds / 60), (unsigned)(seconds % 60));
}
void kitchen_core_take_audio(kitchen_state_t *state, kitchen_audio_batch_t *batch)
{
    memset(batch, 0, sizeof(*batch));
    batch->warnings = state->pending_warnings;
    batch->done = state->pending_done;
    state->pending_warnings = state->pending_done = 0;
    if (!state->volume_percent) batch->warnings = batch->done = 0;
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        const kitchen_timer_t *timer = &state->timers[i];
        batch->session[i] = timer->session;
        batch->deadline_ms[i] = timer->deadline_ms;
        if (timer->status == KITCHEN_RUNNING) batch->running |= 1U << i;
        if (timer->status == KITCHEN_DONE) batch->finished |= 1U << i;
    }
}
void kitchen_core_update(kitchen_state_t *state, uint64_t now_ms)
{
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        kitchen_timer_t *timer = &state->timers[i];
        uint8_t bit = 1U << i;
        if (timer->status != KITCHEN_RUNNING) continue;
        if (now_ms >= timer->deadline_ms) {
            timer->status = KITCHEN_DONE;
            timer->remaining_ms = 0;
            timer->warning_armed = false;
            state->pending_warnings &= ~bit;
            if (state->volume_percent) state->pending_done |= bit;
        } else if (timer->warning_armed && timer->deadline_ms - now_ms <= 5000) {
            timer->warning_armed = false;
            if (state->volume_percent) state->pending_warnings |= bit;
        }
    }
}
static void invalidate(kitchen_state_t *state, kitchen_timer_t *timer)
{
    uint8_t bit = 1U << (timer->id - 1);
    state->pending_warnings &= ~bit;
    state->pending_done &= ~bit;
    if (!++timer->session) ++timer->session;
}
static bool valid_name(const char *name)
{
    if (!name) return false;
    for (unsigned i = 0; i <= KITCHEN_NAME_MAX; ++i) {
        if (!name[i]) return true;
        if ((unsigned char)name[i] < 32 || (unsigned char)name[i] > 126) return false;
    }
    return false;
}
static bool valid_seconds(int32_t seconds)
{
    return seconds > 0 && seconds <= (int32_t)KITCHEN_MAX_SECONDS;
}
static bool run(kitchen_state_t *state, kitchen_timer_t *timer, uint64_t ms, uint64_t now_ms)
{
    if (now_ms > UINT64_MAX - ms) return false;
    invalidate(state, timer);
    timer->warning_armed = ms > 5000;
    timer->status = KITCHEN_RUNNING;
    timer->remaining_ms = timer->span_ms = ms;
    timer->deadline_ms = now_ms + ms;
    return true;
}
bool kitchen_core_apply(kitchen_state_t *state, const kitchen_command_t *cmd, uint64_t now_ms)
{
    if (!state || !cmd) return false;
    kitchen_core_update(state, now_ms);
    if (cmd->kind == KITCHEN_SET_VOLUME) {
        if (cmd->seconds < 0 || cmd->seconds > 100) return false;
        state->volume_percent = (uint8_t)cmd->seconds;
        if (!cmd->seconds) state->pending_warnings = state->pending_done = 0;
        return true;
    }
    kitchen_timer_t *timer = kitchen_core_find(state, cmd->id);
    if (!timer) return false;
    if (cmd->kind == KITCHEN_CREATE) {
        if (timer->status != KITCHEN_EMPTY || !valid_seconds(cmd->seconds) ||
            !valid_name(cmd->name) || (unsigned)cmd->icon >= KITCHEN_ICON_COUNT ||
            now_ms > UINT64_MAX - (uint64_t)cmd->seconds * 1000) return false;
        timer->duration_s = (uint32_t)cmd->seconds;
        timer->icon = cmd->icon;
        memcpy(timer->name, cmd->name, strlen(cmd->name) + 1);
        return run(state, timer, (uint64_t)cmd->seconds * 1000, now_ms);
    }
    if (timer->status == KITCHEN_EMPTY) return false;
    switch (cmd->kind) {
    case KITCHEN_PAUSE:
        if (timer->status != KITCHEN_RUNNING) return false;
        timer->remaining_ms = kitchen_core_remaining(timer, now_ms);
        timer->status = KITCHEN_PAUSED;
        return true;
    case KITCHEN_RESUME:
        if (timer->status != KITCHEN_PAUSED || now_ms > UINT64_MAX - timer->remaining_ms) return false;
        timer->deadline_ms = now_ms + timer->remaining_ms;
        timer->status = KITCHEN_RUNNING;
        return true;
    case KITCHEN_CANCEL:
    case KITCHEN_ACKNOWLEDGE:
        if (cmd->kind == KITCHEN_CANCEL ?
            (timer->status != KITCHEN_RUNNING && timer->status != KITCHEN_PAUSED) : timer->status != KITCHEN_DONE) return false;
        invalidate(state, timer);
        timer->warning_armed = false;
        timer->status = KITCHEN_READY;
        timer->remaining_ms = timer->deadline_ms = timer->span_ms = 0;
        return true;
    case KITCHEN_START:
        if (timer->status != KITCHEN_READY) return false;
        return run(state, timer, (uint64_t)timer->duration_s * 1000, now_ms);
    case KITCHEN_REPEAT:
        if (timer->status != KITCHEN_DONE) return false;
        return run(state, timer, (uint64_t)timer->duration_s * 1000, now_ms);
    case KITCHEN_EXTEND:
        if (timer->status != KITCHEN_DONE || !valid_seconds(cmd->seconds)) return false;
        return run(state, timer, (uint64_t)cmd->seconds * 1000, now_ms);
    case KITCHEN_ADJUST:
    case KITCHEN_SET_TIME: {
        if (timer->status == KITCHEN_DONE) return false;
        if (cmd->kind == KITCHEN_ADJUST && cmd->seconds != -60 && cmd->seconds != 60) return false;
        int64_t ms = cmd->kind == KITCHEN_ADJUST ?
            (int64_t)kitchen_core_remaining(timer, now_ms) + (int64_t)cmd->seconds * 1000 :
            (int64_t)cmd->seconds * 1000;
        if (ms <= 0 || (cmd->kind == KITCHEN_SET_TIME && ms < 1000) ||
            ms > (int64_t)KITCHEN_MAX_SECONDS * 1000) return false;
        if (timer->status == KITCHEN_READY) timer->duration_s = (uint32_t)(ms / 1000);
        else {
            if (timer->status == KITCHEN_RUNNING && now_ms > UINT64_MAX - (uint64_t)ms) return false;
            ++timer->time_revision;
            state->pending_warnings &= ~(1U << (timer->id - 1));
            if (ms > 5000) timer->warning_armed = true;
            timer->remaining_ms = (uint64_t)ms;
            if (timer->status == KITCHEN_RUNNING) timer->deadline_ms = now_ms + (uint64_t)ms;
            if (cmd->kind == KITCHEN_SET_TIME || (uint64_t)ms > timer->span_ms) timer->span_ms = (uint64_t)ms;
        }
        return true;
    }
    case KITCHEN_EDIT:
        if (!valid_name(cmd->name) || (unsigned)cmd->icon >= KITCHEN_ICON_COUNT) return false;
        if (timer->name != cmd->name) memcpy(timer->name, cmd->name, strlen(cmd->name) + 1);
        timer->icon = cmd->icon;
        return true;
    default: return false;
    }
}
