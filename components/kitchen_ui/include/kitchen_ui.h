#pragma once
#include "lvgl.h"
#include "kitchen_core.h"
#include "esp_err.h"
#if LVGL_VERSION_MAJOR != 8 || LVGL_VERSION_MINOR != 4
#error "Kitchen UI requires LVGL 8.4"
#endif
#if !LV_USE_ARC || !LV_USE_LABEL || !LV_USE_BTN || !LV_USE_ROLLER || !LV_USE_TEXTAREA || !LV_USE_KEYBOARD || !LV_USE_SLIDER
#error "Kitchen UI requires ARC, LABEL, BTN, ROLLER, TEXTAREA, KEYBOARD and SLIDER widgets"
#endif

typedef struct kitchen_ui kitchen_ui_t;
typedef struct {
    kitchen_ui_t *ui;
    uint32_t id;
    lv_obj_t *object, *ring, *time, *name, *status, *icon, *action;
    int32_t current_x, current_y, current_size;
    int32_t from_x, from_y, from_size, to_x, to_y, to_size;
    bool present, painted, highlighted;
    uint32_t painted_color;
    kitchen_status_t painted_status;
    kitchen_icon_t painted_icon;
    int painted_arc, painted_opa;
    int painted_shadow_spread, painted_shadow_opa, painted_shadow_width;
    char painted_name[KITCHEN_NAME_MAX + 1];
} kitchen_card_t;
typedef struct {
    uint32_t id;
    lv_coord_t x, y, size;
} kitchen_layout_t;
typedef struct {
    kitchen_ui_t *ui;
    int action, value;
} kitchen_binding_t;
struct kitchen_ui {
    lv_obj_t *root, *page, *content, *panel, *modal, *keyboard, *name_field;
    lv_obj_t *hours, *minutes, *seconds, *apply, *message, *new_button, *empty, *edit_button, *drop_target, *cancel_button;
    lv_obj_t *volume_button, *volume_slider, *volume_value;
    bool audio_available, volume_gesture;
    lv_img_dsc_t halo[9];
    uint8_t *halo_pixels;
    int halo_size;
    lv_obj_t *minus[3], *plus[3], *presets[10], *icon_buttons[KITCHEN_ICON_COUNT];
    lv_timer_t *tick;
    kitchen_state_t *state;
    void *context;
    bool (*command)(void *context, const kitchen_command_t *command);
    uint64_t (*now_ms)(void *context);
    void (*back)(void *context);
    kitchen_card_t cards[KITCHEN_MAX_TIMERS], detail;
    kitchen_layout_t layout[2][KITCHEN_MAX_TIMERS];
    bool layout_valid[2];
    bool animate_active_entry;
    kitchen_binding_t bindings[96];
    unsigned binding_count, modal_binding_base;
    uint32_t generation, selected, pressed_id, press_tick, drag_id;
    uint32_t preview_order[KITCHEN_MAX_TIMERS];
    lv_point_t press_point, drag_point;
    int screen, return_screen, editor;
    kitchen_status_t detail_status;
    kitchen_icon_t draft_icon;
    lv_coord_t scroll[2];
    bool active, dirty, list_dirty, dragged, press_moved, failed, completion_shown;
};
/* All operations require the owner's GUI lock / GUI callback context.
 * Parent is borrowed. Only root, child objects, timer and animations are owned. */
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t kitchen_ui_create(kitchen_ui_t *ui, lv_obj_t *parent, kitchen_state_t *state,
    bool (*command)(void *, const kitchen_command_t *), uint64_t (*now_ms)(void *),
    void (*back)(void *), bool audio_available, void *context);
void kitchen_ui_destroy(kitchen_ui_t *ui);
void kitchen_ui_set_active(kitchen_ui_t *ui, bool active);
void kitchen_ui_refresh(kitchen_ui_t *ui);
#ifdef __cplusplus
}
#endif
