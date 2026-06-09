#include "ui.h"
#include <stdio.h>
#include <string.h>

void ui_page_curve_create(lv_obj_t *parent)
{
    if (g_selected_gas < 0 || g_selected_gas >= g_gas_count) {
        g_selected_gas = 0;
    }
    
    gas_ch_t *g = &g_gas[g_selected_gas];
    
    // Create Topbar
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), ui_get_text("%s Trend"), g->symbol);
    
    ui_topbar_create(parent, title_buf, NULL, lv_color_hex(0), lv_color_hex(0));
    
    // Create Chart container or card
    lv_obj_t *card = ui_card_create(parent, 4, 34, SCR_W - 8, 178, C_BG_CARD, C_BORDER);
    
    // Value color mapping
    lv_color_t vcol = (g->status == GAS_ALARM) ? C_ALARM : C_OK;

    // --- TOP SECTION OF CARD ---
    
    // Current value label
    lv_obj_t *lbl_cur_title = lv_label_create(card);
    lv_label_set_text(lbl_cur_title, ui_get_text("Current Value"));
    lv_obj_set_style_text_color(lbl_cur_title, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lbl_cur_title, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_cur_title, LV_ALIGN_TOP_LEFT, 10, 6);

    lv_obj_t *lbl_cur_val = lv_label_create(card);
    char cur_buf[48];
    snprintf(cur_buf, sizeof(cur_buf), "%.1f %s", g->value, g->unit);
    lv_label_set_text(lbl_cur_val, cur_buf);
    lv_obj_set_style_text_color(lbl_cur_val, vcol, 0);
    lv_obj_set_style_text_font(lbl_cur_val, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_cur_val, LV_ALIGN_TOP_LEFT, 10, 20);

    // Alarm limit label
    lv_obj_t *lbl_alarm_title = lv_label_create(card);
    lv_label_set_text(lbl_alarm_title, ui_get_text("Alarm Limit"));
    lv_obj_set_style_text_color(lbl_alarm_title, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lbl_alarm_title, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_alarm_title, LV_ALIGN_TOP_RIGHT, -10, 6);

    lv_obj_t *lbl_alarm_val = lv_label_create(card);
    char alarm_buf[48];
    snprintf(alarm_buf, sizeof(alarm_buf), "%.1f %s", g->alarm_hi, g->unit);
    lv_label_set_text(lbl_alarm_val, alarm_buf);
    lv_obj_set_style_text_color(lbl_alarm_val, C_ALARM, 0);
    lv_obj_set_style_text_font(lbl_alarm_val, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_alarm_val, LV_ALIGN_TOP_RIGHT, -10, 20);
    
    // --- MIDDLE SECTION (CHART) ---
    
    // Create Chart
    lv_obj_t *chart_obj = lv_chart_create(card);
    lv_obj_set_size(chart_obj, 256, 90);
    lv_obj_align(chart_obj, LV_ALIGN_TOP_MID, 16, 44);
    lv_chart_set_type(chart_obj, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_obj, 10);
    lv_chart_set_div_line_count(chart_obj, 5, 5);
    lv_obj_set_style_line_width(chart_obj, 2, LV_PART_ITEMS);
    
    // Style chart background to be transparent and borderless
    lv_obj_set_style_bg_opa(chart_obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart_obj, 0, 0);
    lv_obj_set_style_line_color(chart_obj, lv_color_hex(0x2E3B4E), LV_PART_MAIN);
    
    // Set ranges
    float y_max = g->range_max;
    if (y_max <= 0) y_max = 100.0f;
    lv_chart_set_range(chart_obj, LV_CHART_AXIS_PRIMARY_Y, 0, (int32_t)y_max);
    
    // Add series
    lv_chart_series_t *ser_val = lv_chart_add_series(chart_obj, vcol, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *ser_alarm = lv_chart_add_series(chart_obj, C_ALARM, LV_CHART_AXIS_PRIMARY_Y);
    
    // Populate mock data
    float base_val = g->value;
    float mock_factor[10] = { -0.5f, 0.4f, -0.2f, 0.8f, -0.1f, -0.6f, 0.5f, -0.3f, 0.7f, 0.0f };
    float step_val = g->range_max * 0.01f;
    if (step_val <= 0.01f) step_val = 0.1f;
    
    for (int j = 0; j < 10; j++) {
        float v = base_val + mock_factor[j] * step_val * 5.0f;
        if (v < 0) v = 0;
        if (v > y_max) v = y_max;
        lv_chart_set_next_value(chart_obj, ser_val, (int32_t)v);
        
        // Alarm threshold line is straight
        lv_chart_set_next_value(chart_obj, ser_alarm, (int32_t)g->alarm_hi);
    }
    
    // Y-Axis Labels
    lv_obj_t *lbl_ymax = lv_label_create(card);
    char max_buf[16];
    if (y_max < 99) {
        snprintf(max_buf, sizeof(max_buf), "%.1f", y_max);
    } else {
        snprintf(max_buf, sizeof(max_buf), "%.0f", y_max);
    }
    lv_label_set_text(lbl_ymax, max_buf);
    lv_obj_set_style_text_color(lbl_ymax, C_TEXT_HINT, 0);
    lv_obj_set_style_text_font(lbl_ymax, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_ymax, LV_ALIGN_TOP_LEFT, 2, 44);
    
    lv_obj_t *lbl_ymin = lv_label_create(card);
    lv_label_set_text(lbl_ymin, "0");
    lv_obj_set_style_text_color(lbl_ymin, C_TEXT_HINT, 0);
    lv_obj_set_style_text_font(lbl_ymin, &lv_font_montserrat_10, 0);
    lv_obj_align(lbl_ymin, LV_ALIGN_TOP_LEFT, 2, 124);
    
    // --- BOTTOM SECTION ---
    
    // Collection info below chart (bottom left)
    lv_obj_t *lbl_info = lv_label_create(card);
    lv_label_set_text(lbl_info, ui_get_text("Data Points: 1 - 10"));
    lv_obj_set_style_text_color(lbl_info, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lbl_info, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_info, LV_ALIGN_BOTTOM_LEFT, 10, -8);
    
    // Channel info below chart (bottom right)
    lv_obj_t *lbl_chan = lv_label_create(card);
    char chan_buf[64];
    snprintf(chan_buf, sizeof(chan_buf), ui_get_text("Channel: %d/%d"), g_selected_gas + 1, g_gas_count);
    lv_label_set_text(lbl_chan, chan_buf);
    lv_obj_set_style_text_color(lbl_chan, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(lbl_chan, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_chan, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
    
    // Bottom hint bar
    ui_hintbar_create(parent, "", "", "", "ESC Back");
}

void ui_curve_key(ui_key_t key)
{
    if (key == KEY_ESC) {
        ui_goto(PAGE_HOME);
    }
}
