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

/* ── live-update handles ─────────────────────────── */
static lv_obj_t *h_val [5];   /* value labels          */
static lv_obj_t *h_bar [5];   /* lv_bar indicators     */
static lv_obj_t *h_badge[5];  /* status badge labels   */

#define CONTENT_Y   30
#define CONTENT_H   188
#define PAD         5

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
    lv_color_t col = ui_status_color(g->status);

    if (show_name) {
        lv_obj_t *lbl_name = lv_label_create(card);
        lv_label_set_text(lbl_name, g->name);
        lv_obj_set_style_text_color(lbl_name, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl_name, UI_FONT_CJK_14, 0);
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

    /* Status badge */
    lv_obj_t *badge_bg = lv_obj_create(card);
    lv_obj_set_size(badge_bg, LV_SIZE_CONTENT, 14);
    lv_obj_align(badge_bg, LV_ALIGN_TOP_RIGHT, -4, badge_y);
    lv_obj_set_style_bg_color(badge_bg, ui_status_badge_bg(g->status), 0);
    lv_obj_set_style_bg_opa(badge_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge_bg, 0, 0);
    lv_obj_set_style_radius(badge_bg, 7, 0);
    lv_obj_set_style_pad_hor(badge_bg, 5, 0);
    lv_obj_set_style_pad_ver(badge_bg, 0, 0);
    lv_obj_clear_flag(badge_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_badge = lv_label_create(badge_bg);
    lv_label_set_text(lbl_badge, ui_status_text(g->status));
    lv_obj_set_style_text_color(lbl_badge, col, 0);
    lv_obj_set_style_text_font(lbl_badge, UI_FONT_CJK_14, 0);
    lv_obj_center(lbl_badge);
    h_badge[idx] = lbl_badge;
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 1 – single gas, full-screen large display
 * ═══════════════════════════════════════════════════ */
static void layout_1gas(lv_obj_t *parent)
{
    gas_ch_t *g = &g_gas[0];
    lv_color_t col = ui_status_color(g->status);

    /* Gas name */
    lv_obj_t *ln = lv_label_create(parent);
    lv_label_set_text(ln, g->name);
    lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(ln, UI_FONT_CJK_14, 0);
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
    const char *labels[3] = { "最低", "最高", "报警值" };
    float vals[3] = { g->value * 0.6f, g->value * 1.8f, g->alarm_hi };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *ll = lv_label_create(parent);
        lv_label_set_text(ll, labels[i]);
        lv_obj_set_style_text_color(ll, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(ll, UI_FONT_CJK_14, 0);
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

    /* Badge */
    lv_obj_t *bb = lv_obj_create(parent);
    lv_obj_set_size(bb, LV_SIZE_CONTENT, 16);
    lv_obj_align(bb, LV_ALIGN_TOP_MID, 0, CONTENT_Y + 92);
    lv_obj_set_style_bg_color(bb, ui_status_badge_bg(g->status), 0);
    lv_obj_set_style_bg_opa(bb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bb, 0, 0);
    lv_obj_set_style_radius(bb, 8, 0);
    lv_obj_set_style_pad_hor(bb, 8, 0);
    lv_obj_set_style_pad_ver(bb, 0, 0);
    lv_obj_clear_flag(bb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *lb = lv_label_create(bb);
    lv_label_set_text(lb, ui_status_text(g->status));
    lv_obj_set_style_text_color(lb, col, 0);
    lv_obj_set_style_text_font(lb, UI_FONT_CJK_14, 0);
    lv_obj_center(lb);
    h_badge[0] = lb;
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
        lv_label_set_text(ln, g_gas[i].name);
        lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(ln, UI_FONT_CJK_14, 0);
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
    lv_color_t big_bd = (g_gas[0].status == GAS_ALARM) ? C_BORDER_ALARM : C_BORDER;
    lv_obj_t *big = ui_card_create(parent, PAD, CONTENT_Y + PAD,
                                   SCR_W - PAD*2, big_h, big_bg, big_bd);
    lv_obj_t *ln = lv_label_create(big);
    lv_label_set_text(ln, g_gas[0].name);
    lv_obj_set_style_text_color(ln, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(ln, UI_FONT_CJK_14, 0);
    lv_obj_align(ln, LV_ALIGN_TOP_LEFT, 6, 6);
    card_add_content(big, 0, FN48, 0, 20, 80, 5, 6, false);

    /* 2 small cards */
    for (int i = 1; i <= 2; i++) {
        int x = PAD + (i-1) * (sml_w + PAD);
        int y = CONTENT_Y + PAD + big_h + PAD;
        lv_obj_t *c = ui_card_create(parent, x, y, sml_w, sml_h, C_BG_CARD, C_BORDER);
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

        lv_color_t bg = (g_gas[i].status == GAS_ALARM) ? C_BG_CARD_ALT : C_BG_CARD;
        lv_color_t bd = (g_gas[i].status == GAS_ALARM) ? C_BORDER_ALARM : C_BORDER;
        lv_obj_t *c = ui_card_create(parent, x, y, card_w, card_h, bg, bd);

        lv_obj_t *ls = lv_label_create(c);
        lv_label_set_text(ls, g_gas[i].symbol);
        lv_obj_set_style_text_color(ls, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(ls, FN10, 0);
        lv_obj_align(ls, LV_ALIGN_TOP_LEFT, 6, 5);

        card_add_content(c, i, FN28, 0, 18, 56, 3, 5, false);
    }
}

/* ═══════════════════════════════════════════════════
 *  LAYOUT 5 – 1 alarm-priority large + 2×2 compact
 * ═══════════════════════════════════════════════════ */
static void layout_5gas(lv_obj_t *parent)
{
    int top_h  = 80;
    int sml_h  = (CONTENT_H - top_h - PAD * 3) / 2;
    int sml_w  = (SCR_W - PAD * 3) / 2;

    /* Top big card – always show highest-priority gas (idx 0) */
    lv_color_t tbg = (g_gas[0].status == GAS_ALARM) ? C_BG_CARD_ALT : C_BG_CARD;
    lv_color_t tbd = (g_gas[0].status == GAS_ALARM) ? C_BORDER_ALARM : C_BORDER;
    lv_obj_t *top = ui_card_create(parent, PAD, CONTENT_Y + PAD,
                                   SCR_W - PAD*2, top_h, tbg, tbd);
    lv_obj_t *ln = lv_label_create(top);
    lv_label_set_text(ln, g_gas[0].name);
    lv_obj_set_style_text_color(ln, ui_status_color(g_gas[0].status), 0);
    lv_obj_set_style_text_font(ln, UI_FONT_CJK_14, 0);
    lv_obj_align(ln, LV_ALIGN_TOP_LEFT, 8, 5);
    card_add_content(top, 0, FN40, 0, 14, 66, 4, 5, false);

    /* 2×2 small cards for channels 1‥4 */
    for (int i = 1; i <= 4; i++) {
        int col = (i-1) % 2;
        int row = (i-1) / 2;
        int x = PAD + col * (sml_w + PAD);
        int y = CONTENT_Y + PAD + top_h + PAD + row * (sml_h + PAD);
        lv_obj_t *c = ui_card_create(parent, x, y, sml_w, sml_h, C_BG_CARD, C_BORDER);

        lv_obj_t *ls = lv_label_create(c);
        lv_label_set_text(ls, g_gas[i].symbol);
        lv_obj_set_style_text_color(ls, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(ls, FN10, 0);
        lv_obj_align(ls, LV_ALIGN_TOP_LEFT, 5, 4);

        card_add_content(c, i, FN20, 0, 16, 44, 3, 4, false);
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: create home screen
 * ───────────────────────────────────────────────── */
void ui_page_home_create(lv_obj_t *parent)
{
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
    for (int i = 0; i < g_gas_count; i++) {
        if (!h_val[i]) continue;
        snprintf(buf, sizeof(buf), "%.1f", g_gas[i].value);
        lv_label_set_text(h_val[i], buf);

        lv_color_t col = ui_status_color(g_gas[i].status);
        lv_obj_set_style_text_color(h_val[i], col, 0);

        if (h_bar[i]) {
            lv_obj_set_style_bg_color(h_bar[i], col, LV_PART_INDICATOR);
            lv_bar_set_value(h_bar[i],
                             (int)(g_gas[i].value * 10), LV_ANIM_ON);
        }
        if (h_badge[i]) {
            lv_label_set_text(h_badge[i], ui_status_text(g_gas[i].status));
            lv_obj_set_style_text_color(h_badge[i], col, 0);
        }
    }
}
