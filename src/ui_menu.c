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

/* ── Widget handles ──────────────────────────────── */
static lv_obj_t *menu_rows[MENU_COUNT];
static lv_obj_t *menu_lbls[MENU_COUNT];

#define FN10 (&lv_font_montserrat_10)
#define FN12 (&lv_font_montserrat_12)

/* ─────────────────────────────────────────────────
 *  Redraw selection highlight
 * ───────────────────────────────────────────────── */
static void menu_redraw(void)
{
    for (int i = 0; i < (int)MENU_COUNT; i++) {
        bool sel = (i == menu_sel);
        lv_obj_set_style_bg_color(menu_rows[i],
            sel ? lv_color_hex(0x162840) : lv_color_hex(0x0E1820), 0);
        lv_obj_set_style_border_color(menu_rows[i],
            sel ? lv_color_hex(0x2A6AAA) : lv_color_hex(0x1A2A3A), 0);
        lv_obj_set_style_text_color(menu_lbls[i],
            sel ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x556A7A), 0);
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: key handler
 * ───────────────────────────────────────────────── */
void ui_menu_key(ui_key_t key)
{
    switch (key) {
    case KEY_UP:
        if (menu_sel > 0) { menu_sel--; menu_redraw(); }
        break;
    case KEY_DOWN:
        if (menu_sel < (int)MENU_COUNT - 1) { menu_sel++; menu_redraw(); }
        break;
    case KEY_OK:
        if (menu_sel == 0) ui_goto(PAGE_PARAM);
        break;
    case KEY_ESC:
        menu_sel = 0;
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

    ui_topbar_create(parent, "系统菜单", "已验证",
                     lv_color_hex(0x1A3A20), lv_color_hex(0x4ECB71));

    int content_h = SCR_H - 30 - 22;  /* 188 px */
    int row_h_px  = content_h / (int)MENU_COUNT;  /* evenly divide */

    for (int i = 0; i < (int)MENU_COUNT; i++) {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_pos(row, MENU_X, MENU_Y + i * row_h_px);
        lv_obj_set_size(row, SCR_W, row_h_px);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x0E1820), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(0x1A2A3A), 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        menu_rows[i] = row;

        /* Cursor marker */
        lv_obj_t *cursor = lv_label_create(row);
        lv_label_set_text(cursor, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(cursor, lv_color_hex(0x7ECFFF), 0);
        lv_obj_set_style_text_font(cursor, FN10, 0);
        lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 6, 0);

        /* Icon */
        lv_obj_t *icon = lv_label_create(row);
        lv_label_set_text(icon, MENU_ITEMS[i].icon);
        lv_obj_set_style_text_color(icon, lv_color_hex(0x7ECFFF), 0);
        lv_obj_set_style_text_font(icon, FN12, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 22, 0);

        /* Label */
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, MENU_ITEMS[i].label);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x556A7A), 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 40, 0);
        menu_lbls[i] = lbl;

        /* Chevron (right arrow hint) */
        lv_obj_t *chev = lv_label_create(row);
        lv_label_set_text(chev, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(chev, lv_color_hex(0x2A4A6A), 0);
        lv_obj_set_style_text_font(chev, FN10, 0);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -6, 0);
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
        if (key == KEY_OK) ui_goto(PAGE_PASSWORD);
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
    }
}
