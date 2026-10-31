#include "kitchen_core.h"
#include <string.h>

void kitchen_core_init(kitchen_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->next_id = 1;
    state->volume_percent = 40;
}
kitchen_timer_t *kitchen_core_find(kitchen_state_t *state, uint32_t id)
{
    if (!state || !id) return NULL;
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i)
        if (state->timers[i].id == id) return &state->timers[i];
    return NULL;
}
uint64_t kitchen_core_remaining(const kitchen_timer_t *timer, uint64_t now_ms)
{
    if (timer->status == KITCHEN_RUNNING)
        return timer->deadline_ms > now_ms ? timer->deadline_ms - now_ms : 0;
    if (timer->status == KITCHEN_PAUSED) return timer->remaining_ms;
    if (timer->status == KITCHEN_SAVED) return (uint64_t)timer->duration_s * 1000;
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
        if (timer->id && timer->status == KITCHEN_RUNNING && now_ms >= timer->deadline_ms) {
            timer->status = KITCHEN_DONE;
            timer->remaining_ms = 0;
            if (state->volume_percent && state->pending_chimes < KITCHEN_MAX_TIMERS) ++state->pending_chimes;
        }
    }
}
static void erase_id(uint32_t *order, uint8_t *count, uint32_t id)
{
    for (unsigned i = 0; i < *count; ++i) {
        if (order[i] != id) continue;
        memmove(order + i, order + i + 1, (*count - i - 1) * sizeof(*order));
        --*count;
        break;
    }
}
static void save_timer(kitchen_state_t *state, kitchen_timer_t *timer)
{
    erase_id(state->active, &state->active_count, timer->id);
    memmove(state->saved + 1, state->saved, state->saved_count * sizeof(*state->saved));
    state->saved[0] = timer->id;
    ++state->saved_count;
    timer->status = KITCHEN_SAVED;
    timer->remaining_ms = timer->deadline_ms = 0;
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
static void run(kitchen_timer_t *timer, uint64_t ms, uint64_t now_ms)
{
    timer->status = KITCHEN_RUNNING;
    timer->remaining_ms = timer->span_ms = ms;
    timer->deadline_ms = now_ms + ms;
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
    if (cmd->kind == KITCHEN_CREATE) {
        if (state->active_count + state->saved_count >= KITCHEN_MAX_TIMERS ||
            !valid_seconds(cmd->seconds) || !valid_name(cmd->name) ||
            (unsigned)cmd->icon >= KITCHEN_ICON_COUNT) return false;
        for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
            if (!state->timers[i].id) { timer = &state->timers[i]; break; }
        }
        if (!timer) return false;
        /* Never reuse a live ID, including after the unsigned counter wraps. */
        while (!state->next_id || kitchen_core_find(state, state->next_id)) ++state->next_id;
        *timer = (kitchen_timer_t){.id = state->next_id++, .color = state->next_color,
            .duration_s = (uint32_t)cmd->seconds, .icon = cmd->icon};
        memcpy(timer->name, cmd->name, strlen(cmd->name) + 1);
        state->next_color = (state->next_color + 1) % 5;
        state->active[state->active_count++] = timer->id;
        run(timer, (uint64_t)cmd->seconds * 1000, now_ms);
        return true;
    }
    if (!timer) return false;
    switch (cmd->kind) {
    case KITCHEN_PAUSE:
        if (timer->status != KITCHEN_RUNNING) return false;
        timer->remaining_ms = kitchen_core_remaining(timer, now_ms);
        timer->status = KITCHEN_PAUSED;
        return true;
    case KITCHEN_RESUME:
        if (timer->status != KITCHEN_PAUSED) return false;
        timer->deadline_ms = now_ms + timer->remaining_ms;
        timer->status = KITCHEN_RUNNING;
        return true;
    case KITCHEN_CANCEL:
        if (timer->status != KITCHEN_RUNNING && timer->status != KITCHEN_PAUSED) return false;
        save_timer(state, timer);
        return true;
    case KITCHEN_ACKNOWLEDGE:
        if (timer->status != KITCHEN_DONE) return false;
        save_timer(state, timer);
        return true;
    case KITCHEN_REMOVE:
        if (timer->status != KITCHEN_SAVED) return false;
        erase_id(state->saved, &state->saved_count, timer->id);
        memset(timer, 0, sizeof(*timer));
        return true;
    case KITCHEN_START:
        if (timer->status != KITCHEN_SAVED) return false;
        erase_id(state->saved, &state->saved_count, timer->id);
        state->active[state->active_count++] = timer->id;
        run(timer, (uint64_t)timer->duration_s * 1000, now_ms);
        return true;
    case KITCHEN_REPEAT:
        if (timer->status != KITCHEN_DONE) return false;
        run(timer, (uint64_t)timer->duration_s * 1000, now_ms);
        return true;
    case KITCHEN_EXTEND:
        if (timer->status != KITCHEN_DONE || !valid_seconds(cmd->seconds)) return false;
        run(timer, (uint64_t)cmd->seconds * 1000, now_ms);
        return true;
    case KITCHEN_ADJUST:
    case KITCHEN_SET_TIME: {
        if (timer->status == KITCHEN_DONE) return false;
        int64_t ms = cmd->kind == KITCHEN_ADJUST ?
            (int64_t)kitchen_core_remaining(timer, now_ms) + (int64_t)cmd->seconds * 1000 :
            (int64_t)cmd->seconds * 1000;
        if (ms <= 0 || ms > (int64_t)KITCHEN_MAX_SECONDS * 1000) return false;
        if (timer->status == KITCHEN_SAVED) timer->duration_s = (uint32_t)(ms / 1000);
        else {
            timer->remaining_ms = (uint64_t)ms;
            if (timer->status == KITCHEN_RUNNING) timer->deadline_ms = now_ms + (uint64_t)ms;
            /* A new arc baseline avoids a full arc clipped for minutes after adding time. */
            if (cmd->kind == KITCHEN_SET_TIME || (uint64_t)ms > timer->span_ms) timer->span_ms = (uint64_t)ms;
        }
        return true;
    }
    case KITCHEN_EDIT:
        if (timer->status == KITCHEN_DONE || !valid_name(cmd->name) ||
            (unsigned)cmd->icon >= KITCHEN_ICON_COUNT) return false;
        memcpy(timer->name, cmd->name, strlen(cmd->name) + 1);
        timer->icon = cmd->icon;
        return true;
    case KITCHEN_MOVE: {
        bool saved = timer->status == KITCHEN_SAVED;
        uint32_t *order = saved ? state->saved : state->active;
        uint8_t *count = saved ? &state->saved_count : &state->active_count;
        if (cmd->position >= *count) return false;
        erase_id(order, count, timer->id);
        memmove(order + cmd->position + 1, order + cmd->position,
            (*count - cmd->position) * sizeof(*order));
        order[cmd->position] = timer->id;
        ++*count;
        return true;
    }
    default: return false;
    }
}
