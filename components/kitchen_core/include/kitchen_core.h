#pragma once
#include <stdint.h>
#include <stdbool.h>

#define KITCHEN_MAX_TIMERS 5
#define KITCHEN_NAME_MAX 32
#define KITCHEN_MAX_SECONDS 86399U

typedef enum { KITCHEN_EMPTY, KITCHEN_READY, KITCHEN_RUNNING, KITCHEN_PAUSED, KITCHEN_DONE } kitchen_status_t;
typedef enum { KITCHEN_ICON_NONE, KITCHEN_ICON_CUP, KITCHEN_ICON_POT,
    KITCHEN_ICON_PAN, KITCHEN_ICON_OVEN, KITCHEN_ICON_COUNT } kitchen_icon_t;
typedef struct {
    uint32_t id;
    kitchen_status_t status;
    char name[KITCHEN_NAME_MAX + 1];
    kitchen_icon_t icon;
    uint8_t color;
    uint32_t duration_s;
    uint64_t deadline_ms, remaining_ms, span_ms;
} kitchen_timer_t;
typedef struct {
    kitchen_timer_t timers[KITCHEN_MAX_TIMERS];
    uint8_t volume_percent, pending_chimes;
} kitchen_state_t;
typedef enum {
    KITCHEN_CREATE, KITCHEN_PAUSE, KITCHEN_RESUME, KITCHEN_CANCEL,
    KITCHEN_ACKNOWLEDGE, KITCHEN_START, KITCHEN_REPEAT,
    KITCHEN_ADJUST, KITCHEN_SET_TIME, KITCHEN_EXTEND, KITCHEN_EDIT,
    KITCHEN_SET_VOLUME, KITCHEN_PREVIEW_VOLUME, KITCHEN_SAVE_VOLUME, KITCHEN_RETRY_SAVE
} kitchen_command_kind_t;
typedef struct {
    kitchen_command_kind_t kind;
    uint32_t id;
    int32_t seconds;
    const char *name;
    kitchen_icon_t icon;
} kitchen_command_t;
#ifdef __cplusplus
extern "C" {
#endif
void kitchen_core_init(kitchen_state_t *state);
void kitchen_core_update(kitchen_state_t *state, uint64_t now_ms);
kitchen_timer_t *kitchen_core_find(kitchen_state_t *state, uint32_t id);
uint64_t kitchen_core_remaining(const kitchen_timer_t *timer, uint64_t now_ms);
uint64_t kitchen_core_elapsed(const kitchen_timer_t *timer, uint64_t now_ms);
bool kitchen_core_apply(kitchen_state_t *state, const kitchen_command_t *command, uint64_t now_ms);
#ifdef __cplusplus
}
#endif
