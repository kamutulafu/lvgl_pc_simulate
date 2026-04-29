/**
 * ui_common.c
 * Shared style helpers, screen manager, and key-event dispatcher.
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

/* ── Demo gas data ───────────────────────────────── */
gas_ch_t g_gas[5] = {
    { "CO  一氧化碳", "CO",  "ppm",   23.5f,  30.0f,  50.0f, 150.0f, GAS_NORMAL },
    { "H2S 硫化氢",  "H2S", "ppm",    8.1f,   5.0f,  10.0f,  15.0f, GAS_WARN   },
    { "O2  氧气",    "O2",  "%VOL",  20.9f,  19.5f,  18.0f,  25.0f, GAS_NORMAL },
    { "CH4 甲烷",    "CH4", "%LEL",   1.2f,  10.0f,  20.0f, 100.0f, GAS_NORMAL },
    { "NO2 二氧化氮","NO2", "ppm",    0.3f,   1.0f,   3.0f,   5.0f, GAS_NORMAL },
};
int g_gas_count = 5;  /* change to 1‥5 to test each layout */

/* ── Screen objects ──────────────────────────────── */
static lv_obj_t *scr_home;
static lv_obj_t *scr_password;
static lv_obj_t *scr_menu;

ui_page_t cur_page = PAGE_HOME;

static void ui_key_catcher_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    switch (key) {
    case LV_KEY_UP:
        ui_key_event(KEY_UP);
        break;
    case LV_KEY_DOWN:
        ui_key_event(KEY_DOWN);
        break;
    case LV_KEY_RIGHT:
    case LV_KEY_ENTER:
        ui_key_event(KEY_OK);
        break;
    case LV_KEY_LEFT:
    case LV_KEY_ESC:
        ui_key_event(KEY_ESC);
        break;
    default:
        break;
    }
}

/* ─────────────────────────────────────────────────
 *  STYLE HELPERS
 * ───────────────────────────────────────────────── */

/* Apply the common dark background to any screen */
void ui_style_screen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, C_BG_SCREEN, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
}

/* Create the 30 px top status bar */
lv_obj_t *ui_topbar_create(lv_obj_t *parent,
                            const char *title,
                            const char *badge_text,
                            lv_color_t  badge_bg,
                            lv_color_t  badge_col)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCR_W, 30);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, C_BG_TOPBAR, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x1E3A52), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_pad_hor(bar, 8, 0);
    lv_obj_set_style_pad_ver(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label */
    lv_obj_t *lbl_title = lv_label_create(bar);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_color(lbl_title, C_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_title, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    /* Right cluster: temp | date | badge */
    lv_obj_t *lbl_temp = lv_label_create(bar);
    lv_label_set_text(lbl_temp, "24.3°C");
    lv_obj_set_style_text_color(lbl_temp, C_TEXT_TEMP, 0);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_RIGHT_MID, -90, 0);

    lv_obj_t *lbl_date = lv_label_create(bar);
    lv_label_set_text(lbl_date, "04-28");
    lv_obj_set_style_text_color(lbl_date, C_TEXT_DATE, 0);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_date, LV_ALIGN_RIGHT_MID, -45, 0);

    /* Badge pill */
    lv_obj_t *badge = lv_obj_create(bar);
    lv_obj_set_size(badge, LV_SIZE_CONTENT, 16);
    lv_obj_align(badge, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(badge, badge_bg, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_radius(badge, 8, 0);
    lv_obj_set_style_pad_hor(badge, 5, 0);
    lv_obj_set_style_pad_ver(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_badge = lv_label_create(badge);
    lv_label_set_text(lbl_badge, badge_text);
    lv_obj_set_style_text_color(lbl_badge, badge_col, 0);
    lv_obj_set_style_text_font(lbl_badge, UI_FONT_CJK_14, 0);
    lv_obj_center(lbl_badge);

    return bar;
}

/* Create the 22 px bottom hint bar */
lv_obj_t *ui_hintbar_create(lv_obj_t *parent,
                             const char *h_up,
                             const char *h_dn,
                             const char *h_ok,
                             const char *h_esc)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCR_W, 22);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0C1520), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_pad_hor(bar, 4, 0);
    lv_obj_set_style_pad_ver(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    const char *hints[4] = { h_up, h_dn, h_ok, h_esc };
    for (int i = 0; i < 4; i++) {
        if (!hints[i] || hints[i][0] == '\0') continue;
        lv_obj_t *lbl = lv_label_create(bar);
        lv_label_set_text(lbl, hints[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x556A7A), 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_14, 0);
        /* Evenly space 4 slots across 320 px */
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4 + i * 78, 0);
    }
    return bar;
}

/* Make a dark card container */
lv_obj_t *ui_card_create(lv_obj_t *parent, int x, int y, int w, int h,
                          lv_color_t bg, lv_color_t border)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, bg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, border, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 4, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* Colour helpers */
lv_color_t ui_status_color(gas_status_t s)
{
    if (s == GAS_ALARM) return C_ALARM;
    if (s == GAS_WARN)  return C_WARN;
    return C_OK;
}
lv_color_t ui_status_badge_bg(gas_status_t s)
{
    if (s == GAS_ALARM) return C_BADGE_ALARM_BG;
    if (s == GAS_WARN)  return C_BADGE_WARN_BG;
    return C_BADGE_OK_BG;
}
const char *ui_status_text(gas_status_t s)
{
    if (s == GAS_ALARM) return "报警";
    if (s == GAS_WARN)  return "注意";
    return "正常";
}

/* ─────────────────────────────────────────────────
 *  SCREEN MANAGER
 * ───────────────────────────────────────────────── */

void ui_goto(ui_page_t page)
{
    lv_obj_t *target = NULL;
    if      (page == PAGE_HOME)     target = scr_home;
    else if (page == PAGE_PASSWORD) target = scr_password;
    else if (page == PAGE_MENU)     target = scr_menu;

    if (target) {
        lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_FADE_ON, 150, 0, false);
        lv_group_t *group = lv_group_get_default();
        if (group) lv_group_focus_obj(target);
        cur_page = page;
    }
}

/* ─────────────────────────────────────────────────
 *  TOP-LEVEL INIT  (call once from main)
 * ───────────────────────────────────────────────── */

void ui_init(void)
{
    scr_home     = lv_obj_create(NULL);
    scr_password = lv_obj_create(NULL);
    scr_menu     = lv_obj_create(NULL);

    ui_style_screen(scr_home);
    ui_style_screen(scr_password);
    ui_style_screen(scr_menu);

    lv_obj_add_event_cb(scr_home, ui_key_catcher_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(scr_password, ui_key_catcher_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(scr_menu, ui_key_catcher_event_cb, LV_EVENT_KEY, NULL);

    lv_group_t *group = lv_group_get_default();
    if (group) {
        lv_group_add_obj(group, scr_home);
        lv_group_add_obj(group, scr_password);
        lv_group_add_obj(group, scr_menu);
    }

    ui_page_home_create(scr_home);
    ui_page_password_create(scr_password);
    ui_page_menu_create(scr_menu);

    ui_goto(PAGE_HOME);
}

void ui_destroy(void)
{
    if (scr_home) {
        lv_obj_delete(scr_home);
        scr_home = NULL;
    }
    if (scr_password) {
        lv_obj_delete(scr_password);
        scr_password = NULL;
    }
    if (scr_menu) {
        lv_obj_delete(scr_menu);
        scr_menu = NULL;
    }
}
