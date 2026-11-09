#pragma once
#include "lvgl.h"
#include "kitchen_settings.h"
#include "esp_err.h"
#if LVGL_VERSION_MAJOR != 8 || LVGL_VERSION_MINOR != 4
#error "Kitchen UI requires LVGL 8.4"
#endif
#if !LV_USE_LABEL || !LV_USE_BTN || !LV_USE_ROLLER || !LV_USE_TEXTAREA || !LV_USE_KEYBOARD || !LV_USE_SLIDER
#error "Kitchen UI requires LABEL, BTN, ROLLER, TEXTAREA, KEYBOARD and SLIDER widgets"
#endif

typedef struct kitchen_ui kitchen_ui_t;
typedef struct {
    kitchen_ui_t *ui;
    uint32_t id;
    lv_obj_t *object, *time, *name, *status, *icon, *action, *track, *fill;
    kitchen_status_t painted_status;
    kitchen_icon_t painted_icon;
    char painted_name[KITCHEN_NAME_MAX + 1];
    int progress, glow, glow_width, border_width, border_opa;
    int inset, radius;
    uint64_t progress_revision, progress_session, progress_started;
    kitchen_status_t progress_status;
    int progress_from;
    bool painted, progress_valid, progress_smoothing;
} kitchen_card_t;
typedef struct {
    kitchen_ui_t *ui;
    int action, value;
} kitchen_binding_t;
struct kitchen_ui {
    lv_obj_t *root, *modal, *panel, *body, *normal, *editor_body;
    lv_obj_t *keyboard, *name_field, *name_button, *icon_button, *icon_row;
    lv_obj_t *icon_guard, *close_button;
    lv_obj_t *name_apply, *name_cancel;
    lv_obj_t *hours, *minutes, *seconds, *apply, *edit_cancel, *message;
    lv_obj_t *roller_labels[3];
    int fields_width;
    lv_obj_t *heading, *time, *status, *track, *fill, *primary, *secondary, *minus, *plus, *repeat;
    lv_obj_t *presets[6], *icon_buttons[KITCHEN_ICON_COUNT];
    lv_obj_t *volume_button, *volume_slider, *volume_value, *notice, *retry;
    lv_timer_t *tick;
    kitchen_state_t *state;
    kitchen_settings_t *settings;
    void *context;
    bool (*command)(void *, const kitchen_command_t *);
    uint64_t (*now_ms)(void *);
    void (*back)(void *);
    kitchen_card_t cards[KITCHEN_MAX_TIMERS];
    kitchen_card_t overlay_card;
    kitchen_binding_t bindings[64];
    unsigned binding_count, modal_binding_base;
    uint32_t selected;
    int editor, modal_kind;
    kitchen_status_t shown_status;
    kitchen_icon_t draft_icon;
    char draft_name[KITCHEN_NAME_MAX + 1];
    bool active, failed, audio_available, volume_gesture, completion_shown, naming, icons_open;
    bool closing, opening, normal_layout_valid;
};
/* GUI-lock required. Parent is borrowed; root, timers and animations are owned. */
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t kitchen_ui_create(kitchen_ui_t *, lv_obj_t *, kitchen_state_t *, kitchen_settings_t *,
    bool (*command)(void *, const kitchen_command_t *), uint64_t (*now_ms)(void *),
    void (*back)(void *), bool audio_available, void *context);
void kitchen_ui_destroy(kitchen_ui_t *);
void kitchen_ui_set_active(kitchen_ui_t *, bool active);
void kitchen_ui_refresh(kitchen_ui_t *);
#ifdef __cplusplus
}
#endif
