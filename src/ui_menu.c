/**
 * ui_menu.c
 * System settings menu — reached after successful password entry.
 *
 * KEY_UP / KEY_DOWN  : move selection cursor
 * KEY_OK             : enter sub-screen (stub – extend as needed)
 * KEY_ESC            : return to home screen
 */

#include "ui.h"
#include <string.h>

/* ── Menu items ──────────────────────────────────── */
typedef struct {
    const char *icon;   /* LVGL symbol or short text */
    const char *label;
} menu_item_t;

static const menu_item_t MENU_ITEMS[] = {
    { LV_SYMBOL_SETTINGS,  "参数设置"   },
    { LV_SYMBOL_WARNING,   "报警阈值"   },
    { LV_SYMBOL_REFRESH,   "传感器校准" },
    { LV_SYMBOL_SAVE,      "数据记录"   },
    { LV_SYMBOL_LIST,      "系统信息"   },
};
#define MENU_COUNT  (sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]))

#define ROW_H    33
#define MENU_X   0
#define MENU_Y   30   /* below topbar */

/* ── State ───────────────────────────────────────── */
static int menu_sel = 0;
static int menu_top = 0;

/* ── Widget handles ──────────────────────────────── */
#define VISIBLE_ROWS 4
static lv_obj_t *menu_rows[VISIBLE_ROWS];
static lv_obj_t *menu_lbls[VISIBLE_ROWS];
static lv_obj_t *menu_icons[VISIBLE_ROWS];

/* ─────────────────────────────────────────────────
 *  Redraw selection highlight
 * ───────────────────────────────────────────────── */
static void menu_redraw(void)
{
    for (int i = 0; i < VISIBLE_ROWS; i++) {
        int item_idx = menu_top + i;
        if (item_idx < (int)MENU_COUNT) {
            lv_obj_remove_flag(menu_rows[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(menu_lbls[i], ui_get_text(MENU_ITEMS[item_idx].label));
            lv_label_set_text(menu_icons[i], MENU_ITEMS[item_idx].icon);
            
            bool sel = (item_idx == menu_sel);
            lv_obj_set_style_bg_color(menu_rows[i],
                sel ? C_PW_ACTIVE : C_BG_CARD, 0);
            lv_obj_set_style_border_color(menu_rows[i],
                sel ? C_PW_BORDER : C_BORDER, 0);
            lv_obj_set_style_text_color(menu_lbls[i],
                sel ? C_TEXT_PRI : C_TEXT_SEC, 0);
        } else {
            lv_obj_add_flag(menu_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: key handler
 * ───────────────────────────────────────────────── */
void ui_menu_key(ui_key_t key)
{
    switch (key) {
    case KEY_UP:
        if (menu_sel > 0) {
            menu_sel--;
            if (menu_sel < menu_top) menu_top = menu_sel;
            menu_redraw();
        }
        break;
    case KEY_DOWN:
        if (menu_sel < (int)MENU_COUNT - 1) {
            menu_sel++;
            if (menu_sel >= menu_top + VISIBLE_ROWS) menu_top = menu_sel - VISIBLE_ROWS + 1;
            menu_redraw();
        }
        break;
    case KEY_OK:
        if (menu_sel == 0) ui_goto(PAGE_PARAM);
        break;
    case KEY_ESC:
        menu_sel = 0;
        menu_top = 0;
        menu_redraw();
        ui_goto(PAGE_HOME);
        break;
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: create menu screen
 * ───────────────────────────────────────────────── */
void ui_page_menu_create(lv_obj_t *parent)
{
    menu_sel = 0;
    menu_top = 0;

    ui_topbar_create(parent, ui_get_text("系统菜单"), ui_get_text("已验证"),
                     C_BADGE_OK_BG, C_OK);

    int content_h = SCR_H - 30 - 22;  /* 188 px */
    int row_h_px  = content_h / VISIBLE_ROWS;  /* evenly divide */

    for (int i = 0; i < VISIBLE_ROWS; i++) {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_pos(row, MENU_X, MENU_Y + i * row_h_px);
        lv_obj_set_size(row, SCR_W, row_h_px);
        lv_obj_set_style_bg_color(row, C_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        menu_rows[i] = row;

        /* Cursor marker */
        lv_obj_t *cursor = lv_label_create(row);
        lv_label_set_text(cursor, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(cursor, C_ACCENT, 0);
        lv_obj_set_style_text_font(cursor, &lv_font_montserrat_14, 0);
        lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 10, 0);

        /* Icon */
        lv_obj_t *icon = lv_label_create(row);
        lv_obj_set_style_text_color(icon, C_ACCENT, 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 30, 0);
        menu_icons[i] = icon;

        /* Label */
        lv_obj_t *lbl = lv_label_create(row);
        lv_obj_set_style_text_color(lbl, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 56, 0);
        menu_lbls[i] = lbl;

        /* Chevron (right arrow hint) */
        lv_obj_t *chev = lv_label_create(row);
        lv_label_set_text(chev, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(chev, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(chev, &lv_font_montserrat_14, 0);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    menu_redraw();

    ui_hintbar_create(parent, "▲▼ 选择", "", "OK 进入", "ESC 返回");
}

/* ═════════════════════════════════════════════════════════════
 *  ui_key.c – Central key event dispatcher
 *  Call this from your BSP button IRQ or polling task.
 * ═════════════════════════════════════════════════════════════ */

/* Forward declarations from other translation units */
extern void ui_pw_key(ui_key_t key);
extern void ui_menu_key(ui_key_t key);
extern void ui_param_key(ui_key_t key);

/* Current page is tracked in ui_common.c; expose it: */
extern ui_page_t cur_page;   /* defined in ui_common.c as static – move to .h if needed */

void ui_key_event(ui_key_t key)
{
    switch (cur_page) {
    case PAGE_HOME:
        ui_home_key(key);
        break;
    case PAGE_PASSWORD:
        ui_pw_key(key);
        break;
    case PAGE_MENU:
        ui_menu_key(key);
        break;
    case PAGE_PARAM:
        ui_param_key(key);
        break;
    case PAGE_CURVE:
        ui_curve_key(key);
        break;
    }
}
