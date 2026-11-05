#include "kitchen_core.h"
#include <string.h>

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
void kitchen_core_update(kitchen_state_t *state, uint64_t now_ms)
{
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        kitchen_timer_t *timer = &state->timers[i];
        if (timer->status == KITCHEN_RUNNING && now_ms >= timer->deadline_ms) {
            timer->status = KITCHEN_DONE;
            timer->remaining_ms = 0;
            if (state->volume_percent && state->pending_chimes < KITCHEN_MAX_TIMERS) ++state->pending_chimes;
        }
    }
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
static bool run(kitchen_timer_t *timer, uint64_t ms, uint64_t now_ms)
{
    if (now_ms > UINT64_MAX - ms) return false;
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
        return run(timer, (uint64_t)cmd->seconds * 1000, now_ms);
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
        timer->status = KITCHEN_READY;
        timer->remaining_ms = timer->deadline_ms = timer->span_ms = 0;
        return true;
    case KITCHEN_START:
        if (timer->status != KITCHEN_READY) return false;
        return run(timer, (uint64_t)timer->duration_s * 1000, now_ms);
    case KITCHEN_REPEAT:
        if (timer->status != KITCHEN_DONE) return false;
        return run(timer, (uint64_t)timer->duration_s * 1000, now_ms);
    case KITCHEN_EXTEND:
        if (timer->status != KITCHEN_DONE || !valid_seconds(cmd->seconds)) return false;
        return run(timer, (uint64_t)cmd->seconds * 1000, now_ms);
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
