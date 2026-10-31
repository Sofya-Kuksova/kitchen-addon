#include "kitchen_ui.h"
#include "kitchen_symbols.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(kitchen_font_36)
LV_FONT_DECLARE(kitchen_font_48)
LV_FONT_DECLARE(kitchen_font_64)
LV_FONT_DECLARE(kitchen_font_112)
LV_FONT_DECLARE(kitchen_text_18)
LV_FONT_DECLARE(kitchen_text_32)
LV_FONT_DECLARE(kitchen_lvgl_symbols_32)
LV_FONT_DECLARE(kitchen_symbols_40)
LV_FONT_DECLARE(kitchen_symbols_48)
LV_FONT_DECLARE(kitchen_symbols_64)
LV_FONT_DECLARE(kitchen_font_56)
LV_FONT_DECLARE(kitchen_font_72)
LV_FONT_DECLARE(kitchen_text_20)
LV_FONT_DECLARE(kitchen_text_24)

#define BG 0x111113
#define BUTTON 0x29292c
#define ACCENT 0xffb547
#define LIGHT 0xf4f4f5
#define MUTED 0xa1a1a6
#define PANEL 0x1b1b1e
#define FIELD 0x242427
#define EDGE 0x3b3b40
#define DISABLED_TEXT 0x7d7d84
#define ANIMATION_MS 220

enum { SCREEN_ACTIVE, SCREEN_SAVED, SCREEN_NEW, SCREEN_DETAIL, SCREEN_ERROR };
enum { EDIT_NONE, EDIT_REMAINING, EDIT_DURATION, EDIT_BONUS };
enum { A_BACK, A_NEW, A_SAVED, A_ACTIVE, A_PAUSE, A_RESUME, A_CANCEL_TIMER,
    A_ACK, A_REPEAT, A_START, A_REMOVE, A_ADJUST, A_SET, A_APPLY, A_CANCEL_EDIT,
    A_PRESET, A_CREATE, A_EDIT, A_SAVE_NAME, A_CANCEL_NAME, A_ICON, A_VOLUME, A_CLOSE_VOLUME };
static const uint32_t palette[] = {ACCENT, 0x2dd4bf, 0x60a5fa, 0xa78bfa, 0xf472b6};
static const char *icon_names[] = {KITCHEN_SYMBOL_BLOCK, KITCHEN_SYMBOL_LOCAL_CAFE, KITCHEN_SYMBOL_STOCKPOT, KITCHEN_SYMBOL_SKILLET, KITCHEN_SYMBOL_OVEN};
static const int increments[] = {180, 300, 600};
static void action_event(lv_event_t *event);
static void card_event(lv_event_t *event);
static void card_action(lv_event_t *event);
static void tick(lv_timer_t *timer);
static void rebuild(kitchen_ui_t *ui);
static void sync_list(kitchen_ui_t *ui, bool animate);
static void refresh_cards(kitchen_ui_t *ui);
static void close_modal(kitchen_ui_t *ui);

static uint64_t now(kitchen_ui_t *ui) { return ui->now_ms(ui->context); }
static kitchen_timer_t *selected(kitchen_ui_t *ui) { return kitchen_core_find(ui->state, ui->selected); }
static void button_disabled_style(lv_obj_t *obj)
{
    /* Override the theme's grey filter, preserving the normal/checked fill. */
    lv_obj_set_style_color_filter_opa(obj, LV_OPA_TRANSP, LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(obj, lv_color_hex(DISABLED_TEXT), LV_STATE_DISABLED);
    lv_obj_set_style_text_opa(obj, LV_OPA_40, LV_STATE_DISABLED);
}
static void disabled(lv_obj_t *obj, bool value)
{
    if (!obj) return;
    if (value) lv_obj_add_state(obj, LV_STATE_DISABLED);
    else lv_obj_clear_state(obj, LV_STATE_DISABLED);
}
static void visible(lv_obj_t *obj, bool value)
{
    if (!obj) return;
    if (value) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t *box(kitchen_ui_t *ui, lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    if (!parent) { ui->failed = true; return NULL; }
    lv_obj_t *obj = lv_obj_create(parent);
    if (!obj) { ui->failed = true; return NULL; }
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
static lv_obj_t *text(kitchen_ui_t *ui, lv_obj_t *parent, const char *value, int x, int y, int width)
{
    if (!parent) { ui->failed = true; return NULL; }
    lv_obj_t *obj = lv_label_create(parent);
    if (!obj) { ui->failed = true; return NULL; }
    lv_label_set_text(obj, value);
    lv_obj_set_pos(obj, x, y);
    if (width > 0) {
        lv_obj_set_width(obj, width);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
        lv_obj_set_height(obj, lv_font_get_line_height(lv_obj_get_style_text_font(obj, 0)));
    }
    return obj;
}
static void title(kitchen_ui_t *ui, lv_obj_t *parent, const char *value, int x, int y, int width)
{
    lv_obj_t *obj = text(ui, parent, value, x, y, width);
    if (obj) {
        lv_obj_set_style_text_font(obj, &kitchen_text_32, 0);
        lv_obj_set_height(obj, kitchen_text_32.line_height);
    }
}
static lv_obj_t *button(kitchen_ui_t *ui, lv_obj_t *parent, const char *value,
    int x, int y, int w, int h, bool accent, int action, int parameter)
{
    if (!parent || ui->binding_count >= sizeof(ui->bindings) / sizeof(ui->bindings[0])) {
        ui->failed = true; return NULL;
    }
    lv_obj_t *obj = lv_btn_create(parent);
    if (!obj) { ui->failed = true; return NULL; }
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(accent ? ACCENT : BUTTON), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(accent ? BG : LIGHT), 0);
    button_disabled_style(obj);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_t *caption = text(ui, obj, value, 0, 0, w - 12);
    if (caption) {
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(caption);
    }
    kitchen_binding_t *binding = &ui->bindings[ui->binding_count++];
    *binding = (kitchen_binding_t){ui, action, parameter};
    lv_obj_add_event_cb(obj, action_event, LV_EVENT_CLICKED, binding);
    return obj;
}
static lv_obj_t *content_button(kitchen_ui_t *ui, lv_obj_t *parent, const char *value,
    int y, bool accent, int action)
{
    lv_point_t extent;
    lv_txt_get_size(&extent, value, &kitchen_text_20, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int width = extent.x + 32;
    if (width < 48) width = 48;
    lv_obj_t *obj = button(ui, parent, value, 1024 - 20 - width, y, width, 48, accent, action, 0);
    if (obj) {
        lv_obj_set_style_pad_hor(obj, 16, 0);
        lv_obj_set_style_pad_ver(obj, 0, 0);
        lv_obj_t *caption = lv_obj_get_child(obj, 0);
        if (caption) { lv_obj_set_width(caption, extent.x); lv_obj_center(caption); }
    }
    return obj;
}
static void format_time(char *buffer, size_t size, uint64_t seconds)
{
    if (seconds >= 3600)
        snprintf(buffer, size, "%llu:%02u:%02u", (unsigned long long)(seconds / 3600),
            (unsigned)(seconds / 60 % 60), (unsigned)(seconds % 60));
    else snprintf(buffer, size, "%02u:%02u", (unsigned)(seconds / 60), (unsigned)(seconds % 60));
}
static void go(kitchen_ui_t *ui, int screen)
{
    if (ui->content && ui->screen <= SCREEN_SAVED) {
        ui->scroll[ui->screen] = lv_obj_get_scroll_y(ui->content);
        ui->layout_valid[ui->screen] = true;
        for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
            kitchen_card_t *card = &ui->cards[i];
            kitchen_layout_t *cached = &ui->layout[ui->screen][i];
            *cached = (kitchen_layout_t){0};
            if (card->object && card->present)
                *cached = (kitchen_layout_t){card->id, card->current_x, card->current_y, card->current_size};
        }
    }
    ui->screen = screen;
    ui->editor = EDIT_NONE;
    ui->dirty = true;
}
static bool send(kitchen_ui_t *ui, kitchen_command_kind_t kind, int seconds)
{
    kitchen_command_t cmd = {.kind = kind, .id = ui->selected, .seconds = seconds};
    bool changed = ui->command(ui->context, &cmd);
    if (changed && (kind == KITCHEN_START || kind == KITCHEN_CANCEL || kind == KITCHEN_ACKNOWLEDGE))
        ui->animate_active_entry = true;
    return changed;
}
static void message(kitchen_ui_t *ui, const char *value)
{
    if (ui->message) lv_label_set_text(ui->message, value);
}
/* Only selected Material Symbols are baked into the local bitmap font. */
static void draw_symbol(lv_event_t *event, const char *symbol, const lv_font_t *font)
{
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target(event), &area);
    area.y1 += (lv_area_get_height(&area) - font->line_height) / 2;
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.font = font;
    dsc.color = lv_color_hex(lv_obj_has_state(lv_event_get_target(event), LV_STATE_DISABLED) ? DISABLED_TEXT : LIGHT);
    dsc.opa = lv_obj_has_state(lv_event_get_target(event), LV_STATE_DISABLED) ? LV_OPA_40 : LV_OPA_COVER;
    dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_draw_label(lv_event_get_draw_ctx(event), &dsc, &area, symbol, NULL);
}
static void icon_draw(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    kitchen_timer_t *timer = kitchen_core_find(card->ui->state, card->id);
    if (timer && timer->icon != KITCHEN_ICON_NONE) draw_symbol(event, icon_names[timer->icon], card->current_size > 320 ? &kitchen_symbols_64 : &kitchen_symbols_40);
}
static const char *primary_symbol(kitchen_status_t status)
{
    return status == KITCHEN_RUNNING ? LV_SYMBOL_PAUSE : status == KITCHEN_DONE ? LV_SYMBOL_OK : LV_SYMBOL_PLAY;
}
static void action_draw(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    kitchen_timer_t *timer = kitchen_core_find(card->ui->state, card->id);
    if (timer) draw_symbol(event, primary_symbol(timer->status), &kitchen_lvgl_symbols_32);
}
static void position_card(kitchen_card_t *card, int x, int y, int size)
{
    card->current_x = x; card->current_y = y; card->current_size = size;
    lv_obj_set_pos(card->object, x, y);
    lv_obj_set_size(card->object, size, size + 56);
    lv_obj_set_size(card->ring, size, size);
    if (card->time) {
        lv_obj_set_width(card->time, size - 30);
        kitchen_timer_t *timer = kitchen_core_find(card->ui->state, card->id);
        uint64_t current = now(card->ui);
        uint64_t displayed_seconds = timer ? (timer->status == KITCHEN_DONE ?
            kitchen_core_elapsed(timer, current) / 1000 : (kitchen_core_remaining(timer, current) + 999) / 1000) : 0;
        bool hours = displayed_seconds >= 3600;
        const lv_font_t *font = size > 320 ? (hours ? &kitchen_font_72 : &kitchen_font_112) :
            hours ? (size >= 270 ? &kitchen_font_56 : &kitchen_font_48) :
            size > 260 ? &kitchen_font_72 : &kitchen_font_64;
        lv_obj_set_style_text_font(card->time, font, 0);
        lv_obj_set_height(card->time, font->line_height);
        lv_obj_align(card->time, LV_ALIGN_TOP_MID, 0, size / 2 - font->line_height / 2 - 12);
    }
    kitchen_timer_t *item = kitchen_core_find(card->ui->state, card->id);
    bool state_label = item && item->status != KITCHEN_RUNNING;
    bool detail = size > 320;
    if (card->name) {
        const lv_font_t *font = detail ? &kitchen_text_32 : &kitchen_text_24;
        lv_obj_set_style_text_font(card->name, font, 0);
        int inset = detail ? 28 : 22;
        lv_obj_set_size(card->name, size - 2 * inset, font->line_height);
        lv_obj_set_pos(card->name, inset, size / 2 + (detail ? (state_label ? 100 : 72) : (state_label ? 58 : 26)));
    }
    if (card->status) { lv_obj_set_width(card->status, size - 44); lv_obj_set_pos(card->status, 22, size / 2 + (detail ? 56 : 26)); }
    if (card->icon) {
        int badge = detail ? 96 : 64;
        lv_obj_set_size(card->icon, badge, badge);
        lv_obj_set_pos(card->icon, size * 85 / 100 - badge / 2, size * 15 / 100 - badge / 2);
    }
    if (card->action) {
        lv_point_t extent;
        lv_txt_get_size(&extent, primary_symbol(item ? item->status : KITCHEN_SAVED),
            &kitchen_lvgl_symbols_32, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        int width = extent.x + 32;
        if (width < 48) width = 48;
        lv_obj_set_width(card->action, width);
        lv_obj_set_pos(card->action, (size - width) / 2, size + 8);
    }
}
static void card_geometry(void *context, int32_t progress)
{
    kitchen_card_t *card = context;
    position_card(card,
        card->from_x + (card->to_x - card->from_x) * progress / 1024,
        card->from_y + (card->to_y - card->from_y) * progress / 1024,
        card->from_size + (card->to_size - card->from_size) * progress / 1024);
}
static void card_opacity(void *context, int32_t opacity)
{
    kitchen_card_t *card = context;
    lv_obj_set_style_opa(card->object, opacity, 0);
}
static void finish_card_exit(kitchen_card_t *card)
{
    if (!card->object || card->present) return;
    lv_anim_del(card, card_geometry);
    bool was_visible = !lv_obj_has_flag(card->object, LV_OBJ_FLAG_HIDDEN);
    visible(card->object, false);
    lv_obj_set_style_opa(card->object, LV_OPA_TRANSP, 0);
    /* Overflowing children/shadows may extend beyond the wrapper's invalidation
     * rectangle. Repaint the viewport once, including the previous positions. */
    if (was_visible && card->ui->content) lv_obj_invalidate(card->ui->content);
}
static void card_opacity_ready(lv_anim_t *animation)
{
    finish_card_exit(animation->var);
}
static void animate_value(kitchen_card_t *card, lv_anim_exec_xcb_t callback, int from, int to)
{
    lv_anim_del(card, callback);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, card);
    lv_anim_set_exec_cb(&animation, callback);
    lv_anim_set_values(&animation, from, to);
    lv_anim_set_time(&animation, ANIMATION_MS);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    if (callback == card_opacity) lv_anim_set_ready_cb(&animation, card_opacity_ready);
    lv_anim_start(&animation);
}
static void stop_cards(kitchen_ui_t *ui)
{
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) lv_anim_del(&ui->cards[i], NULL);
    lv_anim_del(&ui->detail, NULL);
}
/* One shared set per page; no per-card blur or transformed image layers. */
static void release_halo(kitchen_ui_t *ui)
{
    if (ui->halo_pixels) {
        for (unsigned i = 0; i < 9; ++i) lv_img_cache_invalidate_src(&ui->halo[i]);
        lv_mem_free(ui->halo_pixels);
    }
    ui->halo_pixels = NULL;
    ui->halo_size = 0;
}
static void prepare_halo(kitchen_ui_t *ui, int size)
{
    if (ui->halo_size == size) return;
    release_halo(ui);
    ui->halo_size = size;
    int side = size + 46;
    size_t pixels = (size_t)side * side;
    ui->halo_pixels = lv_mem_alloc(pixels * 9);
    if (!ui->halo_pixels) return; /* Static native shadow if RAM is insufficient. */
    for (int frame = 0; frame < 9; ++frame) {
        uint8_t *data = ui->halo_pixels + pixels * frame;
        int inner = size + 2 * (frame - 4) - 36;
        int outer = size + 2 * (frame - 4) + 38;
        int inner2 = inner * inner, outer2 = outer * outer;
        for (int y = 0; y < side; ++y) {
            int dy = 2 * y - side + 1;
            for (int x = 0; x < side; ++x) {
                int dx = 2 * x - side + 1;
                int distance = dx * dx + dy * dy;
                int alpha = distance <= size * size || distance >= outer2 ? 0 :
                    (outer2 - distance) * 255 / (outer2 - inner2);
                alpha = alpha > 255 ? 255 : alpha;
                data[y * side + x] = (uint8_t)(alpha * alpha * (765 - 2 * alpha) / 65025);
            }
        }
        ui->halo[frame] = (lv_img_dsc_t){.header = {.cf = LV_IMG_CF_ALPHA_8BIT,
            .w = side, .h = side}, .data_size = pixels, .data = data};
    }
}
static void halo_draw(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_coord_t *extra = lv_event_get_param(event);
        if (*extra < 23) *extra = 23;
        return;
    }
    kitchen_ui_t *ui = card->ui;
    if (!card->highlighted || !ui->halo_pixels || card->current_size != ui->halo_size) return;
    lv_area_t area;
    lv_obj_get_coords(card->object, &area);
    area.x1 -= 23; area.y1 -= 23;
    area.x2 = area.x1 + ui->halo_size + 45;
    area.y2 = area.y1 + ui->halo_size + 45;
    lv_draw_img_dsc_t dsc;
    lv_draw_img_dsc_init(&dsc);
    dsc.opa = (uint32_t)card->painted_shadow_opa *
        lv_obj_get_style_opa_recursive(card->object, LV_PART_MAIN) / 255;
    dsc.recolor = lv_color_hex(card->painted_color);
    lv_draw_img(lv_event_get_draw_ctx(event), &dsc, &area,
        &ui->halo[card->painted_shadow_spread + 4]);
}
static bool make_card(kitchen_ui_t *ui, kitchen_card_t *card, lv_obj_t *parent, uint32_t id, bool compact)
{
    *card = (kitchen_card_t){.ui = ui, .id = id};
    card->object = box(ui, parent, 0, 0, 280, 344, BG);
    if (!card->object) return false;
    lv_obj_set_style_bg_opa(card->object, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(card->object, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_event_cb(card->object, halo_draw, LV_EVENT_DRAW_MAIN, card);
    lv_obj_add_event_cb(card->object, halo_draw, LV_EVENT_REFR_EXT_DRAW_SIZE, card);
    lv_obj_refresh_ext_draw_size(card->object);
    card->ring = lv_arc_create(card->object);
    if (!card->ring) return false;
    lv_obj_remove_style_all(card->ring);
    lv_obj_set_size(card->ring, 280, 280);
    lv_arc_set_bg_angles(card->ring, 0, 360);
    lv_arc_set_rotation(card->ring, 270);
    lv_arc_set_range(card->ring, 0, 1000);
    lv_obj_set_style_arc_width(card->ring, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(card->ring, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(card->ring, lv_color_hex(0x36363b), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(card->ring, true, LV_PART_INDICATOR);
    lv_obj_set_style_radius(card->ring, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_clear_flag(card->ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    card->time = text(ui, card->object, "00:00", 0, 0, 240);
    card->name = text(ui, card->object, "", 0, 0, 240);
    card->status = text(ui, card->object, "", 0, 0, 240);
    if (!card->time || !card->name || !card->status) return false;
    lv_obj_set_style_text_font(card->name, &kitchen_text_24, 0);
    lv_obj_set_style_text_font(card->status, &kitchen_text_24, 0);
    lv_obj_set_height(card->name, kitchen_text_24.line_height);
    lv_obj_set_height(card->status, kitchen_text_24.line_height);
    lv_obj_set_style_text_align(card->time, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(card->name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(card->status, LV_TEXT_ALIGN_CENTER, 0);
    card->icon = box(ui, card->object, 0, 0, 64, 64, BG);
    if (!card->icon) return false;
    lv_obj_set_style_radius(card->icon, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(card->icon, 2, 0);
    lv_obj_add_event_cb(card->icon, icon_draw, LV_EVENT_DRAW_MAIN, card);
    if (compact) {
        lv_obj_add_flag(card->object, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card->object, card_event, LV_EVENT_ALL, card);
        card->action = lv_btn_create(card->object);
        if (!card->action) return false;
        lv_obj_set_size(card->action, 64, 48);
        lv_obj_set_style_bg_color(card->action, lv_color_hex(BUTTON), 0);
        lv_obj_set_style_radius(card->action, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(card->action, 0, 0);
        lv_obj_set_style_shadow_width(card->action, 0, 0);
        button_disabled_style(card->action);
        lv_obj_add_event_cb(card->action, card_action, LV_EVENT_CLICKED, card);
        lv_obj_add_event_cb(card->action, action_draw, LV_EVENT_DRAW_MAIN, card);
    }
    position_card(card, 0, 0, 280);
    return true;
}
static void update_card(kitchen_card_t *card, uint64_t current)
{
    kitchen_timer_t *timer = kitchen_core_find(card->ui->state, card->id);
    if (!timer || !card->object) return;
    uint64_t ms = kitchen_core_remaining(timer, current);
    char value[32];
    format_time(value, sizeof(value), timer->status == KITCHEN_DONE ?
        kitchen_core_elapsed(timer, current) / 1000 : (ms + 999) / 1000);
    if (strcmp(lv_label_get_text(card->time), value)) {
        lv_label_set_text(card->time, value);
        position_card(card, card->current_x, card->current_y, card->current_size);
    }
    if (!card->painted || strcmp(card->painted_name, timer->name)) {
        lv_label_set_text(card->name, timer->name);
        memcpy(card->painted_name, timer->name, sizeof(card->painted_name));
    }
    if (!card->painted || card->painted_status != timer->status) {
        const char *status = timer->status == KITCHEN_PAUSED ? "PAUSED" : timer->status == KITCHEN_DONE ?
            "DONE" : timer->status == KITCHEN_SAVED ? "SAVED" : "";
        lv_label_set_text(card->status, status);
        lv_obj_set_style_text_color(card->status, lv_color_hex(timer->status == KITCHEN_SAVED ? MUTED : palette[timer->color]), 0);
        position_card(card, card->current_x, card->current_y, card->current_size);
        if (card->action) lv_obj_invalidate(card->action);
    }
    uint32_t color = timer->status == KITCHEN_SAVED ? 0x7b7b80 : palette[timer->color];
    if (!card->painted || card->painted_color != color) {
        lv_obj_set_style_arc_color(card->ring, lv_color_hex(color), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(card->ring, lv_color_mix(lv_color_hex(color), lv_color_hex(BG), LV_OPA_10), 0);
        lv_obj_set_style_bg_opa(card->ring, LV_OPA_COVER, 0);
        lv_obj_set_style_outline_color(card->ring, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(card->ring, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_border_color(card->icon, lv_color_hex(color), 0);
    }
    if (!card->painted || card->painted_icon != timer->icon) {
        visible(card->icon, timer->icon != KITCHEN_ICON_NONE);
        lv_obj_invalidate(card->icon);
    }
    int arc = timer->status == KITCHEN_DONE || timer->status == KITCHEN_SAVED ? 1000 :
        timer->span_ms ? (int)(ms * 1000 / timer->span_ms) : 0;
    if (!card->painted || card->painted_arc != arc) lv_arc_set_value(card->ring, arc);
    bool highlight = (timer->status == KITCHEN_RUNNING && ms > 0 && ms <= 60000) || timer->status == KITCHEN_DONE;
    if (!card->painted || card->highlighted != highlight) {
        /* Urgency uses only the soft halo; the progress arc stays unchanged. */
        lv_obj_set_style_outline_width(card->ring, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_pad(card->ring, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_ofs_x(card->ring, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_ofs_y(card->ring, 0, LV_PART_MAIN);
    }
    unsigned phase = (unsigned)(current % 1800);
    unsigned triangle = (phase <= 900 ? phase : 1800 - phase) * 256 / 900;
    unsigned eased = triangle * triangle * (768 - 2 * triangle) / 65536;
    int opacity = LV_OPA_TRANSP;
    if (!card->painted || card->painted_opa != opacity)
        lv_obj_set_style_outline_opa(card->ring, opacity, LV_PART_MAIN);
    int shadow_width = highlight && !card->ui->halo_pixels ? 36 : 0;
    int shadow_spread = highlight ? -4 + (int)(8 * eased / 256) : -4;
    int shadow_opa = highlight ? 96 + (int)(112 * eased / 256) : LV_OPA_TRANSP;
    if (card->ui->modal && card->painted && card->highlighted == highlight) {
        shadow_spread = card->painted_shadow_spread;
        shadow_opa = card->painted_shadow_opa;
    }
    if (!card->ui->halo_pixels) { shadow_spread = 0; shadow_opa = highlight ? 152 : 0; }
    if (!card->painted || card->painted_shadow_width != shadow_width) {
        lv_obj_set_style_shadow_width(card->ring, shadow_width, LV_PART_MAIN);
        lv_obj_set_style_shadow_spread(card->ring, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(card->ring, highlight ? 152 : 0, LV_PART_MAIN);
    }
    if (!card->painted || card->highlighted != highlight ||
        card->painted_shadow_spread != shadow_spread || card->painted_shadow_opa != shadow_opa)
        lv_obj_invalidate(card->object);
    card->painted = true;
    card->painted_status = timer->status;
    card->painted_color = color;
    card->painted_icon = timer->icon;
    card->painted_arc = arc;
    card->highlighted = highlight;
    card->painted_opa = opacity;
    card->painted_shadow_spread = shadow_spread;
    card->painted_shadow_opa = shadow_opa;
    card->painted_shadow_width = shadow_width;
}
static void layout_position(unsigned count, unsigned index, int *x, int *y, int *size)
{
    *size = count <= 3 ? 280 : 240;
    if (count == 1) { *x = 372; *y = 38; }
    else if (count == 2) { *x = index ? 594 : 150; *y = 38; }
    else if (count == 3) {
        *x = index == 2 ? 372 : index ? 712 : 32;
        *y = index == 2 ? 168 : 8;
    } else if (count == 4) {
        *x = 16 + index * 248;
        *y = index % 2 ? 160 : 8;
    } else {
        unsigned row = index / 5, local = index % 5;
        *x = local < 3 ? 40 + local * 352 : 216 + (local - 3) * 352;
        *y = row * 528 + (local < 3 ? 8 : 226);
    }
    /* Extra clipping margin for the halo; absolute screen positions stay fixed. */
    *y += 12;
}
static void sync_list(kitchen_ui_t *ui, bool animate)
{
    uint8_t count = ui->screen == SCREEN_ACTIVE ? ui->state->active_count : ui->state->saved_count;
    if (ui->screen == SCREEN_ACTIVE) prepare_halo(ui, count <= 3 ? 280 : 240);
    uint32_t *order = ui->drag_id ? ui->preview_order :
        ui->screen == SCREEN_ACTIVE ? ui->state->active : ui->state->saved;
    for (unsigned slot = 0; slot < KITCHEN_MAX_TIMERS; ++slot) {
        kitchen_card_t *card = &ui->cards[slot];
        uint32_t id = ui->state->timers[slot].id;
        unsigned index = 0;
        while (index < count && order[index] != id) ++index;
        if (!id || index == count) {
            if (card->object) {
                if (card->present) {
                    card->present = false;
                    /* Keep any in-flight movement until fade-out completes. */
                    lv_obj_clear_flag(card->object, LV_OBJ_FLAG_CLICKABLE);
                    disabled(card->action, true);
                    if (animate) animate_value(card, card_opacity,
                        lv_obj_get_style_opa(card->object, LV_PART_MAIN), LV_OPA_TRANSP);
                }
                /* Also settle exits interrupted by hide/suspend, whose animation
                 * was cancelled before its ready callback could run. */
                if (!animate || !lv_anim_get(card, card_opacity)) {
                    lv_anim_del(card, card_opacity);
                    finish_card_exit(card);
                }
            }
            continue;
        }
        bool created = !card->object;
        bool entering = created || !card->present || card->id != id;
        if (!card->object && !make_card(ui, card, ui->content, id, true)) { ui->failed = true; continue; }
        if (card->id != id) card->painted = false;
        card->id = id;
        card->present = true;
        lv_obj_add_flag(card->object, LV_OBJ_FLAG_CLICKABLE);
        disabled(card->action, false);
        visible(card->object, true);
        int x, y, size;
        layout_position(count, index, &x, &y, &size);
        card->from_x = card->current_x;
        card->from_y = card->current_y;
        card->from_size = card->current_size;
        card->to_x = x; card->to_y = y; card->to_size = size;
        if (entering) { card->from_x = x; card->from_y = y + 20; card->from_size = size; }
        if (created && ui->layout_valid[ui->screen]) {
            for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
                const kitchen_layout_t *cached = &ui->layout[ui->screen][i];
                if (cached->id != id) continue;
                card->from_x = cached->x; card->from_y = cached->y; card->from_size = cached->size;
                entering = false;
                break;
            }
        }
        if (ui->drag_id != id) {
            if (animate) animate_value(card, card_geometry, 0, 1024);
            else position_card(card, x, y, size);
        }
        if (entering && animate) animate_value(card, card_opacity, 0, LV_OPA_COVER);
        else lv_obj_set_style_opa(card->object, LV_OPA_COVER, 0);
    }
    visible(ui->empty, count == 0);
    disabled(ui->new_button, ui->state->active_count + ui->state->saved_count >= KITCHEN_MAX_TIMERS);
    if (ui->screen == SCREEN_ACTIVE) message(ui,
        ui->state->active_count + ui->state->saved_count >= KITCHEN_MAX_TIMERS ? "20 timer limit reached" : "");
    ui->list_dirty = false;
    refresh_cards(ui);
}
static void refresh_cards(kitchen_ui_t *ui)
{
    /* Keep translucent dialogs responsive; model/audio heartbeat continues. */
    if (ui->modal) return;
    uint64_t current = now(ui);
    if (ui->screen <= SCREEN_SAVED) {
        for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i)
            if (ui->cards[i].present) update_card(&ui->cards[i], current);
    } else if (ui->screen == SCREEN_DETAIL) update_card(&ui->detail, current);
}
static void rollers_set(kitchen_ui_t *ui, uint32_t seconds)
{
    if (!ui->hours || !ui->minutes || !ui->seconds) return;
    lv_roller_set_selected(ui->hours, seconds / 3600, LV_ANIM_OFF);
    lv_roller_set_selected(ui->minutes, seconds / 60 % 60, LV_ANIM_OFF);
    lv_roller_set_selected(ui->seconds, seconds % 60, LV_ANIM_OFF);
}
static uint32_t rollers_value(kitchen_ui_t *ui)
{
    if (!ui->hours || !ui->minutes || !ui->seconds) return 0;
    return lv_roller_get_selected(ui->hours) * 3600U + lv_roller_get_selected(ui->minutes) * 60U +
        lv_roller_get_selected(ui->seconds);
}
static void make_rollers(kitchen_ui_t *ui, lv_obj_t *parent, int x, int y, int width, uint32_t seconds)
{
    char options[180];
    lv_obj_t **objects[] = {&ui->hours, &ui->minutes, &ui->seconds};
    const char *names[] = {"Hours", "Minutes", "Seconds"};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned length = 0, limit = i ? 60 : 24;
        for (unsigned n = 0; n < limit; ++n)
            length += (unsigned)snprintf(options + length, sizeof(options) - length, n + 1 == limit ? "%02u" : "%02u\n", n);
        *objects[i] = lv_roller_create(parent);
        if (!*objects[i]) { ui->failed = true; return; }
        lv_obj_set_pos(*objects[i], x + i * (width + 12), y);
        lv_obj_set_width(*objects[i], width);
        lv_roller_set_options(*objects[i], options, LV_ROLLER_MODE_NORMAL);
        lv_obj_set_style_text_font(*objects[i], &kitchen_font_48, LV_PART_MAIN);
        lv_obj_set_style_text_font(*objects[i], &kitchen_font_48, LV_PART_SELECTED);
        lv_obj_set_style_bg_color(*objects[i], lv_color_hex(FIELD), LV_PART_MAIN);
        lv_obj_set_style_text_color(*objects[i], lv_color_hex(MUTED), LV_PART_MAIN);
        lv_obj_set_style_bg_color(*objects[i], lv_color_hex(EDGE), LV_PART_SELECTED);
        lv_obj_set_style_text_color(*objects[i], lv_color_hex(LIGHT), LV_PART_SELECTED);
        lv_obj_set_style_border_color(*objects[i], lv_color_hex(EDGE), LV_PART_MAIN);
        lv_obj_set_style_border_width(*objects[i], 1, LV_PART_MAIN);
        lv_obj_set_style_radius(*objects[i], 8, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(*objects[i], 12, LV_PART_MAIN);
        lv_roller_set_visible_row_count(*objects[i], 3);
        lv_obj_update_layout(*objects[i]);
        lv_obj_t *caption = text(ui, parent, names[i], x + i * (width + 12),
            y + lv_obj_get_height(*objects[i]) + 10, width);
        if (caption) {
            lv_obj_set_style_text_font(caption, &kitchen_text_24, 0);
            lv_obj_set_height(caption, kitchen_text_24.line_height);
            lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        }
    }
    rollers_set(ui, seconds);
}
static void hide_keyboard(kitchen_ui_t *ui)
{
    if (!ui->keyboard || lv_obj_has_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN)) return;
    visible(ui->keyboard, false);
    if (ui->modal && ui->name_field) lv_obj_set_y(lv_obj_get_parent(ui->name_field), 90);
    if (ui->screen == SCREEN_NEW) {
        if (ui->apply) lv_obj_set_y(ui->apply, 516);
        if (ui->cancel_button) lv_obj_set_y(ui->cancel_button, 516);
        for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) visible(ui->icon_buttons[i], true);
    }
    if (ui->name_field) lv_obj_clear_state(ui->name_field, LV_STATE_FOCUSED);
}
static void form_click(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target(event);
    if (!ui->active || ui->dirty || ui->failed || target != lv_event_get_current_target(event)) return;
    if (target == ui->modal) { close_modal(ui); return; }
    for (lv_obj_t *obj = target; obj; obj = lv_obj_get_parent(obj))
        if (obj == ui->name_field || obj == ui->keyboard) return;
    hide_keyboard(ui);
}
static void bind_form_clicks(kitchen_ui_t *ui, lv_obj_t *obj)
{
    /* Bind once after the form is built, after its action callbacks. Dismiss on
     * CLICKED so moving controls back cannot cancel the same tap's action. */
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
        lv_obj_add_event_cb(obj, form_click, LV_EVENT_CLICKED, ui);
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(obj); ++i)
        bind_form_clicks(ui, lv_obj_get_child(obj, i));
}
static void keyboard_event(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_READY || lv_event_get_code(event) == LV_EVENT_CANCEL)
        hide_keyboard(ui);
}
static void name_focus(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->keyboard) {
        ui->keyboard = lv_keyboard_create(ui->modal ? ui->modal : ui->page);
        if (!ui->keyboard) { ui->failed = true; return; }
        lv_obj_set_size(ui->keyboard, 1024, 220);
        lv_obj_set_style_text_font(ui->keyboard, &kitchen_text_20, LV_PART_ITEMS);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(PANEL), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(BUTTON), LV_PART_ITEMS);
        lv_obj_set_style_text_color(ui->keyboard, lv_color_hex(LIGHT), LV_PART_ITEMS);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(ui->keyboard, lv_color_hex(BG), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_align(ui->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(ui->keyboard, keyboard_event, LV_EVENT_ALL, ui);
    }
    if (ui->modal) lv_obj_set_y(lv_obj_get_parent(ui->name_field), 0);
    if (ui->screen == SCREEN_NEW) {
        if (ui->apply) lv_obj_set_y(ui->apply, 312);
        if (ui->cancel_button) lv_obj_set_y(ui->cancel_button, 312);
        for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) visible(ui->icon_buttons[i], false);
    }
    lv_keyboard_set_textarea(ui->keyboard, ui->name_field);
    visible(ui->keyboard, true);
    lv_obj_move_foreground(ui->keyboard);
}
static void optional_heading(kitchen_ui_t *ui, lv_obj_t *parent, const char *name, int x, int y, int width)
{
    /* Do not feed recolor markup through the generic LONG_DOT label helper. */
    lv_point_t extent;
    lv_txt_get_size(&extent, name, &kitchen_text_24, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_t *heading = text(ui, parent, name, x, y, 0);
    lv_obj_t *suffix = text(ui, parent, " · optional", x + extent.x, y, 0);
    if (heading) {
        lv_obj_set_style_text_font(heading, &kitchen_text_24, 0);
        lv_obj_set_style_text_color(heading, lv_color_hex(LIGHT), 0);
        lv_obj_set_height(heading, kitchen_text_24.line_height);
    }
    if (suffix) {
        lv_obj_set_style_text_font(suffix, &kitchen_text_24, 0);
        lv_obj_set_style_text_color(suffix, lv_color_hex(MUTED), 0);
        lv_label_set_long_mode(suffix, LV_LABEL_LONG_CLIP);
        lv_obj_set_size(suffix, width - extent.x, kitchen_text_24.line_height);
    }
}
static void make_name(kitchen_ui_t *ui, lv_obj_t *parent, int x, int y, int width, const char *value)
{
    optional_heading(ui, parent, "Name", x, y, width);
    ui->name_field = lv_textarea_create(parent);
    if (!ui->name_field) { ui->failed = true; return; }
    lv_obj_set_pos(ui->name_field, x, y + 38);
    lv_obj_set_style_text_font(ui->name_field, &kitchen_text_24, 0);
    lv_textarea_set_one_line(ui->name_field, true);
    /* one_line sets LV_SIZE_CONTENT: apply the intended height afterwards. */
    int height = ui->screen == SCREEN_NEW ? 64 : 56;
    lv_obj_set_size(ui->name_field, width, height);
    lv_obj_set_style_pad_ver(ui->name_field, (height - kitchen_text_24.line_height - 2) / 2, 0);
    lv_obj_set_style_pad_hor(ui->name_field, 12, 0);
    lv_obj_set_scrollbar_mode(ui->name_field, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(ui->name_field, LV_DIR_HOR);
    lv_textarea_set_max_length(ui->name_field, KITCHEN_NAME_MAX);
    static const char ascii[] =
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    lv_textarea_set_accepted_chars(ui->name_field, ascii);
    lv_textarea_set_text(ui->name_field, value);
    lv_textarea_set_placeholder_text(ui->name_field, "Tap here to name your timer");
    lv_obj_set_style_text_font(ui->name_field, &kitchen_text_24, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(ui->name_field, lv_color_hex(MUTED), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(ui->name_field, lv_color_hex(FIELD), 0);
    lv_obj_set_style_text_color(ui->name_field, lv_color_hex(LIGHT), 0);
    lv_obj_set_style_border_color(ui->name_field, lv_color_hex(EDGE), 0);
    lv_obj_set_style_border_width(ui->name_field, 1, 0);
    lv_obj_set_style_border_color(ui->name_field, lv_color_hex(ACCENT), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ui->name_field, 8, 0);
    lv_obj_add_event_cb(ui->name_field, name_focus, LV_EVENT_FOCUSED, ui);
    lv_obj_add_event_cb(ui->name_field, name_focus, LV_EVENT_CLICKED, ui);
}
static void update_icon_buttons(kitchen_ui_t *ui)
{
    for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) {
        lv_obj_t *obj = ui->icon_buttons[i];
        if (!obj) continue;
        lv_obj_set_style_border_width(obj, i == (unsigned)ui->draft_icon ? 2 : 0, 0);
        lv_obj_set_style_border_color(obj, lv_color_hex(ACCENT), 0);
    }
}
static void make_icons(kitchen_ui_t *ui, lv_obj_t *parent, int x, int y, int width)
{
    optional_heading(ui, parent, "Icon", x, y, width);
    unsigned columns = width < 500 ? 3 : 5;
    for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) {
        lv_obj_t *obj = button(ui, parent, icon_names[i], x + (i % columns) * (width / columns),
            y + 38 + (i / columns) * 112, width / columns - 8, 96, false, A_ICON, i);
        ui->icon_buttons[i] = obj;
        if (!obj) continue;
        lv_obj_set_style_pad_all(obj, 0, 0);
        lv_obj_t *glyph = lv_obj_get_child(obj, 0);
        const lv_font_t *font = &kitchen_symbols_48;
        if (glyph) {
            lv_obj_set_style_text_font(glyph, font, 0);
            lv_obj_set_height(glyph, font->line_height);
            lv_obj_center(glyph);
        }
        if (!i) {
            if (glyph) lv_obj_align(glyph, LV_ALIGN_CENTER, 0, -(kitchen_text_18.line_height + 6) / 2);
            lv_obj_t *caption = text(ui, obj, "No icon", 0, 0, width / columns - 20);
            if (caption) {
                lv_obj_set_style_text_font(caption, &kitchen_text_18, 0);
                lv_obj_set_height(caption, kitchen_text_18.line_height);
                lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_align(caption, LV_ALIGN_CENTER, 0, (font->line_height + 6) / 2);
            }
        }
    }
    update_icon_buttons(ui);
}
static void open_modal(kitchen_ui_t *ui)
{
    kitchen_timer_t *timer = selected(ui);
    if (!timer || timer->status == KITCHEN_DONE || ui->modal) return;
    ui->volume_gesture = false;
    ui->modal_binding_base = ui->binding_count;
    ui->modal = box(ui, ui->root, 0, 0, 1024, 600, 0x000000);
    if (!ui->modal) return;
    lv_obj_add_flag(ui->modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(ui->modal, LV_OPA_70, 0);
    lv_obj_t *inner = box(ui, ui->modal, 182, 90, 660, 390, BG);
    if (!inner) return;
    lv_obj_add_flag(inner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(inner, 18, 0);
    title(ui, inner, "Edit timer", 24, 20, 500);
    button(ui, inner, LV_SYMBOL_CLOSE, 586, 10, 52, 48, false, A_CANCEL_NAME, 0);
    ui->draft_icon = timer->icon;
    make_name(ui, inner, 24, 62, 612, timer->name);
    make_icons(ui, inner, 24, 172, 612);
    button(ui, inner, "Cancel", 300, 326, 150, 48, false, A_CANCEL_NAME, 0);
    button(ui, inner, "Save", 466, 326, 170, 48, true, A_SAVE_NAME, 0);
    bind_form_clicks(ui, ui->modal);
}
static void close_modal(kitchen_ui_t *ui)
{
    if (!ui->modal) return;
    lv_obj_del(ui->modal);
    ui->modal = ui->keyboard = ui->name_field = NULL;
    ui->volume_slider = ui->volume_value = NULL;
    ui->volume_gesture = false;
    memset(ui->icon_buttons, 0, sizeof(ui->icon_buttons));
    ui->binding_count = ui->modal_binding_base;
}
static void speaker_draw(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target(event);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    int x = (a.x1 + a.x2) / 2, y = (a.y1 + a.y2) / 2;
    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.bg_color = lv_color_hex(lv_obj_has_state(obj, LV_STATE_DISABLED) ? DISABLED_TEXT : LIGHT);
    fill.bg_opa = lv_obj_has_state(obj, LV_STATE_DISABLED) ? LV_OPA_40 : LV_OPA_COVER;
    lv_area_t base = {x - 17, y - 6, x - 9, y + 6};
    lv_draw_rect(lv_event_get_draw_ctx(event), &fill, &base);
    const lv_point_t cone[] = {{x - 10,y - 6},{x + 1,y - 14},{x + 1,y + 14},{x - 10,y + 6}};
    lv_draw_polygon(lv_event_get_draw_ctx(event), &fill, cone, 4);
    lv_draw_arc_dsc_t wave;
    lv_draw_arc_dsc_init(&wave);
    wave.color = fill.bg_color; wave.opa = fill.bg_opa; wave.width = 3; wave.rounded = true;
    lv_point_t center = {x + 1, y};
    lv_draw_arc(lv_event_get_draw_ctx(event), &wave, &center, 10, 310, 50);
    lv_draw_arc(lv_event_get_draw_ctx(event), &wave, &center, 17, 310, 50);
}
static void refresh_volume(kitchen_ui_t *ui)
{
    if (ui->volume_value) lv_label_set_text_fmt(ui->volume_value, "%u%%", (unsigned)ui->state->volume_percent);

}
static void volume_changed(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->active || !ui->audio_available) return;
    kitchen_command_t cmd = {.kind = KITCHEN_SET_VOLUME, .seconds = lv_slider_get_value(lv_event_get_target(event))};
    if (ui->command(ui->context, &cmd)) refresh_volume(ui);
    else lv_slider_set_value(lv_event_get_target(event), ui->state->volume_percent, LV_ANIM_OFF);
}
static void volume_released(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->active || ui->dirty || !ui->audio_available) return;
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) { ui->volume_gesture = true; return; }
    if (code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    if (!ui->volume_gesture) return;
    ui->volume_gesture = false;
    kitchen_command_t cmd = {.kind = KITCHEN_PREVIEW_VOLUME};
    (void)ui->command(ui->context, &cmd);
}
static void open_volume(kitchen_ui_t *ui)
{
    if (ui->modal || !ui->audio_available) return;
    ui->volume_gesture = false;
    ui->modal_binding_base = ui->binding_count;
    ui->modal = box(ui, ui->root, 0, 0, 1024, 600, 0x000000);
    if (!ui->modal) return;
    lv_obj_set_style_bg_opa(ui->modal, LV_OPA_70, 0);
    lv_obj_add_flag(ui->modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *inner = box(ui, ui->modal, 212, 144, 600, 312, PANEL);
    if (!inner) return;
    lv_obj_add_flag(inner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(inner, 18, 0);
    title(ui, inner, "Volume", 24, 20, 440);
    button(ui, inner, LV_SYMBOL_CLOSE, 524, 16, 52, 48, false, A_CLOSE_VOLUME, 0);
    ui->volume_value = text(ui, inner, "", 24, 86, 552);
    if (ui->volume_value) {
        lv_obj_set_style_text_font(ui->volume_value, &kitchen_text_24, 0);
        lv_obj_set_height(ui->volume_value, kitchen_text_24.line_height);
        lv_obj_set_style_text_align(ui->volume_value, LV_TEXT_ALIGN_CENTER, 0);
    }
    ui->volume_slider = lv_slider_create(inner);
    if (!ui->volume_slider) { ui->failed = true; return; }
    lv_obj_set_pos(ui->volume_slider, 40, 144);
    lv_obj_set_size(ui->volume_slider, 520, 24);
    lv_slider_set_range(ui->volume_slider, 0, 100);
    lv_slider_set_value(ui->volume_slider, ui->state->volume_percent, LV_ANIM_OFF);
    lv_obj_set_ext_click_area(ui->volume_slider, 16);
    lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(EDGE), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(LIGHT), LV_PART_KNOB);
    lv_obj_add_event_cb(ui->volume_slider, volume_changed, LV_EVENT_VALUE_CHANGED, ui);
    lv_obj_add_event_cb(ui->volume_slider, volume_released, LV_EVENT_ALL, ui);
    text(ui, inner, "0%", 24, 190, 260);
    lv_obj_t *maximum = text(ui, inner, "100%", 500, 190, 76);
    if (maximum) lv_obj_set_style_text_align(maximum, LV_TEXT_ALIGN_RIGHT, 0);
    button(ui, inner, "Done", 200, 236, 200, 52, true, A_CLOSE_VOLUME, 0);
    refresh_volume(ui);
    bind_form_clicks(ui, ui->modal);
}
static void build_list(kitchen_ui_t *ui)
{
    bool saved = ui->screen == SCREEN_SAVED;
    ui->content = box(ui, ui->page, 0, 64, 1024, 536, BG);
    if (!ui->content) return;
    /* Permit the larger halo outside the viewport. The fixed header covers it. */
    lv_obj_add_flag(ui->content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_scroll_dir(ui->content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui->content, LV_SCROLLBAR_MODE_AUTO);
    ui->drop_target = box(ui, ui->content, 0, 0, 190, 190, BG);
    if (ui->drop_target) {
        lv_obj_set_style_bg_opa(ui->drop_target, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ui->drop_target, 2, 0);
        lv_obj_set_style_border_color(ui->drop_target, lv_color_hex(ACCENT), 0);
        lv_obj_set_style_radius(ui->drop_target, LV_RADIUS_CIRCLE, 0);
        visible(ui->drop_target, false);
    }
    ui->empty = text(ui, ui->content, saved ? "No saved timers" : "No active timers. Tap New timer to begin.", 180, 192, 664);
    if (ui->empty) lv_obj_set_style_text_align(ui->empty, LV_TEXT_ALIGN_CENTER, 0);
    bool animate = !saved && ui->animate_active_entry && ui->layout_valid[SCREEN_ACTIVE];
    ui->animate_active_entry = false;
    sync_list(ui, animate);
    lv_obj_update_layout(ui->content);
    lv_obj_scroll_to_y(ui->content, ui->scroll[saved ? 1 : 0], LV_ANIM_OFF);
    lv_obj_t *header = box(ui, ui->page, 0, 0, 1024, 64, BG);
    if (!header) return;
    lv_obj_add_flag(header, LV_OBJ_FLAG_CLICKABLE);
    title(ui, header, saved ? "Saved timers" : "Timers", 24, 20, 380);
    if (saved) content_button(ui, header, LV_SYMBOL_UP " Active timers", 10, false, A_ACTIVE);
    else {
        ui->volume_button = button(ui, header, "", 940, 10, 64, 48, false, A_VOLUME, 0);
        if (ui->volume_button) lv_obj_add_event_cb(ui->volume_button, speaker_draw, LV_EVENT_DRAW_MAIN, ui);
        disabled(ui->volume_button, !ui->audio_available);
        refresh_volume(ui);
        ui->message = text(ui, header, "", 450, 28, 300);
        ui->new_button = content_button(ui, ui->page, LV_SYMBOL_PLUS " New timer", 542, true, A_NEW);
        if (ui->new_button) lv_obj_set_x(ui->new_button, 20);
        disabled(ui->new_button, ui->state->active_count + ui->state->saved_count >= KITCHEN_MAX_TIMERS);
        if (ui->state->active_count + ui->state->saved_count >= KITCHEN_MAX_TIMERS)
            message(ui, "20 timer limit reached");
        content_button(ui, ui->page, LV_SYMBOL_DOWN " Saved timers", 542, false, A_SAVED);
    }
}
static void build_new(kitchen_ui_t *ui)
{
    lv_obj_add_flag(ui->page, LV_OBJ_FLAG_CLICKABLE);
    button(ui, ui->page, LV_SYMBOL_LEFT, 16, 12, 52, 48, false, A_BACK, 0);
    title(ui, ui->page, "New timer", 88, 20, 500);
    lv_obj_t *duration_panel = box(ui, ui->page, 16, 84, 532, 500, PANEL);
    lv_obj_t *name_panel = box(ui, ui->page, 562, 84, 446, 414, PANEL);
    if (duration_panel) lv_obj_set_style_radius(duration_panel, 12, 0);
    if (name_panel) lv_obj_set_style_radius(name_panel, 12, 0);
    lv_obj_t *duration_title = text(ui, ui->page, "Duration", 36, 102, 460);
    if (duration_title) {
        lv_obj_set_style_text_font(duration_title, &kitchen_text_24, 0);
        lv_obj_set_height(duration_title, kitchen_text_24.line_height);
    }
    make_rollers(ui, ui->page, 36, 144, 150, 300);
    const unsigned presets[] = {60,180,300,600,900,1200,1800,2700,3600,7200};
    const char *names[] = {"1 min","3 min","5 min","10 min","15 min","20 min","30 min","45 min","1 hour","2 hours"};
    for (unsigned i = 0; i < 10; ++i) {
        ui->presets[i] = button(ui, ui->page, names[i], 28 + (i % 5) * 102, 416 + (i / 5) * 76, 94, 64, false, A_PRESET, presets[i]);
        if (ui->presets[i]) {
            lv_obj_set_style_bg_color(ui->presets[i], lv_color_hex(ACCENT), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(ui->presets[i], lv_color_hex(BG), LV_STATE_CHECKED);
            lv_obj_t *caption = lv_obj_get_child(ui->presets[i], 0);
            if (caption) lv_obj_set_style_text_font(caption, &kitchen_text_20, 0);
        }
    }
    ui->draft_icon = KITCHEN_ICON_NONE;
    make_name(ui, ui->page, 574, 102, 420, "");
    make_icons(ui, ui->page, 574, 236, 420);
    ui->cancel_button = button(ui, ui->page, "Cancel", 574, 516, 150, 64, false, A_BACK, 0);
    ui->apply = button(ui, ui->page, "Start timer", 740, 516, 254, 64, true, A_CREATE, 0);
    lv_obj_t *actions[] = {ui->cancel_button, ui->apply};
    for (unsigned i = 0; i < 2; ++i) {
        if (!actions[i]) continue;
        lv_obj_set_style_text_font(actions[i], &kitchen_text_24, 0);
        lv_obj_t *caption = lv_obj_get_child(actions[i], 0);
        if (caption) { lv_obj_set_height(caption, kitchen_text_24.line_height); lv_obj_center(caption); }
    }
    ui->message = text(ui, ui->page, "", 574, 208, 420);
    bind_form_clicks(ui, ui->page);
}
static void build_detail_panel(kitchen_ui_t *ui)
{
    kitchen_timer_t *timer = selected(ui);
    if (!timer || !ui->panel) return;
    if (ui->editor != EDIT_NONE) {
        text(ui, ui->panel, ui->editor == EDIT_REMAINING ? "Remaining time" : "Duration", 0, 4, 472);
        uint64_t ms = kitchen_core_remaining(timer, now(ui));
        uint32_t seconds = ui->editor == EDIT_BONUS ? 180 : (uint32_t)((ms + 999) / 1000);
        make_rollers(ui, ui->panel, 0, 64, 149, seconds);
        ui->message = text(ui, ui->panel, "", 0, 342, 472);
        ui->apply = button(ui, ui->panel, "Apply", 248, 400, 224, 76, true, A_APPLY, 0);
        button(ui, ui->panel, "Cancel", 0, 400, 232, 76, false, A_CANCEL_EDIT, 0);
        return;
    }
    bool done = timer->status == KITCHEN_DONE, saved = timer->status == KITCHEN_SAVED;
    int top = done ? 48 : 0;
    if (done) text(ui, ui->panel, "Add more time", 0, 0, 472);
    for (unsigned i = 0; i < 3; ++i) {
        char value[24];
        snprintf(value, sizeof(value), LV_SYMBOL_PLUS " %u min", (unsigned)(increments[i] / 60));
        ui->plus[i] = button(ui, ui->panel, value, i * 161, top, 150, done ? 112 : 76, false, A_ADJUST, increments[i]);
        if (!done) {
            snprintf(value, sizeof(value), LV_SYMBOL_MINUS " %u min", (unsigned)(increments[i] / 60));
            ui->minus[i] = button(ui, ui->panel, value, i * 161, top + 90, 150, 76, false, A_ADJUST, -increments[i]);
        }
    }
    button(ui, ui->panel, saved || done ? "Set duration" : "Set remaining time", 0, 184, 472, 72, false, A_SET, 0);
    const char *primary = done ? LV_SYMBOL_OK " Acknowledge" : saved ? LV_SYMBOL_PLAY " Start timer" :
        timer->status == KITCHEN_PAUSED ? LV_SYMBOL_PLAY " Resume" : LV_SYMBOL_PAUSE " Pause";
    int action = done ? A_ACK : saved ? A_START : timer->status == KITCHEN_PAUSED ? A_RESUME : A_PAUSE;
    button(ui, ui->panel, primary, 0, 274, 472, 84, true, action, 0);
    if (done) {
        char duration[32], value[72];
        if (timer->duration_s % 3600 == 0)
            snprintf(duration, sizeof(duration), "%u hour%s", (unsigned)(timer->duration_s / 3600), timer->duration_s == 3600 ? "" : "s");
        else if (timer->duration_s % 60 == 0)
            snprintf(duration, sizeof(duration), "%u min", (unsigned)(timer->duration_s / 60));
        else format_time(duration, sizeof(duration), timer->duration_s);
        snprintf(value, sizeof(value), KITCHEN_SYMBOL_REPLAY " Repeat · %s", duration);
        button(ui, ui->panel, value, 0, 376, 472, 72, false, A_REPEAT, 0);
    } else button(ui, ui->panel, saved ? LV_SYMBOL_TRASH " Remove timer" : LV_SYMBOL_CLOSE " Cancel timer",
        0, 376, 472, 72, false, saved ? A_REMOVE : A_CANCEL_TIMER, 0);
    ui->message = text(ui, ui->panel, "", 0, 466, 472);
}
static void build_detail(kitchen_ui_t *ui)
{
    kitchen_timer_t *timer = selected(ui);
    if (!timer) { go(ui, SCREEN_ACTIVE); return; }
    ui->detail_status = timer->status;
    prepare_halo(ui, 448);
    button(ui, ui->page, LV_SYMBOL_LEFT, 16, 12, 52, 48, false, A_BACK, 0);
    title(ui, ui->page, timer->name[0] ? timer->name : "Timer", 88, 20, 760);
    if (timer->status != KITCHEN_DONE) ui->edit_button = button(ui, ui->page, LV_SYMBOL_EDIT, 948, 12, 56, 48, false, A_EDIT, 0);
    if (!make_card(ui, &ui->detail, ui->page, timer->id, false)) { ui->failed = true; return; }
    position_card(&ui->detail, 24, 104, 448);
    ui->panel = box(ui, ui->page, 528, 88, 472, 500, BG);
    if (ui->panel) lv_obj_set_style_text_font(ui->panel, &kitchen_text_24, 0);
    build_detail_panel(ui);
    refresh_cards(ui);
}
static void clear_page(kitchen_ui_t *ui)
{
    stop_cards(ui);
    release_halo(ui);
    close_modal(ui);
    if (ui->page) lv_obj_del(ui->page);
    ui->page = ui->content = ui->panel = ui->keyboard = ui->name_field = NULL;
    ui->volume_button = ui->volume_slider = ui->volume_value = NULL;
    ui->hours = ui->minutes = ui->seconds = ui->apply = ui->message = ui->new_button = ui->empty = ui->edit_button = ui->drop_target = ui->cancel_button = NULL;
    memset(ui->cards, 0, sizeof(ui->cards));
    memset(&ui->detail, 0, sizeof(ui->detail));
    memset(ui->minus, 0, sizeof(ui->minus));
    memset(ui->plus, 0, sizeof(ui->plus));
    memset(ui->presets, 0, sizeof(ui->presets));
    memset(ui->icon_buttons, 0, sizeof(ui->icon_buttons));
    ui->binding_count = 0;
    ui->drag_id = ui->pressed_id = 0;
}
static void rebuild(kitchen_ui_t *ui)
{
    clear_page(ui);
    ui->failed = false;
    ui->dirty = false;
    ui->page = box(ui, ui->root, 0, 0, 1024, 600, BG);
    if (!ui->page) return;
    if (ui->screen <= SCREEN_SAVED) build_list(ui);
    else if (ui->screen == SCREEN_NEW) build_new(ui);
    else build_detail(ui);
}
static void recover_page(kitchen_ui_t *ui)
{
    clear_page(ui);
    ui->failed = false;
    ui->dirty = false;
    ui->screen = SCREEN_ERROR;
    ui->page = box(ui, ui->root, 0, 0, 1024, 600, BG);
    if (!ui->page) return;
    text(ui, ui->page, "Unable to create screen. Timers are retained in memory.", 160, 200, 740);
    button(ui, ui->page, "Back to timers", 362, 290, 300, 54, true, A_ACTIVE, 0);
}
static void action_event(lv_event_t *event)
{
    /* Copy before a modal close can recycle its binding slot. */
    kitchen_binding_t binding = *(kitchen_binding_t *)lv_event_get_user_data(event);
    kitchen_ui_t *ui = binding.ui;
    if (!ui->active || ui->dirty || ui->failed) return;
    kitchen_core_update(ui->state, now(ui));
    kitchen_timer_t *timer = selected(ui);
    bool changed = false;
    switch (binding.action) {
    case A_BACK:
        if (ui->modal) { close_modal(ui); break; }
        if (ui->editor != EDIT_NONE) { ui->editor = EDIT_NONE; ui->dirty = true; break; }
        if (ui->screen == SCREEN_ACTIVE) ui->back(ui->context);
        else go(ui, ui->screen == SCREEN_DETAIL ? ui->return_screen : SCREEN_ACTIVE);
        break;
    case A_NEW:
        if (ui->state->active_count + ui->state->saved_count < KITCHEN_MAX_TIMERS) go(ui, SCREEN_NEW);
        break;
    case A_SAVED: go(ui, SCREEN_SAVED); break;
    case A_ACTIVE: go(ui, SCREEN_ACTIVE); break;
    case A_PAUSE: changed = send(ui, KITCHEN_PAUSE, 0); break;
    case A_RESUME: changed = send(ui, KITCHEN_RESUME, 0); break;
    case A_ACK: if (send(ui, KITCHEN_ACKNOWLEDGE, 0)) go(ui, SCREEN_ACTIVE); break;
    case A_CANCEL_TIMER: if (send(ui, KITCHEN_CANCEL, 0)) go(ui, SCREEN_ACTIVE); break;
    case A_START: if (send(ui, KITCHEN_START, 0)) go(ui, SCREEN_ACTIVE); break;
    case A_REMOVE: if (send(ui, KITCHEN_REMOVE, 0)) go(ui, SCREEN_SAVED); break;
    case A_REPEAT: changed = send(ui, KITCHEN_REPEAT, 0); break;
    case A_ADJUST:
        if (timer) changed = send(ui, timer->status == KITCHEN_DONE ? KITCHEN_EXTEND : KITCHEN_ADJUST, binding.value);
        break;
    case A_SET:
        if (!timer) break;
        ui->completion_shown = false;
        ui->editor = timer->status == KITCHEN_DONE ? EDIT_BONUS :
            timer->status == KITCHEN_SAVED ? EDIT_DURATION : EDIT_REMAINING;
        ui->dirty = true;
        break;
    case A_APPLY: {
        uint32_t seconds = rollers_value(ui);
        if (!timer || !seconds) break;
        /* Completion must be displayed before this action can start a new countdown. */
        if (timer->status == KITCHEN_DONE && ui->editor == EDIT_REMAINING && !ui->completion_shown) {
            ui->completion_shown = true;
            message(ui, "Timer finished. Tap Start timer.");
            lv_label_set_text(lv_obj_get_child(ui->apply, 0), "Start timer");
            break;
        }
        kitchen_command_kind_t kind = timer->status == KITCHEN_DONE ? KITCHEN_EXTEND : KITCHEN_SET_TIME;
        if (send(ui, kind, (int)seconds)) { ui->editor = EDIT_NONE; ui->dirty = true; }
        break;
    }
    case A_CANCEL_EDIT: ui->editor = EDIT_NONE; ui->dirty = true; break;
    case A_PRESET: rollers_set(ui, binding.value); break;
    case A_CREATE: {
        if (!ui->name_field) break;
        kitchen_command_t cmd = {.kind = KITCHEN_CREATE, .seconds = (int32_t)rollers_value(ui),
            .name = lv_textarea_get_text(ui->name_field), .icon = ui->draft_icon};
        if (ui->command(ui->context, &cmd)) {
            ui->animate_active_entry = true;
            go(ui, SCREEN_ACTIVE);
        }
        else message(ui, "Choose a duration. Maximum: 20 timers.");
        break;
    }
    case A_EDIT: open_modal(ui); break;
    case A_SAVE_NAME: {
        if (!ui->name_field) break;
        kitchen_command_t cmd = {.kind = KITCHEN_EDIT, .id = ui->selected,
            .name = lv_textarea_get_text(ui->name_field), .icon = ui->draft_icon};
        if (ui->command(ui->context, &cmd)) { close_modal(ui); ui->dirty = true; }
        else if (timer && timer->status == KITCHEN_DONE) { close_modal(ui); ui->dirty = true; }
        break;
    }
    case A_CANCEL_NAME: close_modal(ui); break;
    case A_ICON: ui->draft_icon = (kitchen_icon_t)binding.value; update_icon_buttons(ui); break;
    case A_VOLUME: open_volume(ui); break;
    case A_CLOSE_VOLUME: close_modal(ui); break;
    }
    if (changed) {
        if (ui->screen == SCREEN_DETAIL && ui->editor == EDIT_NONE) ui->dirty = true;
        else ui->list_dirty = true;
    }
}
static void card_action(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    kitchen_ui_t *ui = card->ui;
    if (!ui->active || ui->dirty || !card->present || ui->drag_id) return;
    kitchen_core_update(ui->state, now(ui));
    kitchen_timer_t *timer = kitchen_core_find(ui->state, card->id);
    if (!timer) return;
    kitchen_command_kind_t kind = timer->status == KITCHEN_RUNNING ? KITCHEN_PAUSE :
        timer->status == KITCHEN_PAUSED ? KITCHEN_RESUME : timer->status == KITCHEN_DONE ? KITCHEN_ACKNOWLEDGE : KITCHEN_START;
    kitchen_command_t cmd = {.kind = kind, .id = timer->id};
    if (ui->command(ui->context, &cmd)) {
        if (kind == KITCHEN_START) {
            ui->animate_active_entry = true;
            go(ui, SCREEN_ACTIVE);
        }
        else if (kind == KITCHEN_ACKNOWLEDGE) ui->list_dirty = true;
        else refresh_cards(ui);
    }
}
static int distance_squared(int x, int y) { return x * x + y * y; }
static void drag_update(kitchen_ui_t *ui)
{
    if (!ui->drag_id || !ui->content) return;
    kitchen_card_t *moving = NULL;
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i)
        if (ui->cards[i].present && ui->cards[i].id == ui->drag_id) moving = &ui->cards[i];
    if (!moving) return;
    lv_area_t bounds;
    lv_obj_get_coords(ui->content, &bounds);
    if (ui->drag_point.y < bounds.y1 + 40) lv_obj_scroll_by(ui->content, 0, 12, LV_ANIM_OFF);
    else if (ui->drag_point.y > bounds.y2 - 40) lv_obj_scroll_by(ui->content, 0, -12, LV_ANIM_OFF);
    int x = ui->drag_point.x - bounds.x1 - moving->current_size / 2;
    int y = ui->drag_point.y - bounds.y1 + lv_obj_get_scroll_y(ui->content) - moving->current_size / 2;
    moving->current_x = x; moving->current_y = y;
    lv_obj_set_pos(moving->object, x, y);
    uint8_t count = ui->screen == SCREEN_SAVED ? ui->state->saved_count : ui->state->active_count;
    uint32_t *order = ui->preview_order;
    unsigned best_index = 0;
    int best = INT32_MAX;
    for (unsigned i = 0; i < count; ++i) {
        int tx, ty, size;
        layout_position(count, i, &tx, &ty, &size);
        int distance = distance_squared(x + moving->current_size / 2 - tx - size / 2,
            y + moving->current_size / 2 - ty - size / 2);
        if (distance < best) { best = distance; best_index = i; }
    }
    if (order[best_index] != ui->drag_id) {
        unsigned old_index = 0;
        while (old_index < count && order[old_index] != ui->drag_id) ++old_index;
        if (old_index < count) {
            if (old_index < best_index) memmove(order + old_index, order + old_index + 1,
                (best_index - old_index) * sizeof(*order));
            else memmove(order + best_index + 1, order + best_index,
                (old_index - best_index) * sizeof(*order));
            order[best_index] = ui->drag_id;
            sync_list(ui, true);
        }
    }
    if (ui->drop_target) {
        int tx, ty, size;
        layout_position(count, best_index, &tx, &ty, &size);
        lv_obj_set_pos(ui->drop_target, tx, ty);
        lv_obj_set_size(ui->drop_target, size, size);
        visible(ui->drop_target, true);
    }
    lv_obj_set_style_opa(moving->object, LV_OPA_80, 0);
}
static void card_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_PRESS_LOST &&
        code != LV_EVENT_RELEASED && code != LV_EVENT_CLICKED) return;
    kitchen_card_t *card = lv_event_get_user_data(event);
    kitchen_ui_t *ui = card->ui;
    if (!ui || !ui->active || ui->dirty || !card->present) return;
    lv_indev_t *input = lv_indev_get_act();
    if (code == LV_EVENT_PRESSED && input) {
        ui->pressed_id = card->id;
        ui->press_tick = lv_tick_get();
        lv_indev_get_point(input, &ui->press_point);
        ui->drag_point = ui->press_point;
        ui->press_moved = ui->dragged = false;
    } else if (code == LV_EVENT_PRESSING && input && ui->pressed_id == card->id) {
        lv_indev_get_point(input, &ui->drag_point);
        if (!ui->drag_id && distance_squared(ui->drag_point.x - ui->press_point.x,
            ui->drag_point.y - ui->press_point.y) > 100) ui->press_moved = true;
        if (ui->drag_id) drag_update(ui);
    } else if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_RELEASED) {
        if (ui->drag_id) {
            if (code == LV_EVENT_RELEASED) {
                unsigned count = ui->screen == SCREEN_SAVED ? ui->state->saved_count : ui->state->active_count;
                for (unsigned i = 0; i < count; ++i) {
                    if (ui->preview_order[i] != ui->drag_id) continue;
                    kitchen_command_t cmd = {.kind = KITCHEN_MOVE, .id = ui->drag_id, .position = i};
                    ui->command(ui->context, &cmd);
                    break;
                }
            }
            ui->drag_id = 0;
            visible(ui->drop_target, false);
            lv_obj_set_scroll_dir(ui->content, LV_DIR_VER);
            lv_obj_set_style_opa(card->object, LV_OPA_COVER, 0);
            sync_list(ui, true);
        }
        ui->pressed_id = 0;
    } else if (code == LV_EVENT_CLICKED && !ui->dragged && !ui->press_moved) {
        ui->selected = card->id;
        ui->return_screen = ui->screen;
        go(ui, SCREEN_DETAIL);
    }
}
static void root_deleted(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (lv_event_get_target(event) != ui->root) return;
    stop_cards(ui);
    release_halo(ui);
    if (ui->tick) lv_timer_del(ui->tick);
    ui->tick = NULL;
    ui->root = NULL;
    ui->page = ui->content = ui->panel = ui->modal = ui->keyboard = ui->name_field = NULL;
    ui->volume_button = ui->volume_slider = ui->volume_value = NULL;
    ui->hours = ui->minutes = ui->seconds = ui->apply = ui->message = ui->new_button = ui->empty = ui->edit_button = ui->drop_target = ui->cancel_button = NULL;
    memset(ui->cards, 0, sizeof(ui->cards));
    memset(&ui->detail, 0, sizeof(ui->detail));
    memset(ui->minus, 0, sizeof(ui->minus));
    memset(ui->plus, 0, sizeof(ui->plus));
    memset(ui->presets, 0, sizeof(ui->presets));
    memset(ui->icon_buttons, 0, sizeof(ui->icon_buttons));
    ui->binding_count = 0;
    ui->active = false;
    ++ui->generation;
}
static void tick(lv_timer_t *timer)
{
    kitchen_ui_t *ui = timer->user_data;
    if (!ui->active || !ui->root) return;
    kitchen_core_update(ui->state, now(ui));
    if (ui->dirty) rebuild(ui);
    if (ui->failed) {
        if (ui->screen != SCREEN_ERROR) recover_page(ui);
        return;
    }
    if (ui->screen <= SCREEN_SAVED && ui->list_dirty) sync_list(ui, true);
    if (ui->failed) { recover_page(ui); return; }
    if (ui->pressed_id && !ui->drag_id && !ui->press_moved &&
        lv_tick_elaps(ui->press_tick) >= 500 && ui->content &&
        !lv_obj_is_scrolling(ui->content)) {
        unsigned count = ui->screen == SCREEN_SAVED ? ui->state->saved_count : ui->state->active_count;
        const uint32_t *order = ui->screen == SCREEN_SAVED ? ui->state->saved : ui->state->active;
        memcpy(ui->preview_order, order, count * sizeof(*order));
        ui->drag_id = ui->pressed_id;
        ui->dragged = true;
        lv_obj_set_scroll_dir(ui->content, LV_DIR_NONE);
        for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
            kitchen_card_t *card = &ui->cards[i];
            if (card->present && card->id == ui->drag_id) {
                lv_anim_del(card, NULL);
                lv_obj_move_foreground(card->object);
                lv_obj_set_style_opa(card->object, LV_OPA_80, 0);
            }
        }
    }
    if (ui->drag_id) drag_update(ui);
    if (ui->screen == SCREEN_DETAIL) {
        kitchen_timer_t *item = selected(ui);
        if (!item) { go(ui, SCREEN_ACTIVE); return; }
        if (item->status != ui->detail_status) {
            close_modal(ui);
            if (ui->editor == EDIT_NONE) { ui->dirty = true; return; }
            /* Keep the user's roller selection when the countdown finishes. */
            ui->detail_status = item->status;
            visible(ui->edit_button, item->status != KITCHEN_DONE);
            if (item->status == KITCHEN_DONE && ui->apply) {
                ui->completion_shown = true;
                lv_label_set_text(lv_obj_get_child(ui->apply, 0), "Start timer");
                message(ui, "Timer finished. Tap Start timer.");
            }
        }
        uint64_t remaining = kitchen_core_remaining(item, now(ui));
        for (unsigned i = 0; i < 3; ++i) {
            disabled(ui->minus[i], remaining <= (uint64_t)increments[i] * 1000);
            disabled(ui->plus[i], item->status != KITCHEN_DONE &&
                remaining + (uint64_t)increments[i] * 1000 > (uint64_t)KITCHEN_MAX_SECONDS * 1000);
        }
    }
    if (ui->screen == SCREEN_NEW) {
        static const uint32_t presets[] = {60,180,300,600,900,1200,1800,2700,3600,7200};
        uint32_t seconds = rollers_value(ui);
        for (unsigned i = 0; i < 10; ++i) {
            if (!ui->presets[i]) continue;
            if (seconds == presets[i]) lv_obj_add_state(ui->presets[i], LV_STATE_CHECKED);
            else lv_obj_clear_state(ui->presets[i], LV_STATE_CHECKED);
        }
    }
    if (ui->apply) disabled(ui->apply, rollers_value(ui) == 0);
    refresh_cards(ui);
}
void kitchen_ui_refresh(kitchen_ui_t *ui)
{
    if (ui->root && ui->active) refresh_cards(ui);
}
void kitchen_ui_set_active(kitchen_ui_t *ui, bool active)
{
    active = active && ui->root;
    if (ui->active == active) return;
    ui->active = active;
    visible(ui->root, active);
    if (!active) {
        stop_cards(ui);
        ui->pressed_id = ui->drag_id = 0;
        visible(ui->drop_target, false);
        if (ui->content) lv_obj_set_scroll_dir(ui->content, LV_DIR_VER);
        if (ui->tick) lv_timer_pause(ui->tick);
        return;
    }
    kitchen_core_update(ui->state, now(ui));
    if (ui->screen <= SCREEN_SAVED && ui->content && !ui->failed) sync_list(ui, false);
    if (ui->tick) { lv_timer_resume(ui->tick); lv_timer_ready(ui->tick); }
}
void kitchen_ui_destroy(kitchen_ui_t *ui)
{
    stop_cards(ui);
    if (ui->root) lv_obj_del(ui->root);
    if (ui->tick) { lv_timer_del(ui->tick); ui->tick = NULL; }
    ui->active = false;
}
esp_err_t kitchen_ui_create(kitchen_ui_t *ui, lv_obj_t *parent, kitchen_state_t *state,
    bool (*command)(void *, const kitchen_command_t *), uint64_t (*now_ms)(void *),
    void (*back)(void *), bool audio_available, void *context)
{
    if (!ui || !parent || !state || !command || !now_ms || !back) return ESP_ERR_INVALID_ARG;
    kitchen_ui_destroy(ui);
    uint32_t generation = ui->generation + 1;
    memset(ui, 0, sizeof(*ui));
    ui->generation = generation;
    ui->audio_available = audio_available;
    ui->state = state; ui->command = command; ui->now_ms = now_ms; ui->back = back; ui->context = context;
    ui->root = box(ui, parent, 0, 0, 1024, 600, BG);
    if (!ui->root) return ESP_ERR_NO_MEM;
    lv_obj_set_style_text_color(ui->root, lv_color_hex(LIGHT), 0);
    lv_obj_set_style_text_font(ui->root, &kitchen_text_20, 0);
    lv_obj_add_event_cb(ui->root, root_deleted, LV_EVENT_DELETE, ui);
    kitchen_core_update(state, now(ui));
    rebuild(ui);
    ui->tick = lv_timer_create(tick, 50, ui);
    if (!ui->tick || ui->failed) { kitchen_ui_destroy(ui); return ESP_ERR_NO_MEM; }
    lv_timer_pause(ui->tick);
    visible(ui->root, false);
    return ESP_OK;
}
