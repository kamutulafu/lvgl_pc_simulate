/**
 * ui_calib.c
 * 浓度校准 (Concentration Calibration)
 */

#include "ui.h"
#include <string.h>
#include <stdio.h>

#define VISIBLE_ROWS 5
#define ROW_H 33
#define MENU_X 0
#define MENU_Y 30

typedef enum {
    CALIB_ST_GAS_LIST,
    CALIB_ST_ACTION_LIST
} calib_state_t;

static calib_state_t g_state = CALIB_ST_GAS_LIST;
static int g_gas_sel = 0;
static int g_gas_top = 0;
static int g_act_sel = 0;

static const char *ACTION_NAMES[] = {
    "调零",
    "一级校准",
    "二级校准"
};
#define ACTION_COUNT (sizeof(ACTION_NAMES) / sizeof(ACTION_NAMES[0]))

static lv_obj_t *list_rows[VISIBLE_ROWS];
static lv_obj_t *list_lbls[VISIBLE_ROWS];
static lv_obj_t *list_cursors[VISIBLE_ROWS];
static lv_obj_t *hint_slots[4];

static void calib_redraw(void)
{
    int count = (g_state == CALIB_ST_GAS_LIST) ? g_gas_count : ACTION_COUNT;
    int top = (g_state == CALIB_ST_GAS_LIST) ? g_gas_top : 0;
    int sel = (g_state == CALIB_ST_GAS_LIST) ? g_gas_sel : g_act_sel;

    for (int i = 0; i < VISIBLE_ROWS; i++) {
        int idx = top + i;
        if (idx < count) {
            lv_obj_remove_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
            if (g_state == CALIB_ST_GAS_LIST) {
                lv_label_set_text(list_lbls[i], ui_get_text(g_gas[idx].name));
            } else {
                lv_label_set_text(list_lbls[i], ui_get_text(ACTION_NAMES[idx]));
            }

            bool is_sel = (idx == sel);
            lv_obj_set_style_bg_color(list_rows[i], is_sel ? C_PW_ACTIVE : C_BG_CARD, 0);
            lv_obj_set_style_border_color(list_rows[i], is_sel ? C_PW_BORDER : C_BORDER, 0);
            lv_obj_set_style_text_color(list_lbls[i], is_sel ? C_TEXT_PRI : C_TEXT_SEC, 0);
            lv_label_set_text(list_cursors[i], is_sel ? LV_SYMBOL_RIGHT : " ");
        } else {
            lv_obj_add_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_calib_key(ui_key_t key)
{
    if (key == KEY_LEFT) key = KEY_ESC;
    else if (key == KEY_RIGHT) key = KEY_OK;

    int count = (g_state == CALIB_ST_GAS_LIST) ? g_gas_count : ACTION_COUNT;

    switch (key) {
    case KEY_UP:
        if (g_state == CALIB_ST_GAS_LIST) {
            if (g_gas_sel > 0) {
                g_gas_sel--;
                if (g_gas_sel < g_gas_top) g_gas_top = g_gas_sel;
            }
        } else {
            if (g_act_sel > 0) g_act_sel--;
        }
        calib_redraw();
        break;
    case KEY_DOWN:
        if (g_state == CALIB_ST_GAS_LIST) {
            if (g_gas_sel < count - 1) {
                g_gas_sel++;
                if (g_gas_sel >= g_gas_top + VISIBLE_ROWS) g_gas_top = g_gas_sel - VISIBLE_ROWS + 1;
            }
        } else {
            if (g_act_sel < count - 1) g_act_sel++;
        }
        calib_redraw();
        break;
    case KEY_OK:
        if (g_state == CALIB_ST_GAS_LIST) {
            if (g_gas_count > 0) {
                g_state = CALIB_ST_ACTION_LIST;
                g_act_sel = 0;
                calib_redraw();
            }
        } else {
            // Here you would enter the actual calibration logic.
        }
        break;
    case KEY_ESC:
        if (g_state == CALIB_ST_ACTION_LIST) {
            g_state = CALIB_ST_GAS_LIST;
            calib_redraw();
        } else {
            ui_goto(PAGE_MENU);
        }
        break;
    }
}

void ui_page_calib_create(lv_obj_t *parent)
{
    g_state = CALIB_ST_GAS_LIST;
    g_gas_sel = 0;
    g_gas_top = 0;
    g_act_sel = 0;

    ui_topbar_create(parent, ui_get_text("浓度校准"), "", C_BADGE_OK_BG, C_OK);

    int content_h = SCR_H - 30 - 22;
    int row_h_px = content_h / VISIBLE_ROWS;

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
        list_rows[i] = row;

        lv_obj_t *cursor = lv_label_create(row);
        lv_label_set_text(cursor, " ");
        lv_obj_set_style_text_color(cursor, C_ACCENT, 0);
        lv_obj_set_style_text_font(cursor, &lv_font_montserrat_14, 0);
        lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 10, 0);
        list_cursors[i] = cursor;

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, "");
        lv_obj_set_style_text_color(lbl, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 30, 0);
        list_lbls[i] = lbl;
    }

    calib_redraw();
    ui_hintbar_create(parent, "▲▼ 选择", "", "OK 进入", "ESC 返回");
}
