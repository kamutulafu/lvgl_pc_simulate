/**
 * ui_sys_info.c
 * 设备信息 (System Information)
 */

#include "ui.h"
#include <string.h>

void ui_sys_info_key(ui_key_t key)
{
    // Any key exits the screen (or just ESC)
    if (key == KEY_ESC || key == KEY_OK || key == KEY_LEFT) {
        ui_goto(PAGE_MENU);
    }
}

void ui_page_sys_info_create(lv_obj_t *parent)
{
    ui_topbar_create(parent, ui_get_text("系统信息"), "", C_BADGE_OK_BG, C_OK);

    // Main content area
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, SCR_W, SCR_H - 30 - 22);
    lv_obj_set_pos(cont, 0, 30);
    lv_obj_set_style_bg_color(cont, C_BG_SCREEN, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    const char *labels[] = {
        "软件信息",
        "硬件信息",
        "设备编号"
    };
    
    const char *values[] = {
        "V1.0.0_Build_2026",
        "HW_REV_1.2",
        "DEV-00042"
    };

    for(int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_obj_create(cont);
        lv_obj_set_size(row, SCR_W - 20, 40);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 10 + i * 45);
        lv_obj_set_style_bg_color(row, C_BG_CARD, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // Label
        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, ui_get_text(labels[i]));
        lv_obj_set_style_text_color(lbl, C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 10, 0);

        // Value
        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, values[i]);
        lv_obj_set_style_text_color(val, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    ui_hintbar_create(parent, "", "", "", "ESC 返回");
}
