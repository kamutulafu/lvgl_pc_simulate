/**
 * ui_home.c
 * Main monitoring screen.
 * Adaptive layout for 1 – 5 gas channels.
 *
 * Content area: y=30 … y=218  (height = 188 px)
 * Hint bar    : y=218 … y=240 (height = 22 px)
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

int g_selected_gas = -1;
static lv_obj_t *card_objs[5];

/* ── live-update handles ─────────────────────────── */
static lv_obj_t *h_val [5];   /* value labels          */
static lv_obj_t *h_bar [5];   /* lv_bar indicators     */
static lv_obj_t *h_badge[5];  /* status badge labels   */

#define CONTENT_Y   30
#define CONTENT_H   188
#define PAD         5

static void home_update_selection(void)
{
    for (int i = 0; i < g_gas_count; i++) {
        if (!card_objs[i]) continue;
        if (i == g_selected_gas) {
            lv_obj_set_style_border_color(card_objs[i], C_PW_BORDER, 0);
            lv_obj_set_style_border_width(card_objs[i], 2, 0);
        } else {
            // Requirement 2: do not show selection borders on unselected cards to avoid conflict
            lv_obj_set_style_border_color(card_objs[i], C_BORDER, 0);
            lv_obj_set_style_border_width(card_objs[i], 1, 0);
        }
    }
}

void ui_home_key(ui_key_t key)
{
    if (key == KEY_LEFT) key = KEY_ESC;
    else if (key == KEY_RIGHT) key = KEY_OK;

    if (key == KEY_UP) {
        if (g_selected_gas == -1) {
            g_selected_gas = g_gas_count - 1;
        } else {
            g_selected_gas = (g_selected_gas - 1 + g_gas_count) % g_gas_count;
        }
        home_update_selection();
    }
    else if (key == KEY_DOWN) {
        if (g_selected_gas == -1) {
            g_selected_gas = 0;
        } else {
            g_selected_gas = (g_selected_gas + 1) % g_gas_count;
        }
        home_update_selection();
    }
    else if (key == KEY_OK) {
        if (g_selected_gas == -1) {
            ui_goto(PAGE_PASSWORD);
        } else {
            ui_goto(PAGE_CURVE);
        }
    }
    else if (key == KEY_ESC) {
        if (g_selected_gas != -1) {
            g_selected_gas = -1;
            home_update_selection();
        }
    }
}

/* ── common font shortcuts ─────────────────────────*/
#define FN10  (&lv_font_montserrat_10)
#define FN12  (&lv_font_montserrat_12)
#define FN14  (&lv_font_montserrat_14)
#define FN16  (&lv_font_montserrat_16)
#define FN20  (&lv_font_montserrat_20)
#define FN24  (&lv_font_montserrat_24)
#define FN28  (&lv_font_montserrat_28)
#define FN32  (&lv_font_montserrat_32)
#define FN40  (&lv_font_montserrat_40)
#define FN48  (&lv_font_montserrat_48)

/* ─────────────────────────────────────────────────
 *  Internal: add value + unit + bar + badge to card
 * ───────────────────────────────────────────────── */
static void card_add_content(lv_obj_t *card, int idx,
                              const lv_font_t *val_font,
                              int name_y, int val_y,
                              int bar_y, int bar_h,
                              int badge_y, bool show_name)
{
    gas_ch_t *g = &g_gas[idx];
    // Requirement 2: green for normal/warn, red for alarm
    lv_color_t col = (g->status == GAS_ALARM) ? C_ALARM : C_OK;

    if (show_name) {
        lv_obj_t *lbl_name = lv_label_create(card);
        lv_label_set_text(lbl_name, ui_get_text(g->name));
        lv_obj_set_style_text_color(lbl_name, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl_name, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 6, name_y);
    }

    /* Value label */
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", g->value);
    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, buf);
    lv_obj_set_style_text_color(lbl_val, col, 0);
    lv_obj_set_style_text_font(lbl_val, val_font, 0);
    lv_obj_align(lbl_val, LV_ALIGN_TOP_LEFT, 6, val_y);
    h_val[idx] = lbl_val;

    /* Unit label (right of value, small) */
    lv_obj_t *lbl_unit = lv_label_create(card);
    lv_label_set_text(lbl_unit, g->unit);
    lv_obj_set_style_text_color(lbl_unit, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lbl_unit, FN10, 0);
    lv_obj_align_to(lbl_unit, lbl_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, 0);

    /* Progress bar */
    lv_obj_t *bar = lv_bar_create(card);
    lv_obj_set_size(bar, lv_obj_get_width(card) - 12, bar_h);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 6, bar_y);
    lv_obj_set_style_bg_color(bar, C_BAR_BG, 0);
    lv_obj_set_style_bg_color(bar, col, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_radius(bar, 2, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, (int)(g->range_max * 10));
    lv_bar_set_value(bar, (int)(g->value * 10), LV_ANIM_OFF);
    h_bar[idx] = bar;

    /* Status badge - REMOVED per user requirement 1 */
    h_badge[idx] = NULL;
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 1 – single gas, full-screen large display
 * ═══════════════════════════════════════════════════ */
static void layout_1gas(lv_obj_t *parent)
{
    gas_ch_t *g = &g_gas[0];
    // Requirement 2: green for normal/warn, red for alarm
    lv_color_t col = (g->status == GAS_ALARM) ? C_ALARM : C_OK;

    /* Gas name */
    lv_obj_t *ln = lv_label_create(parent);
    lv_label_set_text(ln, ui_get_text(g->name));
    lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(ln, UI_FONT_CJK_24, 0);
    lv_obj_align(ln, LV_ALIGN_TOP_MID, 0, CONTENT_Y + 18);

    /* Giant value */
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", g->value);
    lv_obj_t *lv = lv_label_create(parent);
    lv_label_set_text(lv, buf);
    lv_obj_set_style_text_color(lv, col, 0);
    lv_obj_set_style_text_font(lv, FN48, 0);
    lv_obj_align(lv, LV_ALIGN_TOP_MID, 0, CONTENT_Y + 36);
    h_val[0] = lv;

    lv_obj_t *lu = lv_label_create(parent);
    lv_label_set_text(lu, g->unit);
    lv_obj_set_style_text_color(lu, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lu, FN14, 0);
    lv_obj_align_to(lu, lv, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);

    /* Wide progress bar */
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, SCR_W - 32, 8);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, CONTENT_Y + 106);
    lv_obj_set_style_bg_color(bar, C_BAR_BG, 0);
    lv_obj_set_style_bg_color(bar, col, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, 0);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, (int)(g->range_max * 10));
    lv_bar_set_value(bar, (int)(g->value * 10), LV_ANIM_OFF);
    h_bar[0] = bar;

    /* Min / max / alarm row */
    const char *labels[3] = { ui_get_text("最低"), ui_get_text("最高"), ui_get_text("报警值") };
    float vals[3] = { g->value * 0.6f, g->value * 1.8f, g->alarm_hi };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *ll = lv_label_create(parent);
        lv_label_set_text(ll, labels[i]);
        lv_obj_set_style_text_color(ll, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(ll, UI_FONT_CJK_24, 0);
        lv_obj_align(ll, LV_ALIGN_TOP_LEFT, 30 + i * 90, CONTENT_Y + 124);

        char vb[12];
        snprintf(vb, sizeof(vb), "%.1f", vals[i]);
        lv_obj_t *lv2 = lv_label_create(parent);
        lv_label_set_text(lv2, vb);
        lv_color_t vc = (i == 2) ? C_ALARM : (i == 1 ? C_WARN : C_TEXT_SEC);
        lv_obj_set_style_text_color(lv2, vc, 0);
        lv_obj_set_style_text_font(lv2, FN12, 0);
        lv_obj_align(lv2, LV_ALIGN_TOP_LEFT, 30 + i * 90, CONTENT_Y + 137);
    }

    /* Status badge - REMOVED per user requirement 1 */
    h_badge[0] = NULL;
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 2 – two channels, split top / bottom
 * ═══════════════════════════════════════════════════ */
static void layout_2gas(lv_obj_t *parent)
{
    int row_h = CONTENT_H / 2;  /* 94 px each */
    for (int i = 0; i < 2; i++) {
        int y = CONTENT_Y + i * row_h;
        lv_obj_t *card = lv_obj_create(parent);
        card_objs[i] = card;
        lv_obj_set_pos(card, 0, y);
        lv_obj_set_size(card, SCR_W, row_h);
        lv_obj_set_style_bg_color(card, C_BG_SCREEN, 0);
        lv_obj_set_style_border_width(card, i == 0 ? 1 : 0, 0);
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x1E3A52), 0);
        lv_obj_set_style_radius(card, 0, 0);
        lv_obj_set_style_pad_all(card, 0, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* Name */
        lv_obj_t *ln = lv_label_create(card);
        lv_label_set_text(ln, ui_get_text(g_gas[i].name));
        lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(ln, UI_FONT_CJK_24, 0);
        lv_obj_align(ln, LV_ALIGN_TOP_LEFT, 14, 10);

        card_add_content(card, i, FN32, 0, 22, 68, 4, 10, false);
    }
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 3 – 1 large (top 55%) + 2 small (bottom)
 * ═══════════════════════════════════════════════════ */
static void layout_3gas(lv_obj_t *parent)
{
    int big_h  = 112;
    int sml_h  = CONTENT_H - big_h - PAD * 2;  /* ~66 */
    int sml_w  = (SCR_W - PAD * 3) / 2;

    /* Big card */
    lv_color_t big_bg = (g_gas[0].status == GAS_ALARM) ? C_BG_CARD_ALT : C_BG_CARD;
    lv_obj_t *big = ui_card_create(parent, PAD, CONTENT_Y + PAD,
                                   SCR_W - PAD*2, big_h, big_bg, C_BORDER);
    card_objs[0] = big;
    lv_obj_t *ln = lv_label_create(big);
    lv_label_set_text(ln, ui_get_text(g_gas[0].name));
    lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(ln, UI_FONT_CJK_24, 0);
    lv_obj_align(ln, LV_ALIGN_TOP_LEFT, 6, 6);
    card_add_content(big, 0, FN48, 0, 20, 80, 5, 6, false);

    /* 2 small cards */
    for (int i = 1; i <= 2; i++) {
        int x = PAD + (i-1) * (sml_w + PAD);
        int y = CONTENT_Y + PAD + big_h + PAD;
        lv_obj_t *c = ui_card_create(parent, x, y, sml_w, sml_h, C_BG_CARD, C_BORDER);
        card_objs[i] = c;
        lv_obj_t *ls = lv_label_create(c);
        lv_label_set_text(ls, g_gas[i].symbol);
        lv_obj_set_style_text_color(ls, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(ls, FN10, 0);
        lv_obj_align(ls, LV_ALIGN_TOP_LEFT, 6, 5);
        card_add_content(c, i, FN24, 0, 18, 50, 3, 5, false);
    }
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 4 – 2×2 equal grid
 * ═══════════════════════════════════════════════════ */
static void layout_4gas(lv_obj_t *parent)
{
    int card_w = (SCR_W - PAD * 3) / 2;
    int card_h = (CONTENT_H - PAD * 3) / 2;

    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = PAD + col * (card_w + PAD);
        int y = CONTENT_Y + PAD + row * (card_h + PAD);

        gas_ch_t *g = &g_gas[i];
        lv_color_t bg = (g->status == GAS_ALARM) ? C_BG_CARD_ALT : C_BG_CARD;
        lv_obj_t *c = ui_card_create(parent, x, y, card_w, card_h, bg, C_BORDER);
        card_objs[i] = c;

        /* 1. Gas Symbol (e.g. CO) */
        lv_obj_t *lbl_symbol = lv_label_create(c);
        lv_label_set_text(lbl_symbol, g->symbol);
        lv_obj_set_style_text_color(lbl_symbol, C_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_symbol, FN16, 0);
        lv_obj_align(lbl_symbol, LV_ALIGN_TOP_LEFT, 10, 8);

        /* 2. Gas Chinese/Translated Name next to symbol */
        const char *full_name = ui_get_text(g->name);
        const char *name_part = strchr(full_name, ' ');
        while (name_part && *name_part == ' ') name_part++;
        if (!name_part) name_part = full_name;

        lv_obj_t *lbl_name = lv_label_create(c);
        lv_label_set_text(lbl_name, name_part);
        lv_obj_set_style_text_color(lbl_name, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl_name, UI_FONT_CJK_14, 0);
        lv_obj_align_to(lbl_name, lbl_symbol, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, -2);

        /* 3. Value Label */
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", g->value);
        lv_color_t val_col = (g->status == GAS_ALARM) ? C_ALARM : C_OK;

        lv_obj_t *lbl_val = lv_label_create(c);
        lv_label_set_text(lbl_val, buf);
        lv_obj_set_style_text_color(lbl_val, val_col, 0);
        lv_obj_set_style_text_font(lbl_val, FN28, 0);
        lv_obj_align(lbl_val, LV_ALIGN_TOP_LEFT, 10, 30);
        h_val[i] = lbl_val;

        /* 4. Unit Label */
        lv_obj_t *lbl_unit = lv_label_create(c);
        lv_label_set_text(lbl_unit, g->unit);
        lv_obj_set_style_text_color(lbl_unit, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl_unit, FN14, 0);
        lv_obj_align_to(lbl_unit, lbl_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -4);

        /* 5. Progress Bar */
        lv_obj_t *bar = lv_bar_create(c);
        lv_obj_set_size(bar, card_w - 20, 4);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_obj_set_style_bg_color(bar, C_BAR_BG, 0);
        lv_color_t ind_col = ui_status_color(g->status);
        lv_obj_set_style_bg_color(bar, ind_col, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_radius(bar, 2, LV_PART_INDICATOR);
        lv_bar_set_range(bar, 0, (int)(g->range_max * 10));
        lv_bar_set_value(bar, (int)(g->value * 10), LV_ANIM_OFF);
        h_bar[i] = bar;

        h_badge[i] = NULL;
    }
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 5 – 5-row list layout
 * ═══════════════════════════════════════════════════ */
static void layout_5gas(lv_obj_t *parent)
{
    int card_h = 35;
    int row_spacing = 37;
    int start_y = CONTENT_Y + 2;

    for (int i = 0; i < 5; i++) {
        gas_ch_t *g = &g_gas[i];
        lv_color_t col = (g->status == GAS_ALARM) ? C_ALARM : C_OK;
        
        lv_color_t bg = (g->status == GAS_ALARM) ? C_BG_CARD_ALT : C_BG_CARD;
        
        lv_obj_t *card = ui_card_create(parent, 4, start_y + i * row_spacing, SCR_W - 8, card_h, bg, C_BORDER);
        card_objs[i] = card;
        
        // Status indicator pill on the left
        lv_obj_t *indicator = lv_obj_create(card);
        lv_obj_set_size(indicator, 4, 20);
        lv_obj_align(indicator, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_bg_color(indicator, ui_status_color(g->status), 0);
        lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(indicator, 0, 0);
        lv_obj_set_style_radius(indicator, 2, 0);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_SCROLLABLE);

        // 1. Gas Name (CO 一氧化碳) - Left Aligned
        lv_obj_t *lbl_name = lv_label_create(card);
        lv_label_set_text(lbl_name, ui_get_text(g->name));
        lv_obj_set_style_text_color(lbl_name, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(lbl_name, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 16, 0);
        
        // 2. Value - Right-aligned (aligned near the right side, size 24)
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", g->value);
        lv_obj_t *lbl_val = lv_label_create(card);
        lv_label_set_text(lbl_val, buf);
        lv_obj_set_style_text_color(lbl_val, col, 0);
        lv_obj_set_style_text_font(lbl_val, FN24, 0);
        lv_obj_align(lbl_val, LV_ALIGN_RIGHT_MID, -45, 0);
        h_val[i] = lbl_val;
        
        // 3. Unit - Positioned right next to Value
        lv_obj_t *lbl_unit = lv_label_create(card);
        lv_label_set_text(lbl_unit, g->unit);
        lv_obj_set_style_text_color(lbl_unit, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl_unit, FN10, 0);
        lv_obj_align_to(lbl_unit, lbl_val, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -2);
        
        h_badge[i] = NULL;
        h_bar[i] = NULL;
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: create home screen
 * ───────────────────────────────────────────────── */
void ui_page_home_create(lv_obj_t *parent)
{
    memset(h_val, 0, sizeof(h_val));
    memset(h_bar, 0, sizeof(h_bar));
    memset(h_badge, 0, sizeof(h_badge));
    memset(card_objs, 0, sizeof(card_objs));
    g_selected_gas = -1;

    /* Top bar */
    ui_topbar_create(parent, "气体检测仪", "运行中",
                     lv_color_hex(0x1A3A20), C_OK);

    /* Gas content area – pick layout by channel count */
    switch (g_gas_count) {
        case 1: layout_1gas(parent); break;
        case 2: layout_2gas(parent); break;
        case 3: layout_3gas(parent); break;
        case 4: layout_4gas(parent); break;
        default: layout_5gas(parent); break;
    }

    /* Bottom hint */
    ui_hintbar_create(parent, "", "", "OK 菜单", "");
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: periodic refresh (call every ~500 ms)
 * ───────────────────────────────────────────────── */
void ui_refresh_home(void)
{
    /* Re-read from sensor driver into g_gas[], then: */
    char buf[16];

    if (cur_page != PAGE_HOME) return;

    for (int i = 0; i < g_gas_count; i++) {
        if (!h_val[i]) continue;
        snprintf(buf, sizeof(buf), "%.1f", g_gas[i].value);
        lv_label_set_text(h_val[i], buf);

        // Requirement 2: green for normal/warn, red for alarm
        lv_color_t val_col = (g_gas[i].status == GAS_ALARM) ? C_ALARM : C_OK;
        lv_obj_set_style_text_color(h_val[i], val_col, 0);

        lv_color_t col = ui_status_color(g_gas[i].status);
        if (h_bar[i]) {
            lv_obj_set_style_bg_color(h_bar[i], col, LV_PART_INDICATOR);
            lv_bar_set_value(h_bar[i],
                             (int)(g_gas[i].value * 10), LV_ANIM_OFF);
        }
        if (h_badge[i]) {
            lv_label_set_text(h_badge[i], ui_status_text(g_gas[i].status));
            lv_obj_set_style_text_color(h_badge[i], col, 0);
        }
    }
    home_update_selection();
}
