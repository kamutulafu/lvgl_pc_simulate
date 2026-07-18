/**
 * ui_alarm_set.c
 * 报警设置 (Alarm Settings)
 */

#include "ui.h"
#include <string.h>
#include <stdio.h>

#define VISIBLE_ROWS 5
#define ROW_H 33
#define MENU_X 0
#define MENU_Y 30

typedef enum {
    AL_ST_GAS_LIST,
    AL_ST_PARAM_LIST,
    AL_ST_EDIT_ENUM,
    AL_ST_NUM_EDIT,
    AL_ST_NUM_CONFIRM
} alarm_state_t;

static alarm_state_t g_state = AL_ST_GAS_LIST;
static int g_gas_sel = 0;
static int g_gas_top = 0;
static int g_param_sel = 0;

/* Temporary edit buffers */
static int g_edit_enum_val = 0;
static float g_edit_val = 0;
static float g_orig_val = 0;

/* Gas specific configs, mocked for now as they might not exist in gas_ch_t */
static int gas_alarm_mode[5] = {0};
static int gas_alarm_type[5] = {7, 7, 7, 7, 7}; // Default sound+light+vib

static const char *PARAM_NAMES[] = {
    "报警模式",
    "报警方式",
    "低报",
    "高报"
};
#define PARAM_COUNT (sizeof(PARAM_NAMES) / sizeof(PARAM_NAMES[0]))

static const char *MODE_NAMES[] = {
    "模式0: 正<低<高",
    "模式1: 正>高>低",
    "模式2: 低>正<高"
};

static const char *TYPE_NAMES[] = {
    "无", "声", "光", "声+光",
    "振", "声+振", "光+振", "声+光+振"
};

static lv_obj_t *list_rows[VISIBLE_ROWS];
static lv_obj_t *list_lbls[VISIBLE_ROWS];
static lv_obj_t *list_vals[VISIBLE_ROWS];
static lv_obj_t *list_cursors[VISIBLE_ROWS];
static lv_obj_t *hint_slots[4];

/* Num Overlay Objects */
static lv_obj_t *num_overlay = NULL;
static lv_obj_t *num_lbl_name = NULL;
static lv_obj_t *num_lbl_val = NULL;
static lv_obj_t *num_bar = NULL;
static lv_obj_t *num_lbl_range = NULL;
static lv_obj_t *num_lbl_orig = NULL;

static void update_num_overlay(void)
{
    if (g_state != AL_ST_NUM_EDIT && g_state != AL_ST_NUM_CONFIRM) {
        lv_obj_add_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_remove_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(num_lbl_name, ui_get_text(PARAM_NAMES[g_param_sel]));

    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", g_edit_val);
    lv_label_set_text(num_lbl_val, buf);

    float max_val = 100.0f; // Mock max value for bar
    int range = (int)(max_val * 10);
    lv_bar_set_range(num_bar, 0, range);
    lv_bar_set_value(num_bar, (int)(g_edit_val * 10), LV_ANIM_ON);

    snprintf(buf, sizeof(buf), "Min: 0.0  Max: %.1f", max_val);
    lv_label_set_text(num_lbl_range, buf);
    
    snprintf(buf, sizeof(buf), "原值: %.1f", g_orig_val);
    lv_label_set_text(num_lbl_orig, buf);
}

static void update_hintbar(void)
{
    const char *h0 = "";
    const char *h1 = "";
    const char *h2 = "";
    const char *h3 = "";

    switch (g_state) {
        case AL_ST_GAS_LIST:
        case AL_ST_PARAM_LIST:
            h0 = "▲▼ 选择"; h2 = "OK 进入"; h3 = "ESC 返回"; break;
        case AL_ST_EDIT_ENUM:
            h0 = "▲▼ 更改"; h2 = "OK 确认"; h3 = "ESC 返回"; break;
        case AL_ST_NUM_EDIT:
            h0 = "▲ +"; h1 = "▼ -"; h2 = "OK 确认"; h3 = "ESC 放弃"; break;
        case AL_ST_NUM_CONFIRM:
            h0 = ""; h1 = ""; h2 = "OK 保存"; h3 = "ESC 继续改"; break;
    }
    
    lv_label_set_text(hint_slots[0], ui_get_text(h0));
    lv_label_set_text(hint_slots[1], ui_get_text(h1));
    lv_label_set_text(hint_slots[2], ui_get_text(h2));
    lv_label_set_text(hint_slots[3], ui_get_text(h3));
}

static void alarm_redraw(void)
{
    int count = (g_state == AL_ST_GAS_LIST) ? g_gas_count : PARAM_COUNT;
    int top = (g_state == AL_ST_GAS_LIST) ? g_gas_top : 0;
    int sel = (g_state == AL_ST_GAS_LIST) ? g_gas_sel : g_param_sel;

    for (int i = 0; i < VISIBLE_ROWS; i++) {
        int idx = top + i;
        if (idx < count) {
            lv_obj_remove_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
            
            if (g_state == AL_ST_GAS_LIST) {
                lv_label_set_text(list_lbls[i], ui_get_text(g_gas[idx].name));
                lv_label_set_text(list_vals[i], "");
            } else {
                lv_label_set_text(list_lbls[i], ui_get_text(PARAM_NAMES[idx]));
                char vbuf[32] = {0};
                
                if (idx == 0) { // 报警模式
                    int mode = (g_state == AL_ST_EDIT_ENUM && sel == 0) ? g_edit_enum_val : gas_alarm_mode[g_gas_sel];
                    snprintf(vbuf, sizeof(vbuf), "%s", MODE_NAMES[mode]);
                } else if (idx == 1) { // 报警方式
                    int type = (g_state == AL_ST_EDIT_ENUM && sel == 1) ? g_edit_enum_val : gas_alarm_type[g_gas_sel];
                    snprintf(vbuf, sizeof(vbuf), "%s", TYPE_NAMES[type]);
                } else { // 高低报
                    float val = (idx == 2) ? g_gas[g_gas_sel].alarm_lo : g_gas[g_gas_sel].alarm_hi;
                    snprintf(vbuf, sizeof(vbuf), "%.1f", val);
                }
                
                lv_label_set_text(list_vals[i], vbuf);
            }

            bool is_sel = (idx == sel);
            lv_color_t bg = C_BG_CARD;
            lv_color_t border = C_BORDER;
            
            if (is_sel) {
                if (g_state == AL_ST_GAS_LIST || g_state == AL_ST_PARAM_LIST) {
                    bg = C_PW_ACTIVE; border = C_PW_BORDER;
                } else if (g_state == AL_ST_EDIT_ENUM) {
                    bg = lv_color_hex(0x2A1F0F); border = C_WARN;
                }
            }
            
            lv_obj_set_style_bg_color(list_rows[i], bg, 0);
            lv_obj_set_style_border_color(list_rows[i], border, 0);
            lv_obj_set_style_text_color(list_lbls[i], is_sel ? C_TEXT_PRI : C_TEXT_SEC, 0);
            lv_obj_set_style_text_color(list_vals[i], (is_sel && g_state == AL_ST_EDIT_ENUM) ? C_WARN : lv_color_hex(0x7ECFFF), 0);
            lv_label_set_text(list_cursors[i], (is_sel && g_state <= AL_ST_PARAM_LIST) ? LV_SYMBOL_RIGHT : " ");
        } else {
            lv_obj_add_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    update_num_overlay();
    update_hintbar();
}

void ui_alarm_set_key(ui_key_t key)
{
    if (key == KEY_LEFT) key = KEY_ESC;
    else if (key == KEY_RIGHT) key = KEY_OK;

    switch (g_state) {
    case AL_ST_GAS_LIST:
        if (key == KEY_UP && g_gas_sel > 0) {
            g_gas_sel--;
            if (g_gas_sel < g_gas_top) g_gas_top = g_gas_sel;
        } else if (key == KEY_DOWN && g_gas_sel < g_gas_count - 1) {
            g_gas_sel++;
            if (g_gas_sel >= g_gas_top + VISIBLE_ROWS) g_gas_top = g_gas_sel - VISIBLE_ROWS + 1;
        } else if (key == KEY_OK && g_gas_count > 0) {
            g_state = AL_ST_PARAM_LIST;
            g_param_sel = 0;
        } else if (key == KEY_ESC) {
            ui_goto(PAGE_MENU);
            return;
        }
        break;

    case AL_ST_PARAM_LIST:
        if (key == KEY_UP && g_param_sel > 0) g_param_sel--;
        else if (key == KEY_DOWN && g_param_sel < PARAM_COUNT - 1) g_param_sel++;
        else if (key == KEY_OK) {
            if (g_param_sel == 0) {
                g_edit_enum_val = gas_alarm_mode[g_gas_sel];
                g_state = AL_ST_EDIT_ENUM;
            } else if (g_param_sel == 1) {
                g_edit_enum_val = gas_alarm_type[g_gas_sel];
                g_state = AL_ST_EDIT_ENUM;
            } else {
                g_orig_val = (g_param_sel == 2) ? g_gas[g_gas_sel].alarm_lo : g_gas[g_gas_sel].alarm_hi;
                g_edit_val = g_orig_val;
                g_state = AL_ST_NUM_EDIT;
            }
        } else if (key == KEY_ESC) {
            g_state = AL_ST_GAS_LIST;
        }
        break;

    case AL_ST_EDIT_ENUM:
        if (key == KEY_UP) {
            int max_val = (g_param_sel == 0) ? 2 : 7;
            g_edit_enum_val = (g_edit_enum_val + 1) % (max_val + 1);
        } else if (key == KEY_DOWN) {
            int max_val = (g_param_sel == 0) ? 2 : 7;
            g_edit_enum_val = (g_edit_enum_val + max_val) % (max_val + 1);
        } else if (key == KEY_OK) {
            if (g_param_sel == 0) gas_alarm_mode[g_gas_sel] = g_edit_enum_val;
            else gas_alarm_type[g_gas_sel] = g_edit_enum_val;
            g_state = AL_ST_PARAM_LIST;
        } else if (key == KEY_ESC) {
            g_state = AL_ST_PARAM_LIST;
        }
        break;

    case AL_ST_NUM_EDIT:
        if (key == KEY_UP) {
            g_edit_val += 0.5f;
            if (g_edit_val > 100.0f) g_edit_val = 100.0f;
        } else if (key == KEY_DOWN) {
            g_edit_val -= 0.5f;
            if (g_edit_val < 0.0f) g_edit_val = 0.0f;
        } else if (key == KEY_OK) {
            g_state = AL_ST_NUM_CONFIRM;
        } else if (key == KEY_ESC) {
            g_state = AL_ST_PARAM_LIST;
        }
        break;

    case AL_ST_NUM_CONFIRM:
        if (key == KEY_OK) {
            if (g_param_sel == 2) g_gas[g_gas_sel].alarm_lo = g_edit_val;
            else g_gas[g_gas_sel].alarm_hi = g_edit_val;
            g_state = AL_ST_PARAM_LIST;
        } else if (key == KEY_ESC) {
            g_state = AL_ST_NUM_EDIT;
        }
        break;
    }
    alarm_redraw();
}

static void build_num_overlay_alarm(lv_obj_t *parent)
{
    num_overlay = lv_obj_create(parent);
    lv_obj_set_size(num_overlay, SCR_W, SCR_H - 30 - 22);
    lv_obj_set_pos(num_overlay, 0, 30);
    lv_obj_set_style_bg_color(num_overlay, lv_color_hex(0x0D1A28), 0);
    lv_obj_set_style_bg_opa(num_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(num_overlay, 0, 0);
    lv_obj_set_style_pad_all(num_overlay, 0, 0);
    lv_obj_clear_flag(num_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);

    num_lbl_name = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_name, "");
    lv_obj_set_style_text_color(num_lbl_name, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(num_lbl_name, UI_FONT_CJK_24, 0);
    lv_obj_align(num_lbl_name, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *ua = lv_label_create(num_overlay);
    lv_label_set_text(ua, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(ua, lv_color_hex(0x2A5A8A), 0);
    lv_obj_set_style_text_font(ua, &lv_font_montserrat_14, 0);
    lv_obj_align(ua, LV_ALIGN_TOP_MID, 0, 30);

    num_lbl_val = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_val, "---");
    lv_obj_set_style_text_color(num_lbl_val, C_WARN, 0);
    lv_obj_set_style_text_font(num_lbl_val, UI_FONT_CJK_24, 0);
    lv_obj_align(num_lbl_val, LV_ALIGN_TOP_MID, 0, 48);

    lv_obj_t *da = lv_label_create(num_overlay);
    lv_label_set_text(da, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(da, lv_color_hex(0x2A5A8A), 0);
    lv_obj_set_style_text_font(da, &lv_font_montserrat_14, 0);
    lv_obj_align(da, LV_ALIGN_TOP_MID, 0, 92);

    num_bar = lv_bar_create(num_overlay);
    lv_obj_set_size(num_bar, SCR_W - 40, 8);
    lv_obj_align(num_bar, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_set_style_bg_color(num_bar, C_BAR_BG, 0);
    lv_obj_set_style_bg_color(num_bar, C_WARN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(num_bar, 4, 0);
    lv_obj_set_style_radius(num_bar, 4, LV_PART_INDICATOR);

    num_lbl_range = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_range, "");
    lv_obj_set_style_text_color(num_lbl_range, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(num_lbl_range, UI_FONT_CJK_14, 0);
    lv_obj_align(num_lbl_range, LV_ALIGN_TOP_MID, 0, 128);

    num_lbl_orig = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_orig, ui_get_text("原值:"));
    lv_obj_set_style_text_color(num_lbl_orig, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(num_lbl_orig, UI_FONT_CJK_14, 0);
    lv_obj_align(num_lbl_orig, LV_ALIGN_TOP_LEFT, 20, 148);
}

void ui_page_alarm_set_create(lv_obj_t *parent)
{
    g_state = AL_ST_GAS_LIST;
    g_gas_sel = 0;
    g_gas_top = 0;
    g_param_sel = 0;

    ui_topbar_create(parent, ui_get_text("报警设置"), "", C_BADGE_OK_BG, C_OK);

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
        
        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, "");
        lv_obj_set_style_text_color(val, lv_color_hex(0x7ECFFF), 0);
        lv_obj_set_style_text_font(val, UI_FONT_CJK_24, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -10, 0);
        list_vals[i] = val;
    }

    build_num_overlay_alarm(parent);

    lv_obj_t *hb = lv_obj_create(parent);
    lv_obj_set_size(hb, SCR_W, 22);
    lv_obj_align(hb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(hb, lv_color_hex(0x0C1520), 0);
    lv_obj_set_style_bg_opa(hb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hb, 1, 0);
    lv_obj_set_style_border_side(hb, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(hb, lv_color_hex(0x1E3040), 0);
    lv_obj_set_style_pad_all(hb, 0, 0);
    lv_obj_clear_flag(hb, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        hint_slots[i] = lv_label_create(hb);
        lv_label_set_text(hint_slots[i], "");
        lv_obj_set_style_text_color(hint_slots[i], lv_color_hex(0xDFE9F0), 0);
        lv_obj_set_style_text_font(hint_slots[i], UI_FONT_CJK_14, 0);
        
        if (i == 3) {
            lv_obj_set_width(hint_slots[i], 90);
            lv_obj_align(hint_slots[i], LV_ALIGN_LEFT_MID, 4, 0);
        } else if (i == 2) {
            lv_obj_set_width(hint_slots[i], 90);
            lv_obj_align(hint_slots[i], LV_ALIGN_RIGHT_MID, -4, 0);
        } else if (i == 0) {
            lv_obj_set_width(hint_slots[i], 80);
            lv_obj_align(hint_slots[i], LV_ALIGN_CENTER, -40, 0);
        } else if (i == 1) {
            lv_obj_set_width(hint_slots[i], 80);
            lv_obj_align(hint_slots[i], LV_ALIGN_CENTER, 40, 0);
        }
    }

    alarm_redraw();
}
