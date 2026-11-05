#include "kitchen_ui.h"
#include "kitchen_symbols.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(kitchen_font_36)
LV_FONT_DECLARE(kitchen_font_48)
LV_FONT_DECLARE(kitchen_font_56)
LV_FONT_DECLARE(kitchen_font_64)
LV_FONT_DECLARE(kitchen_font_72)
LV_FONT_DECLARE(kitchen_font_112)
LV_FONT_DECLARE(kitchen_text_18)
LV_FONT_DECLARE(kitchen_text_20)
LV_FONT_DECLARE(kitchen_text_24)
LV_FONT_DECLARE(kitchen_text_32)
LV_FONT_DECLARE(kitchen_lvgl_symbols_32)
LV_FONT_DECLARE(kitchen_symbols_40)
LV_FONT_DECLARE(kitchen_symbols_48)
LV_FONT_DECLARE(kitchen_symbols_64)
LV_FONT_DECLARE(kitchen_symbols_56)
LV_FONT_DECLARE(kitchen_settings_36)
LV_FONT_DECLARE(kitchen_status_18)
LV_FONT_DECLARE(kitchen_status_20)
LV_FONT_DECLARE(kitchen_status_24)
LV_FONT_DECLARE(kitchen_symbols_88)
LV_FONT_DECLARE(kitchen_controls_48)
LV_FONT_DECLARE(kitchen_settings_52)
LV_FONT_DECLARE(kitchen_status_36)
LV_FONT_DECLARE(kitchen_actions_20)

#define SETTINGS_SYMBOL "\xee\xa2\xb8" /* Material Symbols settings U+E8B8 */
#define BG 0x111113
#define BUTTON 0x29292c
#define ACCENT 0xffb547
#define LIGHT 0xf4f4f5
#define MUTED 0xa1a1a6
#define PANEL 0x1b1b1e
#define FIELD 0x242427
#define EDGE 0x3b3b40
#define OVERLAY_W 600
#define OVERLAY_H 460
#define PRESET_COUNT 6
#define OVERLAY_X ((1024 - OVERLAY_W) / 2)
#define OVERLAY_Y ((600 - OVERLAY_H) / 2)
#define BODY_W 552

enum { MODAL_NONE, MODAL_TIMER, MODAL_VOLUME };
enum { EDIT_NONE, EDIT_SETUP, EDIT_DURATION, EDIT_REMAINING, EDIT_BONUS };
enum { A_CARD, A_CLOSE, A_PRIMARY, A_CANCEL_TIMER, A_MINUS, A_PLUS, A_REPEAT,
    A_TIME, A_APPLY, A_CANCEL_EDIT, A_PRESET, A_NAME, A_ICON_TOGGLE, A_ICON,
    A_VOLUME, A_CLOSE_VOLUME, A_RETRY, A_NAME_APPLY, A_NAME_CANCEL };
static const uint32_t palette[] = {ACCENT, 0x2dd4bf, 0x60a5fa, 0xa78bfa, 0xf472b6};
static const char *icon_names[] = {KITCHEN_SYMBOL_BLOCK, KITCHEN_SYMBOL_LOCAL_CAFE,
    KITCHEN_SYMBOL_STOCKPOT, KITCHEN_SYMBOL_SKILLET, KITCHEN_SYMBOL_OVEN};
static const uint32_t presets[] = {60, 180, 300, 600, 900, 1800};
static const int geometry[KITCHEN_MAX_TIMERS][4] = {
    {12,76,392,512}, {416,76,292,292}, {720,76,292,292},
    {416,380,190,208}, {618,380,191,208}, {821,380,191,208}
};
static void action_event(lv_event_t *event);
static void refresh(kitchen_ui_t *ui);
static void refresh_overlay(kitchen_ui_t *ui);
static void close_modal(kitchen_ui_t *ui);
static void open_timer(kitchen_ui_t *ui, uint32_t id);
static void set_editor(kitchen_ui_t *ui, int editor, uint32_t seconds);
static void toggle_icons(kitchen_ui_t *ui, bool open);
static void finish_name(kitchen_ui_t *ui, bool apply);
static void add_no_icon(kitchen_ui_t *ui, lv_obj_t *obj);
static void layout_fields(kitchen_ui_t *ui);
static uint64_t now(kitchen_ui_t *ui) { return ui->now_ms(ui->context); }
static kitchen_timer_t *selected(kitchen_ui_t *ui) { return kitchen_core_find(ui->state, ui->selected); }
static void visible(lv_obj_t *obj, bool show)
{
    if (!obj) return;
    if (show) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
static void disabled(lv_obj_t *obj, bool value)
{
    if (!obj) return;
    if (value) lv_obj_add_state(obj, LV_STATE_DISABLED);
    else lv_obj_clear_state(obj, LV_STATE_DISABLED);
}
static void label_value(lv_obj_t *obj, const char *value)
{
    if (obj && strcmp(lv_label_get_text(obj), value)) lv_label_set_text(obj, value);
}
static void caption(lv_obj_t *obj, const char *value)
{
    if (obj) {
        lv_obj_t *label = lv_obj_get_child(obj, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
        label_value(label, value);
    }
}
static lv_obj_t *box(kitchen_ui_t *ui, lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    if (!parent || ui->failed) { ui->failed = true; return NULL; }
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
static lv_obj_t *text(kitchen_ui_t *ui, lv_obj_t *parent, const char *value,
    int x, int y, int w, int h, const lv_font_t *font)
{
    if (!parent || ui->failed) { ui->failed = true; return NULL; }
    lv_obj_t *obj = lv_label_create(parent);
    if (!obj) { ui->failed = true; return NULL; }
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, value);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
static lv_obj_t *button(kitchen_ui_t *ui, lv_obj_t *parent, const char *value,
    int x, int y, int w, int h, bool accent, int action, int parameter)
{
    if (!parent || ui->failed || ui->binding_count >= sizeof(ui->bindings) / sizeof(ui->bindings[0])) {
        ui->failed = true; return NULL;
    }
    lv_obj_t *obj = lv_btn_create(parent);
    if (!obj) { ui->failed = true; return NULL; }
    if (action == A_ICON) {
        /* LVGL 8.4 enumerates every style transition and dereferences its
         * descriptor. Remove the theme (including grow/transitions), rather
         * than trying to disable transitions with a NULL descriptor. */
        lv_obj_remove_style_all(obj);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    }
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(accent ? ACCENT : BUTTON), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(accent ? BG : LIGHT), 0);
    lv_obj_set_style_color_filter_opa(obj, LV_OPA_TRANSP, LV_STATE_DISABLED);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x414144), LV_STATE_DISABLED);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xa1a1a6), LV_STATE_DISABLED);
    lv_obj_set_style_text_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    lv_obj_t *label = text(ui, obj, value, 0, 0, w - 12, kitchen_text_20.line_height, &kitchen_text_20);
    if (label) { lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP); lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0); lv_obj_center(label); }
    kitchen_binding_t *binding = &ui->bindings[ui->binding_count++];
    *binding = (kitchen_binding_t){ui, action, parameter};
    lv_obj_add_event_cb(obj, action_event, LV_EVENT_CLICKED, binding);
    return obj;
}
static bool send(kitchen_ui_t *ui, kitchen_command_kind_t kind, int seconds)
{
    kitchen_command_t cmd = {.kind = kind, .id = ui->selected, .seconds = seconds};
    return ui->command(ui->context, &cmd);
}
static void format_time(char *buffer, size_t size, uint64_t seconds)
{
    if (seconds >= 3600) snprintf(buffer, size, "%llu:%02u:%02u", (unsigned long long)(seconds / 3600),
        (unsigned)(seconds / 60 % 60), (unsigned)(seconds % 60));
    else snprintf(buffer, size, "%02u:%02u", (unsigned)(seconds / 60), (unsigned)(seconds % 60));
}
static void time_value(lv_obj_t *label, uint64_t seconds)
{
    if (!label) return;
    char value[32];
    format_time(value, sizeof(value), seconds);
    if (!strcmp(lv_label_get_text(label), value)) return;
    const lv_font_t *fonts[] = {&kitchen_font_112, &kitchen_font_72, &kitchen_font_64,
        &kitchen_font_56, &kitchen_font_48, &kitchen_font_36, &kitchen_text_32,
        &kitchen_text_24, &kitchen_text_20, &kitchen_text_18};
    const lv_font_t *font = &kitchen_text_18;
    bool fits = false;
    for (unsigned i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) {
        lv_point_t extent;
        lv_txt_get_size(&extent, value, fonts[i], 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (extent.x <= lv_obj_get_width(label) && extent.y <= lv_obj_get_height(label)) { font = fonts[i]; fits = true; break; }
    }
    /* Extremely long DONE hours still remain readable without wrapping at a
     * day boundary or truncating digits. This never affects selectable times. */
    if (!fits) {
        char *split = strchr(value, ':');
        if (split && strlen(value) + 1 < sizeof(value)) {
            ++split;
            memmove(split + 1, split, strlen(split) + 1);
            *split = '\n';
        }
    }
    if (!strcmp(lv_label_get_text(label), value)) return;
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, value);
}
static const char *primary_text(kitchen_status_t status, bool compact)
{
    (void)compact;
    switch (status) {
    case KITCHEN_EMPTY: return "Set up";
    case KITCHEN_READY: return "Start timer";
    case KITCHEN_RUNNING: return "Pause";
    case KITCHEN_PAUSED: return "Resume";
    case KITCHEN_DONE: return "OK";
    default: return "";
    }
}
static int fit_button(lv_obj_t *obj, int maximum)
{
    if (!obj) return 0;
    lv_obj_t *label = lv_obj_get_child(obj, 0);
    lv_point_t extent;
    lv_txt_get_size(&extent, lv_label_get_text(label), lv_obj_get_style_text_font(label, 0),
        0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int width = extent.x + 28;
    if (width < 48) width = 48;
    if (width > maximum) width = maximum;
    lv_obj_set_width(obj, width);
    lv_obj_set_width(label, width - 20);
    lv_obj_center(label);
    return width;
}
static void center_button(lv_obj_t *obj, int width)
{
    if (obj) lv_obj_set_x(obj, (width - fit_button(obj, width)) / 2);
}
static void pair_buttons(lv_obj_t *left, lv_obj_t *right, int width)
{
    if (!left || !right) return;
    int a = fit_button(left, (width - 16) / 2), b = fit_button(right, (width - 16) / 2);
    int x = (width - a - b - 16) / 2;
    lv_obj_set_x(left, x);
    lv_obj_set_x(right, x + a + 16);
}
static void primary_caption(lv_obj_t *obj, kitchen_status_t status, int available, bool small)
{
    if (!obj) return;
    lv_obj_t *label = lv_obj_get_child(obj, 0), *glyph = lv_obj_get_child(obj, 1);
    if (!glyph) return;
    int height = lv_obj_get_style_height(obj, 0);
    bool big = height >= 72, medium = height >= 56 && !big;
    const lv_font_t *font = big ? &kitchen_text_32 : medium ? &kitchen_text_24 : small ? &kitchen_text_18 : &kitchen_actions_20;
    int padding = big ? 16 : medium ? 14 : 12;
    int icon_slot = big ? 60 : medium ? 44 : 36;
    int gap = big ? 12 : 8;
    caption(obj, primary_text(status, false));
    lv_obj_set_style_text_font(label, font, 0);
    lv_point_t extent;
    lv_txt_get_size(&extent, lv_label_get_text(label), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    int text_x = padding + icon_slot + gap;
    int width = text_x + extent.x + padding;
    int safe_inset = available * 55 / 1000;
    if (safe_inset < 16) safe_inset = 16;
    if (width > available - 2 * safe_inset) width = available - 2 * safe_inset;
    lv_obj_set_width(obj, width);
    lv_obj_set_width(label, width - text_x - padding);
    lv_obj_set_height(label, font->line_height);
    lv_obj_set_style_align(label, LV_ALIGN_TOP_LEFT, 0);
    lv_obj_set_pos(label, text_x, (height - font->line_height) / 2);
    const char *symbol = status == KITCHEN_EMPTY ? SETTINGS_SYMBOL : status == KITCHEN_RUNNING ? LV_SYMBOL_PAUSE :
        status == KITCHEN_DONE ? LV_SYMBOL_OK : LV_SYMBOL_PLAY;
    const lv_font_t *symbol_font = status == KITCHEN_EMPTY ?
        (big ? &kitchen_settings_52 : &kitchen_settings_36) : (big ? &kitchen_controls_48 : &kitchen_lvgl_symbols_32);
    lv_label_set_long_mode(glyph, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(glyph, symbol_font, 0);
    lv_txt_get_size(&extent, symbol, symbol_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_obj_set_size(glyph, extent.x, extent.y);
    lv_obj_set_style_align(glyph, LV_ALIGN_TOP_LEFT, 0);
    lv_obj_set_pos(glyph, padding + (icon_slot - extent.x) / 2, (height - extent.y) / 2);
    label_value(glyph, symbol);
    lv_obj_set_style_text_opa(glyph, LV_OPA_70, 0);
    lv_obj_set_x(obj, (available - width) / 2);
}
static void make_primary_icon(kitchen_ui_t *ui, lv_obj_t *obj)
{
    if (obj) {
        lv_obj_t *glyph = text(ui, obj, "", 12, 8, 32, 32, &kitchen_lvgl_symbols_32);
        if (glyph) lv_label_set_long_mode(glyph, LV_LABEL_LONG_CLIP);
    }
}
static bool primary(kitchen_ui_t *ui, kitchen_status_t shown)
{
    if (shown == KITCHEN_EMPTY) { open_timer(ui, ui->selected); return false; }
    kitchen_command_kind_t kind = shown == KITCHEN_READY ? KITCHEN_START :
        shown == KITCHEN_RUNNING ? KITCHEN_PAUSE : shown == KITCHEN_PAUSED ? KITCHEN_RESUME : KITCHEN_ACKNOWLEDGE;
    return send(ui, kind, 0);
}
/* Draw only inset outlines. No shadow, transform layer, per-card bitmap or
 * external draw area. The card's children remain above this inner edge. */
static void glow_draw(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    lv_area_t area;
    lv_obj_get_coords(card->object, &area);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_color = lv_color_hex(palette[(card->id - 1) % 5]);
    dsc.border_width = card->border_width;
    dsc.border_opa = (lv_opa_t)card->border_opa;
    int radius = card->radius ? card->radius : 18;
    dsc.radius = radius;
    lv_draw_rect(lv_event_get_draw_ctx(event), &dsc, &area);
    if (!card->glow || !card->glow_width) return;
    dsc.border_width = 1;
    for (int i = 1; i <= card->glow_width; ++i) {
        ++area.x1; ++area.y1; --area.x2; --area.y2;
        dsc.radius = radius - i > 0 ? radius - i : 0;
        int fade = card->glow_width - i;
        dsc.border_opa = (lv_opa_t)(card->glow * fade / card->glow_width);
        lv_draw_rect(lv_event_get_draw_ctx(event), &dsc, &area);
    }
}
/* Invalidate edge strips rather than repainting the whole card per pulse. */
static void invalidate_frame(kitchen_card_t *card)
{
    lv_obj_t *obj = card->object;
    int band = (card->radius ? card->radius : 18) + 2;
    lv_area_t a, edge;
    lv_obj_get_coords(obj, &a);
    edge = a; edge.y2 = a.y1 + band - 1; lv_obj_invalidate_area(obj, &edge);
    edge = a; edge.y1 = a.y2 - band + 1; lv_obj_invalidate_area(obj, &edge);
    edge = a; edge.x2 = a.x1 + band - 1; lv_obj_invalidate_area(obj, &edge);
    edge = a; edge.x1 = a.x2 - band + 1; lv_obj_invalidate_area(obj, &edge);
}
static void update_glow(kitchen_card_t *card, const kitchen_timer_t *timer, uint64_t current)
{
    uint64_t ms = kitchen_core_remaining(timer, current);
    bool urgent = timer->status == KITCHEN_RUNNING && ms > 0 && ms < 60000;
    bool done = timer->status == KITCHEN_DONE;
    unsigned period = done ? 1000 : 1300;
    unsigned phase = (unsigned)(current % period), half = period / 2;
    unsigned t = (phase <= half ? phase : period - phase) * 256 / half;
    unsigned ease = t * t * (768 - 2 * t) / 65536;
    int target = urgent || done ? 60 + (int)(12 * ease / 256) : 0;
    int glow = target ? target : card->glow > 20 ? card->glow - 20 : 0;
    int width = done ? 8 : urgent ? 6 : card->glow_width;
    int base = lv_obj_get_width(card->object) / 58;
    if (base < 4) base = 4;
    if (base > 6) base = 6;
    int border = done ? base + 2 + (int)(8 * ease / 256) : urgent ? base + 1 + (int)(6 * ease / 256) : base;
    int safe_max = card->inset ? card->inset - 5 : 16;
    if (border > safe_max) border = safe_max;
    int opacity = urgent || done ? 48 + (int)(207 * ease / 256) :
        card->border_opa > 236 ? card->border_opa - 16 : 220;
    if (card->glow != glow || card->glow_width != width || card->border_width != border || card->border_opa != opacity)
        invalidate_frame(card);
    card->border_width = border; card->border_opa = opacity;
    card->glow = glow; card->glow_width = width;
}
static void progress_value(lv_obj_t *track, lv_obj_t *fill, int progress)
{
    if (!track || !fill) return;
    int width = lv_obj_get_width(track) * progress / 1000;
    visible(fill, width > 0);
    if (width > 0 && lv_obj_get_width(fill) != width) lv_obj_set_width(fill, width);
}
static void make_progress(kitchen_ui_t *ui, lv_obj_t *parent, lv_obj_t **track, lv_obj_t **fill,
    int x, int y, int width, int height, uint32_t color)
{
    *track = box(ui, parent, x, y, width, height, EDGE);
    if (!*track) return;
    lv_obj_set_style_radius(*track, height, 0);
    *fill = box(ui, *track, 0, 0, width, height, color);
    if (*fill) lv_obj_set_style_radius(*fill, height, 0);
}
static void card_click(lv_event_t *event)
{
    kitchen_card_t *card = lv_event_get_user_data(event);
    kitchen_ui_t *ui = card->ui;
    if (ui->active && !ui->modal && !ui->failed) open_timer(ui, card->id);
}
static void make_card(kitchen_ui_t *ui, unsigned index)
{
    kitchen_card_t *card = &ui->cards[index];
    const int *g = geometry[index];
    int w = g[2], h = g[3];
    uint32_t color = palette[index % 5];
    *card = (kitchen_card_t){.ui = ui, .id = index + 1, .progress = -1};
    card->object = box(ui, ui->root, g[0], g[1], w, h, BG);
    if (!card->object) return;
    lv_obj_set_style_bg_color(card->object, lv_color_mix(lv_color_hex(color), lv_color_hex(BG), LV_OPA_20), 0);
    lv_obj_set_style_radius(card->object, 18, 0);
    lv_obj_set_style_border_color(card->object, lv_color_hex(color), 0);
    lv_obj_add_flag(card->object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card->object, card_click, LV_EVENT_CLICKED, card);
    lv_obj_add_event_cb(card->object, glow_draw, LV_EVENT_DRAW_MAIN, card);
    /* Typography uses available width; anchors and spacings use the actual
     * card height. The minimum touch target takes precedence on small slots. */
    bool large = w >= 340, small = w < 240;
    int inset = w * 55 / 1000;
    if (inset < 16) inset = 16;
    int radius = w * 65 / 1000;
    if (radius < 16) radius = 16;
    card->inset = inset; card->radius = radius;
    lv_obj_set_style_radius(card->object, radius, 0);
    const lv_font_t *icon_font = large ? &kitchen_symbols_88 : small ? &kitchen_symbols_40 : &kitchen_symbols_56;
    const lv_font_t *name_font = large ? &kitchen_text_32 : small ? &kitchen_text_18 : &kitchen_text_24;
    const lv_font_t *status_font = large ? &kitchen_status_36 : small ? &kitchen_status_18 : &kitchen_status_24;
    const lv_font_t *digits = large ? &kitchen_font_112 : small ? &kitchen_font_36 : &kitchen_font_72;
    int header_y = h * 5 / 100;
    if (header_y < 12) header_y = 12;
    int icon_size = icon_font->line_height;
    card->icon = text(ui, card->object, "", inset, header_y, icon_size, icon_size, icon_font);
    if (card->icon) lv_obj_set_style_text_align(card->icon, LV_TEXT_ALIGN_CENTER, 0);
    card->name = text(ui, card->object, "", inset, header_y + (icon_size - name_font->line_height) / 2,
        w - 2 * inset, name_font->line_height, name_font);
    int button_h = small ? 48 : large ? h * 16 / 100 : h * 19 / 100;
    if (!small && button_h < 56) button_h = 56;
    int button_y = h - inset - button_h;
    int track_h = h * 45 / 1000;
    if (track_h < 10) track_h = 10;
    int gap = h * 35 / 1000;
    if (gap < 8) gap = 8;
    int track_y = button_y - gap - track_h;
    int status_gap = large ? 12 : small ? 4 : 6;
    int status_y = track_y - status_gap - status_font->line_height;
    int time_y = status_y - (large ? 12 : small ? 3 : 6) - digits->line_height;
    card->time = text(ui, card->object, "", inset, time_y, w - 2 * inset, digits->line_height, digits);
    if (card->time) lv_obj_set_style_text_align(card->time, LV_TEXT_ALIGN_CENTER, 0);
    card->status = text(ui, card->object, "", inset, status_y, w - 2 * inset, status_font->line_height, status_font);
    if (card->status) lv_obj_set_style_text_align(card->status, LV_TEXT_ALIGN_CENTER, 0);
    make_progress(ui, card->object, &card->track, &card->fill, inset, track_y, w - 2 * inset, track_h, color);
    card->action = button(ui, card->object, "", inset, button_y, w - 2 * inset, button_h, false, A_CARD, index + 1);
    make_primary_icon(ui, card->action);
    if (card->action) {
        lv_obj_set_style_radius(card->action, button_h / 2, 0);
        lv_obj_set_style_bg_opa(card->action, LV_OPA_70, 0);
        lv_obj_set_style_border_width(card->action, 1, 0);
        lv_obj_set_style_border_color(card->action, lv_color_hex(color), 0);
        lv_obj_set_style_border_opa(card->action, LV_OPA_40, 0);
    }
}
static int progress(const kitchen_timer_t *timer, uint64_t current)
{
    if (timer->status != KITCHEN_RUNNING && timer->status != KITCHEN_PAUSED) return 0;
    uint64_t ms = kitchen_core_remaining(timer, current);
    return timer->span_ms ? (int)(ms * 1000 / timer->span_ms) : 0;
}
static void update_card(kitchen_card_t *card, uint64_t current)
{
    kitchen_timer_t *timer = kitchen_core_find(card->ui->state, card->id);
    if (!timer || !card->object) return;
    uint64_t ms = kitchen_core_remaining(timer, current);
    time_value(card->time, timer->status == KITCHEN_DONE ? kitchen_core_elapsed(timer, current) / 1000 : (ms + 999) / 1000);
    if (!card->painted || strcmp(timer->name, card->painted_name) || timer->icon != card->painted_icon) {
        label_value(card->name, timer->name);
        visible(card->icon, timer->icon != KITCHEN_ICON_NONE);
        label_value(card->icon, timer->icon == KITCHEN_ICON_NONE ? "" : icon_names[timer->icon]);
        int inset = card->inset;
        int offset = timer->icon == KITCHEN_ICON_NONE ? inset : inset + lv_obj_get_style_width(card->icon, 0) + 8;
        lv_obj_set_x(card->name, offset);
        lv_obj_set_width(card->name, geometry[card->id - 1][2] - offset - inset);
        memcpy(card->painted_name, timer->name, sizeof(card->painted_name));
        card->painted_icon = timer->icon;
    }
    if (!card->painted || card->painted_status != timer->status) {
        label_value(card->status, timer->status == KITCHEN_PAUSED ? "PAUSED" : timer->status == KITCHEN_DONE ? "DONE" : "");
        primary_caption(card->action, timer->status, geometry[card->id - 1][2], geometry[card->id - 1][2] < 240);
        card->painted_status = timer->status;
    }
    int value = progress(timer, current);
    if (value != card->progress) { progress_value(card->track, card->fill, value); card->progress = value; }
    update_glow(card, timer, current);
    card->painted = true;
}
static uint32_t rollers_value(kitchen_ui_t *ui)
{
    if (!ui->hours || !ui->minutes || !ui->seconds) return 0;
    return lv_roller_get_selected(ui->hours) * 3600U + lv_roller_get_selected(ui->minutes) * 60U + lv_roller_get_selected(ui->seconds);
}
static void rollers_set(kitchen_ui_t *ui, uint32_t seconds)
{
    lv_roller_set_selected(ui->hours, seconds / 3600, LV_ANIM_OFF);
    lv_roller_set_selected(ui->minutes, seconds / 60 % 60, LV_ANIM_OFF);
    lv_roller_set_selected(ui->seconds, seconds % 60, LV_ANIM_OFF);
}
static void make_rollers(kitchen_ui_t *ui)
{
    lv_obj_t **objects[] = {&ui->hours, &ui->minutes, &ui->seconds};
    const char *names[] = {"Hours", "Minutes", "Seconds"};
    for (unsigned i = 0; i < 3; ++i) {
        char options[180];
        unsigned length = 0, limit = i ? 60 : 24;
        for (unsigned n = 0; n < limit; ++n)
            length += (unsigned)snprintf(options + length, sizeof(options) - length, n + 1 == limit ? "%02u" : "%02u\n", n);
        lv_obj_t *label = ui->roller_labels[i] = text(ui, ui->editor_body, names[i], i * 188, 28, 176, 25, &kitchen_text_20);
        if (label) lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        if (ui->failed) return;
        lv_obj_t *obj = *objects[i] = lv_roller_create(ui->editor_body);
        if (!obj) { ui->failed = true; return; }
        lv_obj_set_pos(obj, i * 188, 56);
        lv_obj_set_width(obj, 176);
        lv_roller_set_options(obj, options, LV_ROLLER_MODE_NORMAL);
        lv_obj_set_style_text_font(obj, &kitchen_font_36, LV_PART_MAIN);
        lv_obj_set_style_text_font(obj, &kitchen_font_36, LV_PART_SELECTED);
        lv_obj_set_style_bg_color(obj, lv_color_hex(FIELD), LV_PART_MAIN);
        lv_obj_set_style_bg_color(obj, lv_color_hex(EDGE), LV_PART_SELECTED);
        lv_obj_set_style_text_color(obj, lv_color_hex(MUTED), LV_PART_MAIN);
        lv_obj_set_style_text_color(obj, lv_color_hex(LIGHT), LV_PART_SELECTED);
        lv_obj_set_style_radius(obj, 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
        lv_roller_set_visible_row_count(obj, 3);
    }
}
static void animate_overlay(kitchen_ui_t *ui, bool open)
{
    /* Instant presentation: no opacity/transform layer of the complete panel. */
    ui->opening = ui->closing = false;
    if (!open) close_modal(ui);
}
static void toggle_icons(kitchen_ui_t *ui, bool open)
{
    if (!ui->icon_row || ui->icons_open == open) return;
    if (open && ui->naming) finish_name(ui, false);
    ui->icons_open = open;
    visible(ui->icon_button, !open);
    visible(ui->name_button, !open);
    visible(ui->icon_guard, open);
    visible(ui->icon_row, open);
    if (open) {
        lv_obj_set_width(ui->icon_row, ui->fields_width);
        lv_obj_move_foreground(ui->icon_guard);
        lv_obj_move_foreground(ui->icon_row);
        lv_obj_move_foreground(ui->close_button);
    }
}
static void inside_click(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (ui->icons_open) { toggle_icons(ui, false); lv_event_stop_bubbling(event); }
}
static void finish_volume(kitchen_ui_t *ui, bool preview)
{
    if (!ui->volume_gesture) return;
    ui->volume_gesture = false;
    send(ui, KITCHEN_SAVE_VOLUME, 0);
    if (preview) send(ui, KITCHEN_PREVIEW_VOLUME, 0);
}
static void close_modal(kitchen_ui_t *ui)
{
    finish_volume(ui, false);
    lv_anim_del(ui, NULL);
    if (ui->modal) lv_obj_del(ui->modal);
    ui->modal = ui->panel = ui->body = ui->normal = ui->editor_body = NULL;
    ui->keyboard = ui->name_field = ui->name_button = ui->icon_button = ui->icon_row = NULL;
    ui->name_apply = ui->name_cancel = NULL;
    ui->icon_guard = ui->close_button = NULL;
    ui->hours = ui->minutes = ui->seconds = ui->apply = ui->edit_cancel = ui->message = NULL;
    ui->heading = ui->time = ui->status = ui->track = ui->fill = NULL;
    ui->primary = ui->secondary = ui->minus = ui->plus = ui->repeat = NULL;
    ui->volume_slider = ui->volume_value = NULL;
    memset(ui->presets, 0, sizeof(ui->presets));
    memset(ui->icon_buttons, 0, sizeof(ui->icon_buttons));
    ui->binding_count = ui->modal_binding_base;
    ui->modal_kind = MODAL_NONE;
    ui->normal_layout_valid = false;
    ui->fields_width = 0;
    memset(ui->roller_labels, 0, sizeof(ui->roller_labels));
    memset(&ui->overlay_card, 0, sizeof(ui->overlay_card));
    ui->editor = EDIT_NONE;
    ui->naming = ui->icons_open = ui->closing = ui->opening = false;
}
static void finish_name(kitchen_ui_t *ui, bool apply)
{
    if (!ui->naming) return;
    if (apply && ui->name_field) {
        const char *value = lv_textarea_get_text(ui->name_field);
        if (selected(ui)->status == KITCHEN_EMPTY) memcpy(ui->draft_name, value, strlen(value) + 1);
        else {
            kitchen_command_t cmd = {.kind = KITCHEN_EDIT, .id = ui->selected,
                .name = value, .icon = selected(ui)->icon};
            ui->command(ui->context, &cmd);
        }
    }
    ui->naming = false;
    if (ui->keyboard) lv_keyboard_set_textarea(ui->keyboard, NULL);
    if (ui->name_field) lv_obj_clear_state(ui->name_field, LV_STATE_FOCUSED);
    visible(ui->keyboard, false);
    visible(ui->name_field, false);
    visible(ui->name_apply, false);
    visible(ui->name_cancel, false);
    visible(ui->name_button, true);
    visible(ui->body, true);
    lv_obj_set_pos(ui->panel, OVERLAY_X, OVERLAY_Y);
    lv_obj_set_size(ui->panel, OVERLAY_W, OVERLAY_H);
    refresh_overlay(ui);
}
static void keyboard_event(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_READY) finish_name(ui, true);
    else if (lv_event_get_code(event) == LV_EVENT_CANCEL) finish_name(ui, false);
}
static void open_name(kitchen_ui_t *ui)
{
    if (ui->naming) return;
    if (!ui->name_field) {
        ui->name_field = lv_textarea_create(ui->panel);
        if (!ui->name_field) { ui->failed = true; return; }
        lv_obj_set_pos(ui->name_field, 132, 68);
        lv_textarea_set_one_line(ui->name_field, true);
        lv_obj_set_size(ui->name_field, 444, 56);
        lv_obj_set_style_text_font(ui->name_field, &kitchen_text_32, 0);
        lv_obj_set_style_bg_color(ui->name_field, lv_color_hex(FIELD), 0);
        lv_obj_set_style_text_color(ui->name_field, lv_color_hex(LIGHT), 0);
        lv_obj_set_style_border_color(ui->name_field, lv_color_hex(ACCENT), 0);
        /* Bold 39px line + 14px padding + 2px border fits the fixed 56px field. */
        lv_obj_set_style_pad_ver(ui->name_field, 7, 0);
        lv_obj_set_style_pad_ver(ui->name_field, 7, LV_STATE_FOCUSED);
        lv_obj_set_style_pad_hor(ui->name_field, 12, 0);
        lv_obj_set_style_border_width(ui->name_field, 1, 0);
        lv_obj_set_style_border_width(ui->name_field, 1, LV_STATE_FOCUSED);
        lv_obj_set_style_text_line_space(ui->name_field, 0, 0);
        lv_obj_set_style_text_font(ui->name_field, &kitchen_text_32, LV_PART_TEXTAREA_PLACEHOLDER);
        lv_textarea_set_max_length(ui->name_field, KITCHEN_NAME_MAX);
        static const char ascii[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        lv_textarea_set_accepted_chars(ui->name_field, ascii);
        lv_textarea_set_placeholder_text(ui->name_field, "Add name");
        lv_obj_set_scroll_dir(ui->name_field, LV_DIR_HOR);
        ui->name_cancel = button(ui, ui->panel, "Cancel", 132, 164, 200, 48, false, A_NAME_CANCEL, 0);
        ui->name_apply = button(ui, ui->panel, "Done", 352, 164, 200, 48, true, A_NAME_APPLY, 0);
        if (ui->failed) return;
        ui->keyboard = lv_keyboard_create(ui->modal);
        if (!ui->keyboard) { ui->failed = true; return; }
        lv_obj_set_size(ui->keyboard, 1024, 220);
        lv_obj_align(ui->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_text_font(ui->keyboard, &kitchen_text_20, LV_PART_ITEMS);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(PANEL), LV_PART_MAIN);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(BUTTON), LV_PART_ITEMS);
        lv_obj_set_style_text_color(ui->keyboard, lv_color_hex(LIGHT), LV_PART_ITEMS);
        lv_obj_set_style_bg_color(ui->keyboard, lv_color_hex(ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(ui->keyboard, lv_color_hex(BG), LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_set_style_color_filter_opa(ui->keyboard, LV_OPA_TRANSP, LV_PART_ITEMS | LV_STATE_PRESSED);
        lv_obj_add_event_cb(ui->keyboard, keyboard_event, LV_EVENT_ALL, ui);
    }
    lv_textarea_set_text(ui->name_field, selected(ui)->status == KITCHEN_EMPTY ? ui->draft_name : selected(ui)->name);
    lv_obj_scroll_to_y(ui->name_field, 0, LV_ANIM_OFF);
    lv_textarea_set_cursor_pos(ui->name_field, LV_TEXTAREA_CURSOR_LAST);
    lv_keyboard_set_mode(ui->keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(ui->keyboard, ui->name_field);
    lv_obj_add_state(ui->name_field, LV_STATE_FOCUSED);
    ui->naming = true;
    visible(ui->name_button, false);
    visible(ui->body, false);
    visible(ui->name_field, true);
    visible(ui->name_apply, true);
    visible(ui->name_cancel, true);
    visible(ui->keyboard, true);
    /* Preserve the header in place; only raise the bottom above the keyboard. */
    lv_obj_set_pos(ui->panel, OVERLAY_X, OVERLAY_Y);
    lv_obj_set_size(ui->panel, OVERLAY_W, 380 - 8 - OVERLAY_Y);
    layout_fields(ui);
    pair_buttons(ui->name_cancel, ui->name_apply, OVERLAY_W);
    lv_obj_move_foreground(ui->keyboard);
}
static void set_editor(kitchen_ui_t *ui, int editor, uint32_t seconds)
{
    ui->editor = editor;
    ui->normal_layout_valid = false;
    ui->completion_shown = false;
    if (editor != EDIT_NONE) rollers_set(ui, seconds);
    visible(ui->normal, editor == EDIT_NONE);
    visible(ui->editor_body, editor != EDIT_NONE);
    label_value(ui->message, "");
    bool preset = editor == EDIT_SETUP || editor == EDIT_DURATION;
    for (unsigned i = 0; i < PRESET_COUNT; ++i) visible(ui->presets[i], preset);
    visible(ui->edit_cancel, editor != EDIT_SETUP);
    refresh_overlay(ui);
}
static void timer_backdrop(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->active || ui->opening || ui->closing || lv_event_get_target(event) != ui->modal) return;
    if (ui->naming) finish_name(ui, false);
    animate_overlay(ui, false);
}
static void open_timer(kitchen_ui_t *ui, uint32_t id)
{
    if (ui->modal || id < 1 || id > KITCHEN_MAX_TIMERS) return;
    ui->selected = id;
    kitchen_timer_t *timer = selected(ui);
    ui->modal_binding_base = ui->binding_count;
    ui->modal_kind = MODAL_TIMER;
    ui->normal_layout_valid = false;
    ui->draft_name[0] = 0;
    ui->draft_icon = KITCHEN_ICON_NONE;
    ui->modal = box(ui, ui->root, 0, 0, 1024, 600, 0);
    if (!ui->modal) return;
    lv_obj_set_style_bg_opa(ui->modal, LV_OPA_70, 0);
    lv_obj_add_flag(ui->modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->modal, timer_backdrop, LV_EVENT_CLICKED, ui);
    uint32_t color = palette[(id - 1) % 5];
    ui->panel = box(ui, ui->modal, OVERLAY_X, OVERLAY_Y, OVERLAY_W, OVERLAY_H, BG);
    if (!ui->panel) return;
    lv_color_t surface = lv_color_mix(lv_color_hex(color), lv_color_hex(BG), LV_OPA_20);
    lv_obj_set_style_bg_color(ui->panel, surface, 0);
    lv_obj_set_style_radius(ui->panel, 18, 0);
    lv_obj_set_style_clip_corner(ui->panel, true, 0);
    /* Fixed inset frame, independent of the selected timer's status/deadline.
     * Drawing it keeps the content origin unchanged by a styled border. */
    ui->overlay_card = (kitchen_card_t){.ui = ui, .id = id, .object = ui->panel,
        .inset = 24, .radius = 18, .border_width = 6, .border_opa = LV_OPA_COVER};
    lv_obj_add_event_cb(ui->panel, glow_draw, LV_EVENT_DRAW_MAIN, &ui->overlay_card);
    lv_obj_add_flag(ui->panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->panel, inside_click, LV_EVENT_CLICKED, ui);
    ui->heading = text(ui, ui->panel, "", 24, 8, 490, 39, &kitchen_text_32);
    ui->close_button = button(ui, ui->panel, LV_SYMBOL_CLOSE, 550, 0, 48, 48, false, A_CLOSE, 0);
    ui->icon_button = button(ui, ui->panel, "", 24, 56, 96, 80, false, A_ICON_TOGGLE, 0);
    if (ui->close_button) {
        lv_obj_set_style_radius(ui->close_button, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ui->close_button, 0, 0);
        lv_obj_t *label = lv_obj_get_child(ui->close_button, 0);
        lv_obj_set_style_text_font(label, &kitchen_lvgl_symbols_32, 0);
        lv_obj_set_style_align(label, LV_ALIGN_TOP_LEFT, 0);
        lv_obj_set_size(label, 48, 48);
        lv_font_glyph_dsc_t close_glyph;
        if (lv_font_get_glyph_dsc(&kitchen_lvgl_symbols_32, &close_glyph, 0xf00d, 0)) {
            int x = ((int)close_glyph.adv_w - close_glyph.box_w) / 2 - close_glyph.ofs_x;
            int y = (48 - close_glyph.box_h) / 2 - (kitchen_lvgl_symbols_32.line_height -
                kitchen_lvgl_symbols_32.base_line - close_glyph.box_h - close_glyph.ofs_y);
            lv_obj_set_pos(label, x, y);
        }
        lv_obj_set_style_bg_color(ui->close_button, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(ui->close_button, LV_OPA_30, 0);
        lv_obj_set_style_bg_opa(ui->close_button, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_text_opa(ui->close_button, LV_OPA_80, 0);
    }
    add_no_icon(ui, ui->icon_button);
    ui->name_button = button(ui, ui->panel, "", 132, 68, 444, 56, false, A_NAME, 0);
    if (ui->name_button) {
        lv_obj_t *label = lv_obj_get_child(ui->name_button, 0);
        lv_obj_set_style_text_font(label, &kitchen_text_32, 0);
        lv_obj_set_height(label, kitchen_text_32.line_height);
        lv_obj_center(label);
    }
    ui->icon_row = box(ui, ui->panel, 24, 56, 1, 80, FIELD);
    if (ui->icon_row) {
        lv_obj_set_style_radius(ui->icon_row, 12, 0);
        lv_obj_set_style_clip_corner(ui->icon_row, true, 0);
        for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) {
            int x = i * 96;
            ui->icon_buttons[i] = button(ui, ui->icon_row, "", x, 0, 88, 80, false, A_ICON, i);
            if (!i) add_no_icon(ui, ui->icon_buttons[i]);
            if (ui->icon_buttons[i]) {
                /* Reserve border space so pressing never shifts the glyph. */
                lv_obj_set_style_border_width(ui->icon_buttons[i], 2, 0);
                lv_obj_set_style_border_opa(ui->icon_buttons[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(ui->icon_buttons[i], 2, LV_STATE_CHECKED);
                lv_obj_set_style_border_color(ui->icon_buttons[i], lv_color_hex(ACCENT), LV_STATE_PRESSED);
                lv_obj_set_style_border_opa(ui->icon_buttons[i], LV_OPA_COVER, LV_STATE_PRESSED);
                lv_obj_set_style_border_opa(ui->icon_buttons[i], LV_OPA_COVER, LV_STATE_CHECKED | LV_STATE_PRESSED);
                lv_obj_set_style_bg_color(ui->icon_buttons[i], lv_color_hex(ACCENT), LV_STATE_CHECKED);
                lv_obj_set_style_bg_opa(ui->icon_buttons[i], LV_OPA_COVER, LV_STATE_CHECKED);
                lv_obj_set_style_text_color(ui->icon_buttons[i], lv_color_hex(BG), LV_STATE_CHECKED);
            }
        }
        visible(ui->icon_row, false);
    }
    ui->body = box(ui, ui->panel, 24, 140, BODY_W, 300, BG);
    if (!ui->body) return;
    lv_obj_set_style_bg_opa(ui->body, LV_OPA_TRANSP, 0);
    ui->normal = box(ui, ui->body, 0, 0, BODY_W, 300, BG);
    ui->editor_body = box(ui, ui->body, 0, 0, BODY_W, 300, BG);
    if (ui->failed) return;
    lv_obj_set_style_bg_opa(ui->normal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(ui->editor_body, LV_OPA_TRANSP, 0);
    ui->time = text(ui, ui->normal, "", 0, 12, BODY_W, 88, &kitchen_font_72);
    if (ui->time) {
        lv_obj_set_style_text_align(ui->time, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_flag(ui->time, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui->time, action_event, LV_EVENT_CLICKED, &ui->bindings[ui->binding_count]);
        ui->bindings[ui->binding_count++] = (kitchen_binding_t){ui, A_TIME, 0};
    }
    ui->status = text(ui, ui->normal, "", 0, 92, BODY_W, 30, &kitchen_status_24);
    if (ui->status) lv_obj_set_style_text_align(ui->status, LV_TEXT_ALIGN_CENTER, 0);
    make_progress(ui, ui->normal, &ui->track, &ui->fill, 0, 220, BODY_W, 16, color);
    ui->minus = button(ui, ui->normal, "-1 min", 0, 152, 264, 56, false, A_MINUS, 0);
    ui->plus = button(ui, ui->normal, "+1 min", 288, 152, 264, 56, false, A_PLUS, 0);
    ui->repeat = button(ui, ui->normal, "", 288, 252, 264, 48, false, A_REPEAT, 0);
    lv_obj_t *time_buttons[] = {ui->minus, ui->plus};
    for (unsigned i = 0; i < sizeof(time_buttons) / sizeof(time_buttons[0]); ++i) {
        if (!time_buttons[i]) continue;
        lv_obj_t *label = lv_obj_get_child(time_buttons[i], 0);
        lv_obj_set_style_text_font(label, &kitchen_text_24, 0);
        lv_obj_set_height(label, kitchen_text_24.line_height);
        lv_obj_center(label);
    }
    ui->primary = button(ui, ui->normal, "", 0, 244, BODY_W, 48, true, A_PRIMARY, 0);
    make_primary_icon(ui, ui->primary);
    ui->secondary = button(ui, ui->normal, "Cancel timer", 0, 244, BODY_W, 48, false, A_CANCEL_TIMER, 0);
    lv_obj_t *bottom_buttons[] = {ui->secondary, ui->repeat};
    for (unsigned i = 0; i < sizeof(bottom_buttons) / sizeof(bottom_buttons[0]); ++i) {
        if (!bottom_buttons[i]) continue;
        lv_obj_t *label = lv_obj_get_child(bottom_buttons[i], 0);
        lv_obj_set_style_text_font(label, &kitchen_actions_20, 0);
        lv_obj_set_height(label, kitchen_actions_20.line_height);
        lv_obj_center(label);
    }
    make_rollers(ui);
    for (unsigned i = 0; i < PRESET_COUNT; ++i) {
        char value[16];
        snprintf(value, sizeof(value), "%u min", (unsigned)(presets[i] / 60));
        ui->presets[i] = button(ui, ui->editor_body, value, i * 92, 196, 90, 48, false, A_PRESET, presets[i]);
        if (ui->presets[i]) {
            lv_obj_t *label = lv_obj_get_child(ui->presets[i], 0);
            lv_obj_set_style_text_font(label, &kitchen_text_18, 0);
            lv_obj_set_height(label, kitchen_text_18.line_height);
            lv_obj_set_style_bg_color(ui->presets[i], lv_color_hex(ACCENT), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(ui->presets[i], lv_color_hex(BG), LV_STATE_CHECKED);
        }
    }
    ui->message = text(ui, ui->editor_body, "", 0, 200, BODY_W, 25, &kitchen_text_18);
    ui->edit_cancel = button(ui, ui->editor_body, "Cancel", 0, 252, 264, 48, false, A_CANCEL_EDIT, 0);
    ui->apply = button(ui, ui->editor_body, "", 288, 252, 264, 48, true, A_APPLY, 0);
    make_primary_icon(ui, ui->apply);
    ui->icon_guard = box(ui, ui->panel, 0, 0, OVERLAY_W, OVERLAY_H, BG);
    if (ui->icon_guard) {
        lv_obj_set_style_bg_opa(ui->icon_guard, LV_OPA_TRANSP, 0);
        lv_obj_add_flag(ui->icon_guard, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui->icon_guard, inside_click, LV_EVENT_CLICKED, ui);
        visible(ui->icon_guard, false);
    }
    if (ui->failed) return;
    lv_obj_update_layout(ui->root);
    ui->shown_status = timer->status;
    set_editor(ui, timer->status == KITCHEN_EMPTY ? EDIT_SETUP : EDIT_NONE, 0);
    lv_obj_move_foreground(ui->notice);
    lv_obj_move_foreground(ui->retry);
    animate_overlay(ui, true);
}
static void icon_value(lv_obj_t *button_obj, kitchen_icon_t icon)
{
    if (!button_obj) return;
    lv_obj_t *glyph = lv_obj_get_child(button_obj, 0);
    label_value(glyph, icon_names[icon]);
    if (glyph) {
        /* Block exists in the retained 48px subset, not in the 40px subset. */
        lv_obj_set_style_text_font(glyph, icon == KITCHEN_ICON_NONE ? &kitchen_symbols_48 : &kitchen_symbols_40, 0);
        lv_obj_set_size(glyph, 56, icon == KITCHEN_ICON_NONE ? 58 : 48);
        if (icon == KITCHEN_ICON_NONE) lv_obj_align(glyph, LV_ALIGN_TOP_MID, 0, 0);
        else lv_obj_center(glyph);
    }
    lv_obj_t *description = lv_obj_get_child(button_obj, 1);
    if (description) lv_obj_align(description, LV_ALIGN_BOTTOM_MID, 0, -4);
    visible(description, icon == KITCHEN_ICON_NONE);
}
static void add_no_icon(kitchen_ui_t *ui, lv_obj_t *obj)
{
    if (obj) {
        lv_obj_t *label = text(ui, obj, "No icon", 2, 54, 84, 23, &kitchen_text_18);
        if (label) lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }
}
/* Measure one shared column from the actual preset labels, once per overlay.
 * Rollers, header fields and the icon chooser use these exact outer edges. */
static void layout_fields(kitchen_ui_t *ui)
{
    int width = ui->fields_width;
    if (!width) {
        int widths[PRESET_COUNT];
        width = (PRESET_COUNT - 1) * 8;
        for (unsigned i = 0; i < PRESET_COUNT; ++i) {
            widths[i] = fit_button(ui->presets[i], 90);
            width += widths[i];
        }
        ui->fields_width = width;
        int x = (BODY_W - width) / 2;
        for (unsigned i = 0; i < PRESET_COUNT; ++i) {
            lv_obj_set_x(ui->presets[i], x);
            x += widths[i] + 8;
        }
        int left = (BODY_W - width) / 2;
        lv_obj_set_x(ui->track, left);
        lv_obj_set_width(ui->track, width);
        lv_obj_t *rollers[] = {ui->hours, ui->minutes, ui->seconds};
        int column = (width - 24) / 3;
        for (unsigned i = 0; i < 3; ++i) {
            int w = i == 2 ? width - 2 * (column + 12) : column;
            lv_obj_set_x(rollers[i], left + i * (column + 12));
            lv_obj_set_width(rollers[i], w);
            lv_obj_set_x(ui->roller_labels[i], left + i * (column + 12));
            lv_obj_set_width(ui->roller_labels[i], w);
        }
        int header_left = 24 + left;
        lv_obj_set_pos(ui->icon_button, header_left, 56);
        lv_obj_set_pos(ui->name_button, header_left + 108, 68);
        lv_obj_set_width(ui->name_button, width - 108);
        lv_obj_t *name_label = lv_obj_get_child(ui->name_button, 0);
        lv_obj_set_width(name_label, width - 120);
        lv_obj_center(name_label);
        lv_obj_set_pos(ui->icon_row, header_left, 56);
        int cell = (width - 32) / KITCHEN_ICON_COUNT;
        for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) {
            lv_obj_set_x(ui->icon_buttons[i], i * (cell + 8));
            lv_obj_set_width(ui->icon_buttons[i], i == KITCHEN_ICON_COUNT - 1 ? width - i * (cell + 8) : cell);
        }
        lv_obj_update_layout(ui->panel);
    }
    if (ui->name_field) {
        lv_obj_set_pos(ui->name_field, 24 + (BODY_W - width) / 2 + 108, 68);
        lv_obj_set_width(ui->name_field, width - 108);
    }
}
static void refresh_overlay(kitchen_ui_t *ui)
{
    if (ui->modal_kind != MODAL_TIMER || !ui->panel || ui->failed) return;
    kitchen_timer_t *timer = selected(ui);
    uint64_t current = now(ui), ms = kitchen_core_remaining(timer, current);
    bool empty = timer->status == KITCHEN_EMPTY, done = timer->status == KITCHEN_DONE;
    /* The title follows the active editor without rebuilding its controls.
     * Icon selection leaves the current editing context/title unchanged. */
    const char *title = ui->naming ? "Timer name" :
        ui->editor == EDIT_DURATION ? "Duration" :
        ui->editor == EDIT_REMAINING ? (done ? "Add more time" : "Remaining time") :
        ui->editor == EDIT_BONUS ? "Add more time" : empty ? "Set up timer" : "Timer";
    label_value(ui->heading, title);
    layout_fields(ui);
    const char *name = empty ? ui->draft_name : timer->name;
    kitchen_icon_t icon = empty ? ui->draft_icon : timer->icon;
    caption(ui->name_button, name[0] ? name : "Add name");
    if (ui->name_button) lv_obj_set_style_text_color(ui->name_button, lv_color_hex(name[0] ? LIGHT : MUTED), 0);
    icon_value(ui->icon_button, icon);
    for (unsigned i = 0; i < KITCHEN_ICON_COUNT; ++i) {
        icon_value(ui->icon_buttons[i], (kitchen_icon_t)i);
        if (ui->icon_buttons[i]) {
            bool check = i == (unsigned)icon;
            if (check != lv_obj_has_state(ui->icon_buttons[i], LV_STATE_CHECKED)) {
                if (check) lv_obj_add_state(ui->icon_buttons[i], LV_STATE_CHECKED);
                else lv_obj_clear_state(ui->icon_buttons[i], LV_STATE_CHECKED);
            }
        }
    }
    if (ui->editor != EDIT_NONE) {
        visible(lv_obj_get_child(ui->apply, 1), ui->editor == EDIT_SETUP);
        if (ui->editor == EDIT_REMAINING && done && !ui->completion_shown) {
            ui->completion_shown = true;
            label_value(ui->message, "Timer finished");
        }
        caption(ui->apply, ui->editor == EDIT_SETUP || ui->editor == EDIT_BONUS ||
            (ui->editor == EDIT_REMAINING && ui->completion_shown) ? "Start timer" : "Apply");
        uint32_t seconds = rollers_value(ui);
        disabled(ui->apply, !seconds || seconds > KITCHEN_MAX_SECONDS);
        for (unsigned i = 0; i < PRESET_COUNT; ++i) {
            if (!ui->presets[i]) continue;
            bool check = seconds == presets[i];
            if (check != lv_obj_has_state(ui->presets[i], LV_STATE_CHECKED)) {
                if (check) lv_obj_add_state(ui->presets[i], LV_STATE_CHECKED);
                else lv_obj_clear_state(ui->presets[i], LV_STATE_CHECKED);
            }
        }
    } else {
        time_value(ui->time, done ? kitchen_core_elapsed(timer, current) / 1000 : (ms + 999) / 1000);
        label_value(ui->status, done ? "DONE" : timer->status == KITCHEN_PAUSED ? "PAUSED" : "");
        progress_value(ui->track, ui->fill, progress(timer, current));
        visible(ui->minus, !done);
        visible(ui->repeat, done);
        visible(ui->secondary, timer->status == KITCHEN_RUNNING || timer->status == KITCHEN_PAUSED);
        disabled(ui->minus, ms <= 60000);
        disabled(ui->plus, !done && ms > (uint64_t)(KITCHEN_MAX_SECONDS - 60) * 1000);
        primary_caption(ui->primary, timer->status, BODY_W, false);
        if (done) {
            char value[64], duration[32];
            if (!(timer->duration_s % 60) && timer->duration_s < 3600)
                snprintf(value, sizeof(value), "Repeat · %u min", (unsigned)(timer->duration_s / 60));
            else {
                format_time(duration, sizeof(duration), timer->duration_s);
                snprintf(value, sizeof(value), "Repeat · %s", duration);
            }
            caption(ui->repeat, value);
        }
    }
    if (ui->editor != EDIT_NONE) {
        if (ui->editor == EDIT_SETUP) primary_caption(ui->apply, KITCHEN_READY, BODY_W, false);
        else pair_buttons(ui->edit_cancel, ui->apply, BODY_W);

    } else {
        int left = (BODY_W - ui->fields_width) / 2;
        int right = left + ui->fields_width;
        if (done) {
            fit_button(ui->plus, (ui->fields_width - 16) / 2);
            int repeat_width = fit_button(ui->repeat, (ui->fields_width - 16) / 2);
            lv_obj_set_x(ui->plus, left);
            lv_obj_set_x(ui->repeat, right - repeat_width);
            /* primary_caption already centers the icon-aware OK button. */
        } else {
            fit_button(ui->minus, (ui->fields_width - 16) / 2);
            int plus_width = fit_button(ui->plus, (ui->fields_width - 16) / 2);
            lv_obj_set_x(ui->minus, left);
            lv_obj_set_x(ui->plus, right - plus_width);
        }
        if (timer->status == KITCHEN_RUNNING || timer->status == KITCHEN_PAUSED) {
            int secondary_width = fit_button(ui->secondary, (ui->fields_width - 16) / 2);
            lv_obj_set_x(ui->primary, left);
            lv_obj_set_x(ui->secondary, right - secondary_width);
        }
        if (timer->status != ui->shown_status || !ui->normal_layout_valid) {
            bool show_status = timer->status == KITCHEN_PAUSED || done;
            /* Preserve fixed anchors for normal states; DONE uses the
             * earlier layout with Repeat beside +1 min above progress. */
            visible(ui->status, show_status);
            lv_obj_set_y(ui->time, done ? 0 : 12);
            lv_obj_set_y(ui->status, done ? 92 : 88);
            lv_obj_set_y(ui->minus, 120);
            lv_obj_set_y(ui->plus, done ? 140 : 120);
            lv_obj_set_y(ui->track, done ? 212 : 196);
            lv_obj_set_y(ui->repeat, 140);
            lv_obj_set_height(ui->repeat, 56);
            lv_obj_set_y(ui->primary, 252);
            lv_obj_set_y(ui->secondary, 252);
            ui->normal_layout_valid = true;
        }
    }
    ui->shown_status = timer->status;
}
static void speaker_draw(lv_event_t *event)
{
    lv_obj_t *obj = lv_event_get_target(event);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    int x = (a.x1 + a.x2) / 2, y = (a.y1 + a.y2) / 2;
    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.bg_color = lv_color_hex(LIGHT);
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
static void volume_changed(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->active || !ui->audio_available) return;
    int value = lv_slider_get_value(lv_event_get_target(event));
    if (!send(ui, KITCHEN_SET_VOLUME, value)) lv_slider_set_value(ui->volume_slider, ui->state->volume_percent, LV_ANIM_OFF);
    char percent[8];
    snprintf(percent, sizeof(percent), "%u%%", (unsigned)ui->state->volume_percent);
    label_value(ui->volume_value, percent);
}
static void volume_gesture(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (!ui->active) return;
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) ui->volume_gesture = true;
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) finish_volume(ui, true);
}
static void volume_backdrop(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (ui->active && lv_event_get_target(event) == ui->modal) close_modal(ui);
}
static void open_volume(kitchen_ui_t *ui)
{
    if (ui->modal || !ui->audio_available) return;
    ui->modal_binding_base = ui->binding_count;
    ui->modal_kind = MODAL_VOLUME;
    ui->modal = box(ui, ui->root, 0, 0, 1024, 600, 0);
    if (!ui->modal) return;
    lv_obj_set_style_bg_opa(ui->modal, LV_OPA_70, 0);
    lv_obj_add_flag(ui->modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->modal, volume_backdrop, LV_EVENT_CLICKED, ui);
    ui->panel = box(ui, ui->modal, 212, 144, 600, 312, PANEL);
    if (!ui->panel) return;
    lv_obj_add_flag(ui->panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui->panel, 18, 0);
    text(ui, ui->panel, "Volume", 24, 20, 440, 39, &kitchen_text_32);
    button(ui, ui->panel, LV_SYMBOL_CLOSE, 524, 16, 52, 48, false, A_CLOSE_VOLUME, 0);
    char percent[8];
    snprintf(percent, sizeof(percent), "%u%%", (unsigned)ui->state->volume_percent);
    ui->volume_value = text(ui, ui->panel, percent, 24, 86, 552, 30, &kitchen_text_24);
    if (ui->volume_value) lv_obj_set_style_text_align(ui->volume_value, LV_TEXT_ALIGN_CENTER, 0);
    if (ui->failed) return;
    ui->volume_slider = lv_slider_create(ui->panel);
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
    lv_obj_add_event_cb(ui->volume_slider, volume_gesture, LV_EVENT_ALL, ui);
    text(ui, ui->panel, "0%", 24, 190, 260, 25, &kitchen_text_20);
    lv_obj_t *maximum = text(ui, ui->panel, "100%", 500, 190, 76, 25, &kitchen_text_20);
    if (maximum) lv_obj_set_style_text_align(maximum, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *done_button = button(ui, ui->panel, "Done", 200, 236, 200, 48, true, A_CLOSE_VOLUME, 0);
    center_button(done_button, 600);
    lv_obj_move_foreground(ui->notice);
    lv_obj_move_foreground(ui->retry);
}
static void apply_editor(kitchen_ui_t *ui)
{
    kitchen_timer_t *timer = selected(ui);
    uint32_t seconds = rollers_value(ui);
    if (!seconds || seconds > KITCHEN_MAX_SECONDS) return;
    /* Model may finish between the displayed frame and this click. This first
     * click then changes the action only; it must never restart implicitly. */
    kitchen_core_update(ui->state, now(ui));
    if (ui->editor == EDIT_REMAINING && timer->status == KITCHEN_DONE && !ui->completion_shown) {
        ui->completion_shown = true;
        label_value(ui->message, "Timer finished");
        caption(ui->apply, "Start timer");
        pair_buttons(ui->edit_cancel, ui->apply, BODY_W);
        return;
    }
    if (ui->editor == EDIT_SETUP) {
        kitchen_command_t cmd = {.kind = KITCHEN_CREATE, .id = ui->selected, .seconds = (int32_t)seconds,
            .name = ui->draft_name, .icon = ui->draft_icon};
        if (ui->command(ui->context, &cmd)) animate_overlay(ui, false);
    } else if (send(ui, timer->status == KITCHEN_DONE ? KITCHEN_EXTEND : KITCHEN_SET_TIME, (int)seconds))
        set_editor(ui, EDIT_NONE, 0);
}
static void action_event(lv_event_t *event)
{
    kitchen_binding_t binding = *(kitchen_binding_t *)lv_event_get_user_data(event);
    kitchen_ui_t *ui = binding.ui;
    lv_event_stop_bubbling(event);
    if (!ui->active || ui->failed || ui->closing || ui->opening) return;
    if (ui->icons_open && binding.action != A_ICON && binding.action != A_ICON_TOGGLE && binding.action != A_CLOSE) {
        toggle_icons(ui, false);
        return;
    }
    /* Use the visible primary action even if a deadline was crossed meanwhile. */
    kitchen_status_t shown = ui->shown_status;
    kitchen_core_update(ui->state, now(ui));
    kitchen_timer_t *timer = selected(ui);
    switch (binding.action) {
    case A_CARD:
        if (ui->modal) break;
        ui->selected = (uint32_t)binding.value;
        shown = ui->cards[binding.value - 1].painted_status;
        primary(ui, shown);
        break;
    case A_CLOSE:
        if (ui->naming) finish_name(ui, false);
        animate_overlay(ui, false);
        break;
    case A_PRIMARY:
        if (primary(ui, shown) && (shown == KITCHEN_READY || shown == KITCHEN_DONE)) animate_overlay(ui, false);
        break;
    case A_CANCEL_TIMER:
        if (send(ui, KITCHEN_CANCEL, 0)) close_modal(ui);
        break;
    case A_MINUS: send(ui, KITCHEN_ADJUST, -60); break;
    case A_PLUS: send(ui, timer->status == KITCHEN_DONE ? KITCHEN_EXTEND : KITCHEN_ADJUST, 60); break;
    case A_REPEAT:
        if (send(ui, KITCHEN_REPEAT, 0)) close_modal(ui);
        break;
    case A_TIME:
        set_editor(ui, timer->status == KITCHEN_READY ? EDIT_DURATION : timer->status == KITCHEN_DONE ? EDIT_BONUS : EDIT_REMAINING,
            timer->status == KITCHEN_DONE ? 60 : (uint32_t)((kitchen_core_remaining(timer, now(ui)) + 999) / 1000));
        break;
    case A_APPLY: apply_editor(ui); break;
    case A_CANCEL_EDIT: set_editor(ui, EDIT_NONE, 0); break;
    case A_PRESET: rollers_set(ui, (uint32_t)binding.value); break;
    case A_NAME: open_name(ui); break;
    case A_NAME_APPLY: finish_name(ui, true); break;
    case A_NAME_CANCEL: finish_name(ui, false); break;
    case A_ICON_TOGGLE: toggle_icons(ui, !ui->icons_open); break;
    case A_ICON:
        if (!ui->icons_open) break;
        if (timer->status == KITCHEN_EMPTY) ui->draft_icon = (kitchen_icon_t)binding.value;
        else {
            kitchen_command_t cmd = {.kind = KITCHEN_EDIT, .id = ui->selected, .name = timer->name,
                .icon = (kitchen_icon_t)binding.value};
            ui->command(ui->context, &cmd);
        }
        toggle_icons(ui, false);
        break;
    case A_VOLUME: open_volume(ui); break;
    case A_CLOSE_VOLUME: close_modal(ui); break;
    case A_RETRY: send(ui, KITCHEN_RETRY_SAVE, 0); break;
    }
    refresh(ui);
}
static void refresh(kitchen_ui_t *ui)
{
    if (!ui->root || ui->failed) return;
    uint64_t current = now(ui);
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) {
        /* Freeze hidden motion, but still display newly completed timers.
         * Core/heartbeat/audio/storage continue independently of this cache. */
        if (!ui->modal || ui->cards[i].painted_status != ui->state->timers[i].status)
            update_card(&ui->cards[i], current);
    }
    refresh_overlay(ui);
    bool error = kitchen_settings_failed(ui->settings);
    const char *notice = !ui->settings->available ? "Settings won't be saved" :
        error ? "Couldn't save settings" : kitchen_settings_protected(ui->settings) ? "Newer settings are read-only" : "";
    label_value(ui->notice, notice);
    visible(ui->retry, error && ui->settings->available);
}
static void tick(lv_timer_t *timer)
{
    kitchen_ui_t *ui = timer->user_data;
    if (!ui->active || !ui->root) return;
    kitchen_core_update(ui->state, now(ui));
    refresh(ui);
    if (ui->failed && ui->modal) {
        /* Roll back a partial form allocation; keep all timer sessions alive. */
        close_modal(ui);
        ui->failed = false;
        label_value(ui->notice, "Unable to open controls. Try again.");
    }
}
static void root_deleted(lv_event_t *event)
{
    kitchen_ui_t *ui = lv_event_get_user_data(event);
    if (lv_event_get_target(event) != ui->root) return;
    finish_volume(ui, false);
    lv_anim_del(ui, NULL);
    if (ui->tick) lv_timer_del(ui->tick);
    ui->tick = NULL;
    ui->root = NULL;
    ui->modal = ui->panel = ui->keyboard = NULL;
    ui->body = ui->normal = ui->editor_body = ui->name_field = NULL;
    ui->name_button = ui->icon_button = ui->icon_row = ui->icon_guard = ui->close_button = NULL;
    ui->name_apply = ui->name_cancel = NULL;
    ui->hours = ui->minutes = ui->seconds = ui->apply = ui->edit_cancel = ui->message = NULL;
    ui->heading = ui->time = ui->status = ui->track = ui->fill = NULL;
    ui->primary = ui->secondary = ui->minus = ui->plus = ui->repeat = NULL;
    ui->volume_button = ui->notice = ui->retry = NULL;
    ui->volume_slider = ui->volume_value = NULL;
    memset(ui->presets, 0, sizeof(ui->presets));
    memset(ui->icon_buttons, 0, sizeof(ui->icon_buttons));
    memset(ui->roller_labels, 0, sizeof(ui->roller_labels));
    ui->fields_width = 0;
    memset(ui->cards, 0, sizeof(ui->cards));
    memset(&ui->overlay_card, 0, sizeof(ui->overlay_card));
    ui->binding_count = 0;
    ui->active = false;
    ui->modal_kind = MODAL_NONE;
    ui->opening = ui->closing = false;
    ui->naming = ui->icons_open = false;
}
void kitchen_ui_refresh(kitchen_ui_t *ui)
{
    if (ui->active) refresh(ui);
}
void kitchen_ui_set_active(kitchen_ui_t *ui, bool active)
{
    active = active && ui->root;
    if (ui->active == active) return;
    if (!active) {
        finish_volume(ui, false);
        lv_anim_del(ui, NULL);
        if (ui->closing) close_modal(ui);
        else if (ui->modal_kind == MODAL_TIMER && ui->panel) {
            ui->opening = false;

            if (ui->icon_row) {
                lv_obj_set_width(ui->icon_row, ui->icons_open ? ui->fields_width : 1);
                visible(ui->icon_row, ui->icons_open);
                if (!ui->icons_open) lv_obj_set_width(ui->icon_button, 96);
            }
        }
    }
    ui->active = active;
    visible(ui->root, active);
    if (ui->tick) {
        if (active) { lv_timer_resume(ui->tick); lv_timer_ready(ui->tick); }
        else lv_timer_pause(ui->tick);
    }
    if (active) { kitchen_core_update(ui->state, now(ui)); refresh(ui); }
}
void kitchen_ui_destroy(kitchen_ui_t *ui)
{
    lv_anim_del(ui, NULL);
    if (ui->root) lv_obj_del(ui->root);
    if (ui->tick) { lv_timer_del(ui->tick); ui->tick = NULL; }
    ui->active = false;
}
esp_err_t kitchen_ui_create(kitchen_ui_t *ui, lv_obj_t *parent, kitchen_state_t *state, kitchen_settings_t *settings,
    bool (*command)(void *, const kitchen_command_t *), uint64_t (*now_ms)(void *),
    void (*back)(void *), bool audio_available, void *context)
{
    if (!ui || !parent || !state || !settings || !command || !now_ms || !back) return ESP_ERR_INVALID_ARG;
    kitchen_ui_destroy(ui);
    memset(ui, 0, sizeof(*ui));
    ui->state = state; ui->settings = settings; ui->command = command; ui->now_ms = now_ms;
    ui->back = back; ui->context = context; ui->audio_available = audio_available;
    ui->root = box(ui, parent, 0, 0, 1024, 600, BG);
    if (!ui->root) return ESP_ERR_NO_MEM;
    lv_obj_set_style_text_color(ui->root, lv_color_hex(LIGHT), 0);
    lv_obj_set_style_text_font(ui->root, &kitchen_text_20, 0);
    lv_obj_add_event_cb(ui->root, root_deleted, LV_EVENT_DELETE, ui);
    text(ui, ui->root, "Timers", 24, 12, 180, 40, &kitchen_text_32);
    ui->volume_button = button(ui, ui->root, "", 936, 8, 64, 48, false, A_VOLUME, 0);
    if (ui->volume_button) {
        lv_obj_add_event_cb(ui->volume_button, speaker_draw, LV_EVENT_DRAW_MAIN, ui);
        disabled(ui->volume_button, !audio_available);
    }
    for (unsigned i = 0; i < KITCHEN_MAX_TIMERS; ++i) make_card(ui, i);
    ui->notice = text(ui, ui->root, "", 224, 20, 530, 25, &kitchen_text_20);
    ui->retry = button(ui, ui->root, "Retry", 782, 8, 130, 48, false, A_RETRY, 0);
    if (ui->retry) fit_button(ui->retry, 130);
    visible(ui->retry, false);
    if (ui->failed) { kitchen_ui_destroy(ui); return ESP_ERR_NO_MEM; }
    lv_obj_update_layout(ui->root);
    refresh(ui);
    ui->tick = lv_timer_create(tick, 50, ui);
    if (!ui->tick) { kitchen_ui_destroy(ui); return ESP_ERR_NO_MEM; }
    lv_timer_pause(ui->tick);
    visible(ui->root, false);
    return ESP_OK;
}
