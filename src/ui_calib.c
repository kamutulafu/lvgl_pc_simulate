/**
 * ui_calib.c
 * 浓度校准 (Concentration Calibration)
 * Zero, Level 1, and Level 2 Calibration screens ported from HTML.
 */

#include "ui.h"
#include <string.h>
#include <stdio.h>

#define VISIBLE_ROWS 5
#define ROW_H 33
#define MENU_X 0
#define MENU_Y 30

#define FN10  (&lv_font_montserrat_10)
#define FN12  (&lv_font_montserrat_12)
#define FN14  (&lv_font_montserrat_14)
#define FN20  (&lv_font_montserrat_20)
#define FN24  (&lv_font_montserrat_24)
#define FN32  (&lv_font_montserrat_32)

typedef enum {
    CALIB_ST_GAS_LIST,
    CALIB_ST_ACTION_LIST,
    CALIB_ST_EXECUTION
} calib_state_t;

typedef enum {
    EXEC_STATUS_IDLE,
    EXEC_STATUS_CALIBRATING,
    EXEC_STATUS_SUCCESS,
    EXEC_STATUS_FAIL
} exec_status_t;

typedef enum {
    EXEC_FOCUS_READING,
    EXEC_FOCUS_TARGET
} exec_focus_t;

typedef enum {
    DIG_STATE_BROWSE,
    DIG_STATE_SELECT,
    DIG_STATE_EDIT,
    DIG_STATE_CONFIRM
} dig_state_t;

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

/* Execution UI State & Timers */
static exec_status_t g_exec_status = EXEC_STATUS_IDLE;
static exec_focus_t g_exec_focus = EXEC_FOCUS_READING;
static dig_state_t g_exec_dig_state = DIG_STATE_SELECT;

static int g_active_bit = 0;
static int g_digits[4] = {0};
static int g_orig_digits[4] = {0};
static int g_num_digits = 3;
static int g_decimal_pos = 2; // Index after dot. (e.g. 2 means format XX.X)

static lv_timer_t *g_calib_timer = NULL;
static lv_timer_t *g_hint_err_timer = NULL;
static lv_timer_t *g_live_timer = NULL;
static bool g_show_hint_err = false;
static char g_hint_err_text[64] = "";
static bool g_live_hint_ok = false;
static bool g_live_hint_valid = false;

/* Execution UI Widgets */
static lv_obj_t *g_exec_cnt = NULL;
static lv_obj_t *g_reading_card = NULL;
static lv_obj_t *g_val_cnt = NULL;
static lv_obj_t *g_badge_lbl = NULL;
static lv_obj_t *g_gas_lbl = NULL;
static lv_obj_t *g_value_lbl = NULL;
static lv_obj_t *g_unit_lbl = NULL;
static lv_obj_t *g_range_lbl = NULL;
static lv_obj_t *g_zero_tip_lbl = NULL;

static lv_obj_t *g_target_card = NULL;
static lv_obj_t *g_target_title_lbl = NULL;
static lv_obj_t *g_static_target_lbl = NULL;

static lv_obj_t *g_cell_objs[4] = {NULL};
static lv_obj_t *g_cell_lbls[4] = {NULL};
static lv_obj_t *g_arrow_up[4] = {NULL};
static lv_obj_t *g_arrow_dn[4] = {NULL};
static lv_obj_t *g_dot_lbl = NULL;
static lv_obj_t *g_dig_hint_lbl = NULL;

static lv_obj_t *g_overlay_cnt = NULL;
static lv_obj_t *g_overlay_icon = NULL;
static lv_obj_t *g_overlay_text = NULL;
static lv_obj_t *g_overlay_subtext = NULL;
static lv_obj_t *g_overlay_ok_btn = NULL;
static lv_obj_t *g_overlay_ok_btn_lbl = NULL;

static lv_obj_t *g_parent_screen = NULL;
static lv_obj_t *g_hintbar = NULL;

/* Declarations */
static void calib_redraw(void);
static void redraw_exec_ui(void);
static void apply_exec_layout(void);
static void stop_live_timer(void);
static void start_live_timer(void);
static void get_calib_ranges(float *cal_lo, float *cal_hi, float *ok_lo, float *ok_hi);

static float decode_target_value(void)
{
    float val = 0.0f;
    if (g_act_sel == 1) {
        val = g_digits[1] * 10.0f + g_digits[2] + g_digits[3] / 10.0f;
    } else if (g_act_sel == 2) {
        val = g_digits[0] * 100.0f + g_digits[1] * 10.0f + g_digits[2] + g_digits[3] / 10.0f;
    }
    return val;
}

static void init_exec_digits(void)
{
    float range = g_gas[g_gas_sel].range_max;
    float target = 0.0f;
    memset(g_digits, 0, sizeof(g_digits));

    if (g_act_sel == 1) { // 一级校准 (3 digits: D1 D2 . D3)
        target = range * 0.3f;
        g_num_digits = 3;
        g_decimal_pos = 2; // Dot is before the last digit (i.e. between cell 2 and cell 3)
        int target_x10 = (int)(target * 10.0f + 0.5f);
        g_digits[3] = target_x10 % 10;
        g_digits[2] = (target_x10 / 10) % 10;
        g_digits[1] = (target_x10 / 100) % 10;
        g_digits[0] = 0; // Unused
        g_active_bit = 1;
    } else if (g_act_sel == 2) { // 二级校准 (4 digits: D0 D1 D2 . D3)
        target = range * 0.8f;
        g_num_digits = 4;
        g_decimal_pos = 3; // Dot is before cell 3
        int target_x10 = (int)(target * 10.0f + 0.5f);
        g_digits[3] = target_x10 % 10;
        g_digits[2] = (target_x10 / 10) % 10;
        g_digits[1] = (target_x10 / 100) % 10;
        g_digits[0] = (target_x10 / 1000) % 10;
        g_active_bit = 0;
    }
    g_exec_status = EXEC_STATUS_IDLE;
    g_exec_focus = (g_act_sel == 0) ? EXEC_FOCUS_READING : EXEC_FOCUS_TARGET;
    g_exec_dig_state = DIG_STATE_SELECT;
    memcpy(g_orig_digits, g_digits, sizeof(g_digits));
    g_show_hint_err = false;
    start_live_timer();
}

static void stop_live_timer(void)
{
    if (g_live_timer) {
        lv_timer_del(g_live_timer);
        g_live_timer = NULL;
    }
}

static void live_refresh_reading(void)
{
    if (g_state != CALIB_ST_EXECUTION) return;
    if (g_exec_status != EXEC_STATUS_IDLE) return;
    if (!g_value_lbl || !g_range_lbl || !g_badge_lbl) return;

    gas_ch_t *gas = &g_gas[g_gas_sel];
    float v = gas->value;
    float cal_lo, cal_hi, ok_lo, ok_hi;
    get_calib_ranges(&cal_lo, &cal_hi, &ok_lo, &ok_hi);
    bool ok = (v >= cal_lo && v <= cal_hi);

    if (g_act_sel == 0) {
        lv_label_set_text_fmt(g_value_lbl, "%.1f", v);
        lv_label_set_text(g_unit_lbl, gas->unit);

        lv_label_set_text_fmt(g_range_lbl, "%s: %.1f ~ %.1f %s",
                              ui_get_text("校准范围"), cal_lo, cal_hi, gas->unit);

        if (ok) {
            lv_label_set_text(g_badge_lbl, ui_get_text("可校准"));
            lv_obj_set_style_bg_color(g_badge_lbl, lv_color_hex(0x004400), 0);
            lv_obj_set_style_text_color(g_badge_lbl, lv_color_hex(0x4ECB71), 0);
            lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0x00FF00), 0);
        } else {
            lv_label_set_text(g_badge_lbl, ui_get_text("超出范围"));
            lv_obj_set_style_bg_color(g_badge_lbl, lv_color_hex(0x440000), 0);
            lv_obj_set_style_text_color(g_badge_lbl, lv_color_hex(0xE05A5A), 0);
            lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0xFF0000), 0);
        }

        if (g_zero_tip_lbl) {
            lv_label_set_text(g_zero_tip_lbl,
                              ok ? ui_get_text("按 OK 开始校准")
                                 : ui_get_text("请通入零气"));
            lv_obj_set_style_text_color(g_zero_tip_lbl,
                                        ok ? lv_color_hex(0x7ECFFF) : lv_color_hex(0xE05A5A), 0);
        }

        /* Update OK slot text only when availability flips (avoid recreating whole bar every tick) */
        if (!g_show_hint_err && (!g_live_hint_valid || g_live_hint_ok != ok)) {
            g_live_hint_ok = ok;
            g_live_hint_valid = true;
            char h_ok[64];
            strcpy(h_ok, ok ? "OK 开始校准" : "OK 不可用");
            if (g_hintbar) {
                lv_obj_del(g_hintbar);
            }
            g_hintbar = ui_hintbar_create(g_parent_screen, "", "", h_ok, "ESC 返回");
        }
    } else {
        lv_label_set_text_fmt(g_value_lbl, "%s：%.1f %s", ui_get_text("实时值"), v, gas->unit);
        lv_obj_align(g_value_lbl, LV_ALIGN_CENTER, 0, 0);

        if (ok) {
            lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0x00FF00), 0);
        } else {
            lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0xFF0000), 0);
        }
    }
}

static void live_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    live_refresh_reading();
}

static void start_live_timer(void)
{
    stop_live_timer();
    g_live_hint_valid = false;
    g_live_timer = lv_timer_create(live_timer_cb, 300, NULL);
}

/* Zero: single read-only card. Level 1/2: dual cards with target editing. */
static void apply_exec_layout(void)
{
    if (!g_reading_card || !g_target_card) return;

    if (g_act_sel == 0) {
        lv_obj_remove_flag(g_badge_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(g_reading_card, 8, 16);
        lv_obj_set_size(g_reading_card, 304, 156);
        lv_obj_set_style_border_color(g_reading_card, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_bg_color(g_reading_card, lv_color_hex(0x0A1420), 0);

        if (g_val_cnt) {
            lv_obj_set_size(g_val_cnt, 240, 48);
            lv_obj_align(g_val_cnt, LV_ALIGN_TOP_MID, 0, 40);
        }
        if (g_value_lbl) {
            lv_obj_set_style_text_font(g_value_lbl, &lv_font_montserrat_32, 0);
            lv_obj_align(g_value_lbl, LV_ALIGN_CENTER, -12, 0);
        }
        if (g_unit_lbl) {
            lv_obj_remove_flag(g_unit_lbl, LV_OBJ_FLAG_HIDDEN);
            if (g_value_lbl) {
                lv_obj_align_to(g_unit_lbl, g_value_lbl, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -6);
            }
        }
        if (g_range_lbl) {
            lv_obj_remove_flag(g_range_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_range_lbl, LV_ALIGN_TOP_MID, 0, 100);
        }
        if (g_zero_tip_lbl) {
            lv_obj_remove_flag(g_zero_tip_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_zero_tip_lbl, LV_ALIGN_BOTTOM_MID, 0, -10);
        }

        lv_obj_add_flag(g_target_card, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_badge_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_range_lbl, LV_OBJ_FLAG_HIDDEN);
        if (g_unit_lbl) {
            lv_obj_add_flag(g_unit_lbl, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_pos(g_reading_card, 8, 6);
        lv_obj_set_size(g_reading_card, 304, 82);

        if (g_val_cnt) {
            lv_obj_set_size(g_val_cnt, 200, 34);
            lv_obj_align(g_val_cnt, LV_ALIGN_TOP_MID, 0, 20);
        }
        if (g_value_lbl) {
            lv_obj_set_style_text_font(g_value_lbl, UI_FONT_CJK_24, 0);
            lv_obj_align(g_value_lbl, LV_ALIGN_CENTER, 0, 0);
        }
        if (g_zero_tip_lbl) {
            lv_obj_add_flag(g_zero_tip_lbl, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_remove_flag(g_target_card, LV_OBJ_FLAG_HIDDEN);
    }
}

static void get_calib_ranges(float *cal_lo, float *cal_hi, float *ok_lo, float *ok_hi)
{
    float range = g_gas[g_gas_sel].range_max;
    if (g_act_sel == 0) { // 调零
        *cal_lo = 0.0f;
        *cal_hi = range * 0.10f;
        *ok_lo = 0.0f;
        *ok_hi = range * 0.0333f;
    } else if (g_act_sel == 1) { // 一级校准
        float target = decode_target_value();
        *cal_lo = target * 0.70f;
        *cal_hi = target * 1.30f;
        *ok_lo = target * 0.90f;
        *ok_hi = target * 1.10f;
    } else if (g_act_sel == 2) { // 二级校准
        float target = decode_target_value();
        *cal_lo = target * 0.80f;
        *cal_hi = target * 1.20f;
        *ok_lo = target * 0.90f;
        *ok_hi = target * 1.10f;
    }
}

static void hint_err_timer_cb(lv_timer_t *timer)
{
    g_show_hint_err = false;
    g_hint_err_timer = NULL;
    lv_timer_delete(timer);
    redraw_exec_ui();
}

static void calib_timer_cb(lv_timer_t *timer)
{
    gas_ch_t *gas = &g_gas[g_gas_sel];
    float v = gas->value;
    float cal_lo, cal_hi, ok_lo, ok_hi;
    get_calib_ranges(&cal_lo, &cal_hi, &ok_lo, &ok_hi);

    if (v >= ok_lo && v <= ok_hi) {
        g_exec_status = EXEC_STATUS_SUCCESS;
        if (g_act_sel == 0) {
            gas->value = 0.0f;
        } else {
            gas->value = decode_target_value();
        }
    } else {
        g_exec_status = EXEC_STATUS_FAIL;
    }

    g_calib_timer = NULL;
    lv_timer_delete(timer);
    redraw_exec_ui();
}

static void overlay_ok_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        g_exec_status = EXEC_STATUS_IDLE;
        start_live_timer();
        redraw_exec_ui();
    }
}

static void create_execution_ui(lv_obj_t *parent)
{
    g_exec_cnt = lv_obj_create(parent);
    lv_obj_set_pos(g_exec_cnt, 0, 30);
    lv_obj_set_size(g_exec_cnt, SCR_W, 188);
    lv_obj_set_style_bg_opa(g_exec_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_exec_cnt, 0, 0);
    lv_obj_set_style_pad_all(g_exec_cnt, 0, 0);
    lv_obj_clear_flag(g_exec_cnt, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Reading Card */
    g_reading_card = lv_obj_create(g_exec_cnt);
    lv_obj_set_pos(g_reading_card, 8, 6);
    lv_obj_set_size(g_reading_card, 304, 82);
    lv_obj_set_style_radius(g_reading_card, 6, 0);
    lv_obj_set_style_bg_color(g_reading_card, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_bg_opa(g_reading_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_reading_card, 1, 0);
    lv_obj_set_style_border_color(g_reading_card, C_BORDER, 0);
    lv_obj_set_style_pad_all(g_reading_card, 0, 0);
    lv_obj_clear_flag(g_reading_card, LV_OBJ_FLAG_SCROLLABLE);

    g_gas_lbl = lv_label_create(g_reading_card);
    lv_obj_align(g_gas_lbl, LV_ALIGN_TOP_LEFT, 8, 4);
    lv_obj_set_style_text_font(g_gas_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_gas_lbl, C_TEXT_SEC, 0);

    g_badge_lbl = lv_label_create(g_reading_card);
    lv_obj_align(g_badge_lbl, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_set_style_text_font(g_badge_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_bg_opa(g_badge_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(g_badge_lbl, 4, 0);
    lv_obj_set_style_pad_hor(g_badge_lbl, 6, 0);
    lv_obj_set_style_pad_ver(g_badge_lbl, 1, 0);

    g_val_cnt = lv_obj_create(g_reading_card);
    lv_obj_set_size(g_val_cnt, 200, 34);
    lv_obj_align(g_val_cnt, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_opa(g_val_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_val_cnt, 0, 0);
    lv_obj_set_style_pad_all(g_val_cnt, 0, 0);
    lv_obj_clear_flag(g_val_cnt, LV_OBJ_FLAG_SCROLLABLE);

    g_value_lbl = lv_label_create(g_val_cnt);
    lv_obj_align(g_value_lbl, LV_ALIGN_CENTER, -15, 0);
    lv_obj_set_style_text_font(g_value_lbl, &lv_font_montserrat_32, 0);

    g_unit_lbl = lv_label_create(g_val_cnt);
    lv_obj_align_to(g_unit_lbl, g_value_lbl, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -4);
    lv_obj_set_style_text_font(g_unit_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(g_unit_lbl, lv_color_hex(0x7ECFFF), 0);

    g_range_lbl = lv_label_create(g_reading_card);
    lv_obj_align(g_range_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_font(g_range_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_range_lbl, lv_color_hex(0x88AACC), 0);

    /* Zero-mode only tip (hidden by default) */
    g_zero_tip_lbl = lv_label_create(g_reading_card);
    lv_label_set_text(g_zero_tip_lbl, "");
    lv_obj_align(g_zero_tip_lbl, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(g_zero_tip_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_zero_tip_lbl, lv_color_hex(0x7ECFFF), 0);
    lv_obj_add_flag(g_zero_tip_lbl, LV_OBJ_FLAG_HIDDEN);

    /* 2. Target Card */
    g_target_card = lv_obj_create(g_exec_cnt);
    lv_obj_set_pos(g_target_card, 8, 94);
    lv_obj_set_size(g_target_card, 304, 88);
    lv_obj_set_style_radius(g_target_card, 6, 0);
    lv_obj_set_style_bg_color(g_target_card, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_bg_opa(g_target_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_target_card, 1, 0);
    lv_obj_set_style_border_color(g_target_card, C_BORDER, 0);
    lv_obj_set_style_pad_all(g_target_card, 0, 0);
    lv_obj_clear_flag(g_target_card, LV_OBJ_FLAG_SCROLLABLE);

    g_target_title_lbl = lv_label_create(g_target_card);
    lv_obj_align(g_target_title_lbl, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_font(g_target_title_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_target_title_lbl, lv_color_hex(0x88AACC), 0);

    g_static_target_lbl = lv_label_create(g_target_card);
    lv_obj_align(g_static_target_lbl, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_font(g_static_target_lbl, UI_FONT_CJK_24, 0);
    lv_obj_set_style_text_color(g_static_target_lbl, lv_color_hex(0x00FFFF), 0);

    for (int i = 0; i < 4; i++) {
        g_cell_objs[i] = lv_obj_create(g_target_card);
        lv_obj_set_size(g_cell_objs[i], 24, 30);
        lv_obj_set_style_radius(g_cell_objs[i], 3, 0);
        lv_obj_set_style_border_width(g_cell_objs[i], 1, 0);
        lv_obj_set_style_pad_all(g_cell_objs[i], 0, 0);
        lv_obj_clear_flag(g_cell_objs[i], LV_OBJ_FLAG_SCROLLABLE);

        g_cell_lbls[i] = lv_label_create(g_cell_objs[i]);
        lv_obj_align(g_cell_lbls[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(g_cell_lbls[i], &lv_font_montserrat_20, 0);

        g_arrow_up[i] = lv_label_create(g_target_card);
        lv_label_set_text(g_arrow_up[i], LV_SYMBOL_UP);
        lv_obj_set_style_text_color(g_arrow_up[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(g_arrow_up[i], &lv_font_montserrat_10, 0);
        lv_obj_set_style_opa(g_arrow_up[i], LV_OPA_TRANSP, 0);

        g_arrow_dn[i] = lv_label_create(g_target_card);
        lv_label_set_text(g_arrow_dn[i], LV_SYMBOL_DOWN);
        lv_obj_set_style_text_color(g_arrow_dn[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(g_arrow_dn[i], &lv_font_montserrat_10, 0);
        lv_obj_set_style_opa(g_arrow_dn[i], LV_OPA_TRANSP, 0);
    }

    g_dot_lbl = lv_label_create(g_target_card);
    lv_label_set_text(g_dot_lbl, ".");
    lv_obj_set_style_text_font(g_dot_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g_dot_lbl, lv_color_hex(0x7ECFFF), 0);

    g_dig_hint_lbl = lv_label_create(g_target_card);
    lv_obj_align(g_dig_hint_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_font(g_dig_hint_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_dig_hint_lbl, lv_color_hex(0x88AACC), 0);

    /* 3. Overlay Container */
    g_overlay_cnt = lv_obj_create(g_exec_cnt);
    lv_obj_set_pos(g_overlay_cnt, 0, 0);
    lv_obj_set_size(g_overlay_cnt, SCR_W, 188);
    lv_obj_set_style_bg_color(g_overlay_cnt, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_overlay_cnt, LV_OPA_90, 0);
    lv_obj_set_style_border_width(g_overlay_cnt, 0, 0);
    lv_obj_set_style_radius(g_overlay_cnt, 0, 0);
    lv_obj_set_style_pad_all(g_overlay_cnt, 0, 0);
    lv_obj_clear_flag(g_overlay_cnt, LV_OBJ_FLAG_SCROLLABLE);

    g_overlay_icon = lv_label_create(g_overlay_cnt);
    lv_obj_align(g_overlay_icon, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(g_overlay_icon, &lv_font_montserrat_32, 0);

    g_overlay_text = lv_label_create(g_overlay_cnt);
    lv_obj_align(g_overlay_text, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_text_font(g_overlay_text, UI_FONT_CJK_24, 0);

    g_overlay_subtext = lv_label_create(g_overlay_cnt);
    lv_obj_align(g_overlay_subtext, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_set_style_text_font(g_overlay_subtext, UI_FONT_CJK_14, 0);

    g_overlay_ok_btn = lv_button_create(g_overlay_cnt);
    lv_obj_set_size(g_overlay_ok_btn, 80, 28);
    lv_obj_align(g_overlay_ok_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_color(g_overlay_ok_btn, lv_color_hex(0x1E3A52), 0);
    lv_obj_set_style_border_width(g_overlay_ok_btn, 1, 0);
    lv_obj_set_style_border_color(g_overlay_ok_btn, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_radius(g_overlay_ok_btn, 4, 0);
    lv_obj_add_event_cb(g_overlay_ok_btn, overlay_ok_btn_cb, LV_EVENT_CLICKED, NULL);

    g_overlay_ok_btn_lbl = lv_label_create(g_overlay_ok_btn);
    lv_label_set_text(g_overlay_ok_btn_lbl, "确定 (OK)");
    lv_obj_set_style_text_font(g_overlay_ok_btn_lbl, UI_FONT_CJK_14, 0);
    lv_obj_set_style_text_color(g_overlay_ok_btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_overlay_ok_btn_lbl, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_flag(g_exec_cnt, LV_OBJ_FLAG_HIDDEN);
}

static void update_cells_layout(void)
{
    apply_exec_layout();

    if (g_act_sel == 0) { // 调零：只读单卡，无位选
        return;
    }

    if (g_exec_dig_state == DIG_STATE_BROWSE) {
        lv_obj_remove_flag(g_static_target_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_dot_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_dig_hint_lbl, LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < 4; i++) {
            lv_obj_add_flag(g_cell_objs[i], LV_OBJ_FLAG_HIDDEN);
            if (g_arrow_up[i]) lv_obj_add_flag(g_arrow_up[i], LV_OBJ_FLAG_HIDDEN);
            if (g_arrow_dn[i]) lv_obj_add_flag(g_arrow_dn[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(g_static_target_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(g_dot_lbl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_dig_hint_lbl, LV_OBJ_FLAG_HIDDEN);

        if (g_act_sel == 1) { // Level 1: 3 digits (format XX.X)
            lv_obj_add_flag(g_cell_objs[0], LV_OBJ_FLAG_HIDDEN);
            if (g_arrow_up[0]) lv_obj_add_flag(g_arrow_up[0], LV_OBJ_FLAG_HIDDEN);
            if (g_arrow_dn[0]) lv_obj_add_flag(g_arrow_dn[0], LV_OBJ_FLAG_HIDDEN);

            for (int i = 1; i <= 3; i++) {
                lv_obj_remove_flag(g_cell_objs[i], LV_OBJ_FLAG_HIDDEN);
                if (g_arrow_up[i]) lv_obj_remove_flag(g_arrow_up[i], LV_OBJ_FLAG_HIDDEN);
                if (g_arrow_dn[i]) lv_obj_remove_flag(g_arrow_dn[i], LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_set_pos(g_cell_objs[1], 103, 30);
            if (g_arrow_up[1]) lv_obj_set_pos(g_arrow_up[1], 103 + 7, 30 - 10);
            if (g_arrow_dn[1]) lv_obj_set_pos(g_arrow_dn[1], 103 + 7, 30 + 30 + 1);

            lv_obj_set_pos(g_cell_objs[2], 133, 30);
            if (g_arrow_up[2]) lv_obj_set_pos(g_arrow_up[2], 133 + 7, 30 - 10);
            if (g_arrow_dn[2]) lv_obj_set_pos(g_arrow_dn[2], 133 + 7, 30 + 30 + 1);

            lv_obj_set_pos(g_dot_lbl, 161, 34);

            lv_obj_set_pos(g_cell_objs[3], 171, 30);
            if (g_arrow_up[3]) lv_obj_set_pos(g_arrow_up[3], 171 + 7, 30 - 10);
            if (g_arrow_dn[3]) lv_obj_set_pos(g_arrow_dn[3], 171 + 7, 30 + 30 + 1);
        } else { // Level 2: 4 digits (format XXX.X)
            for (int i = 0; i < 4; i++) {
                lv_obj_remove_flag(g_cell_objs[i], LV_OBJ_FLAG_HIDDEN);
                if (g_arrow_up[i]) lv_obj_remove_flag(g_arrow_up[i], LV_OBJ_FLAG_HIDDEN);
                if (g_arrow_dn[i]) lv_obj_remove_flag(g_arrow_dn[i], LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_set_pos(g_cell_objs[0], 88, 30);
            if (g_arrow_up[0]) lv_obj_set_pos(g_arrow_up[0], 88 + 7, 30 - 10);
            if (g_arrow_dn[0]) lv_obj_set_pos(g_arrow_dn[0], 88 + 7, 30 + 30 + 1);

            lv_obj_set_pos(g_cell_objs[1], 118, 30);
            if (g_arrow_up[1]) lv_obj_set_pos(g_arrow_up[1], 118 + 7, 30 - 10);
            if (g_arrow_dn[1]) lv_obj_set_pos(g_arrow_dn[1], 118 + 7, 30 + 30 + 1);

            lv_obj_set_pos(g_cell_objs[2], 148, 30);
            if (g_arrow_up[2]) lv_obj_set_pos(g_arrow_up[2], 148 + 7, 30 - 10);
            if (g_arrow_dn[2]) lv_obj_set_pos(g_arrow_dn[2], 148 + 7, 30 + 30 + 1);

            lv_obj_set_pos(g_dot_lbl, 176, 34);

            lv_obj_set_pos(g_cell_objs[3], 186, 30);
            if (g_arrow_up[3]) lv_obj_set_pos(g_arrow_up[3], 186 + 7, 30 - 10);
            if (g_arrow_dn[3]) lv_obj_set_pos(g_arrow_dn[3], 186 + 7, 30 + 30 + 1);
        }
    }
}

static void redraw_exec_ui(void)
{
    if (g_state != CALIB_ST_EXECUTION) return;

    gas_ch_t *gas = &g_gas[g_gas_sel];
    float v = gas->value;

    /* 1. Gas Name */
    if (g_act_sel == 0) {
        lv_label_set_text_fmt(g_gas_lbl, "%s (%s)", ui_get_text(gas->name), ui_get_text("实时值"));
    } else {
        lv_label_set_text(g_gas_lbl, ui_get_text(gas->name));
    }

    /* 2. Value and Unit */
    if (g_act_sel == 0) {
        lv_label_set_text_fmt(g_value_lbl, "%.1f", v);
        lv_label_set_text(g_unit_lbl, gas->unit);
    } else {
        lv_label_set_text_fmt(g_value_lbl, "%s：%.1f %s", ui_get_text("实时值"), v, gas->unit);
        lv_obj_align(g_value_lbl, LV_ALIGN_CENTER, 0, 0);
    }

    /* 3. Acceptable Range Label */
    float cal_lo, cal_hi, ok_lo, ok_hi;
    get_calib_ranges(&cal_lo, &cal_hi, &ok_lo, &ok_hi);
    const char *range_key = (g_act_sel == 0) ? "校准范围" : "允许范围";
    lv_label_set_text_fmt(g_range_lbl, "%s: %.1f ~ %.1f %s",
                          ui_get_text(range_key), cal_lo, cal_hi, gas->unit);

    /* 4. Reading status / badge */
    bool ok = (v >= cal_lo && v <= cal_hi);
    if (ok) {
        lv_label_set_text(g_badge_lbl, ui_get_text("可校准"));
        lv_obj_set_style_bg_color(g_badge_lbl, lv_color_hex(0x004400), 0);
        lv_obj_set_style_text_color(g_badge_lbl, lv_color_hex(0x4ECB71), 0);
        lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(g_badge_lbl, ui_get_text("超出范围"));
        lv_obj_set_style_bg_color(g_badge_lbl, lv_color_hex(0x440000), 0);
        lv_obj_set_style_text_color(g_badge_lbl, lv_color_hex(0xE05A5A), 0);
        lv_obj_set_style_text_color(g_value_lbl, lv_color_hex(0xFF0000), 0);
    }

    /* 5. Layout + focus styling */
    update_cells_layout();

    if (g_act_sel == 0) {
        /* Zero: read-only single card, fixed cyan focus border */
        lv_obj_set_style_border_color(g_reading_card, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_bg_color(g_reading_card, lv_color_hex(0x0A1420), 0);
        if (g_zero_tip_lbl) {
            lv_label_set_text(g_zero_tip_lbl,
                              ok ? ui_get_text("按 OK 开始校准")
                                 : ui_get_text("请通入零气"));
            lv_obj_set_style_text_color(g_zero_tip_lbl,
                                        ok ? lv_color_hex(0x7ECFFF) : lv_color_hex(0xE05A5A), 0);
        }
    } else {
        /* Level 1/2: real-time card is read-only, target card displays "校准值" */
        lv_obj_set_style_border_color(g_reading_card, C_BORDER, 0);
        lv_obj_set_style_bg_color(g_reading_card, lv_color_hex(0x0A1420), 0);

        lv_label_set_text(g_target_title_lbl, "");

        bool is_confirm = (g_exec_dig_state == DIG_STATE_CONFIRM);
        bool is_browse = (g_exec_dig_state == DIG_STATE_BROWSE);
        int start_idx = (g_act_sel == 1) ? 1 : 0;

        if (is_browse) {
            float decoded_target = decode_target_value();
            lv_label_set_text_fmt(g_static_target_lbl, "%.1f %s", decoded_target, gas->unit);
            lv_obj_set_style_text_font(g_static_target_lbl, UI_FONT_CJK_24, 0);
            lv_obj_set_style_text_color(g_static_target_lbl, lv_color_hex(0x00FFFF), 0);
            lv_obj_align(g_static_target_lbl, LV_ALIGN_CENTER, 0, 0);
        } else {
            for (int i = 0; i < 4; i++) {
                if (i < start_idx) continue;

                lv_label_set_text_fmt(g_cell_lbls[i], "%d", g_digits[i]);

                bool active = (i == g_active_bit && !is_confirm);
                lv_color_t border = lv_color_hex(0x2A4A6A);
                lv_color_t bg = lv_color_hex(0x1A2A3A);
                lv_color_t col = lv_color_hex(0x00FFFF);

                if (is_confirm) {
                    border = lv_color_hex(0xFFDD00);
                    bg = lv_color_hex(0x2A1A10);
                    col = lv_color_hex(0xFFDD00);
                } else if (active) {
                    if (g_exec_dig_state == DIG_STATE_EDIT) {
                        border = lv_color_hex(0x00FF00);
                        bg = lv_color_hex(0x1A3010);
                        col = lv_color_hex(0x00FF00);
                    } else if (g_exec_dig_state == DIG_STATE_SELECT) {
                        border = lv_color_hex(0xAA88FF);
                        bg = lv_color_hex(0x10203A);
                        col = lv_color_hex(0xAA88FF);
                    }
                }

                lv_obj_set_style_border_color(g_cell_objs[i], border, 0);
                lv_obj_set_style_bg_color(g_cell_objs[i], bg, 0);
                lv_obj_set_style_text_color(g_cell_lbls[i], col, 0);

                // Show arrows on active slot when selecting or editing
                bool show_arrows = active;
                if (g_arrow_up[i]) {
                    lv_obj_set_style_opa(g_arrow_up[i], show_arrows ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
                }
                if (g_arrow_dn[i]) {
                    lv_obj_set_style_opa(g_arrow_dn[i], show_arrows ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
                }
            }
        }

        if (g_exec_dig_state != DIG_STATE_BROWSE) {
            lv_color_t bc = (g_exec_dig_state == DIG_STATE_CONFIRM) ? lv_color_hex(0xFFDD00) : lv_color_hex(0x00FFFF);
            lv_obj_set_style_border_color(g_target_card, bc, 0);
            lv_obj_set_style_bg_color(g_target_card, lv_color_hex(0x0A1420), 0);
        } else {
            lv_obj_set_style_border_color(g_target_card, C_BORDER, 0);
            lv_obj_set_style_bg_color(g_target_card, lv_color_hex(0x0A1420), 0);
        }
    }

    /* 7. Overlay */
    if (g_exec_status == EXEC_STATUS_IDLE) {
        lv_obj_add_flag(g_overlay_cnt, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(g_overlay_cnt, LV_OBJ_FLAG_HIDDEN);
        if (g_exec_status == EXEC_STATUS_CALIBRATING) {
            lv_label_set_text(g_overlay_icon, LV_SYMBOL_LOOP);
            lv_obj_set_style_text_color(g_overlay_icon, lv_color_hex(0xFFDD00), 0);
            lv_label_set_text(g_overlay_text, ui_get_text("校准中..."));
            lv_obj_set_style_text_color(g_overlay_text, lv_color_hex(0xFFDD00), 0);
            lv_label_set_text(g_overlay_subtext, "");
            lv_obj_add_flag(g_overlay_ok_btn, LV_OBJ_FLAG_HIDDEN);
        } else if (g_exec_status == EXEC_STATUS_SUCCESS) {
            lv_label_set_text(g_overlay_icon, LV_SYMBOL_OK);
            lv_obj_set_style_text_color(g_overlay_icon, lv_color_hex(0x4ECB71), 0);
            lv_label_set_text(g_overlay_text, ui_get_text("校准成功"));
            lv_obj_set_style_text_color(g_overlay_text, lv_color_hex(0x4ECB71), 0);
            lv_label_set_text(g_overlay_subtext, ui_get_text("当前值已写入校准系数"));
            lv_obj_remove_flag(g_overlay_ok_btn, LV_OBJ_FLAG_HIDDEN);
        } else if (g_exec_status == EXEC_STATUS_FAIL) {
            lv_label_set_text(g_overlay_icon, LV_SYMBOL_CLOSE);
            lv_obj_set_style_text_color(g_overlay_icon, lv_color_hex(0xE05A5A), 0);
            lv_label_set_text(g_overlay_text, ui_get_text("校准失败"));
            lv_obj_set_style_text_color(g_overlay_text, lv_color_hex(0xE05A5A), 0);
            lv_label_set_text(g_overlay_subtext, ui_get_text("偏差过大,请检查标气或传感器"));
            lv_obj_remove_flag(g_overlay_ok_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 8. Hintbar */
    char h_up[64] = "";
    char h_dn[64] = "";
    char h_ok[64] = "";
    char h_esc[64] = "";

    if (g_show_hint_err) {
        snprintf(h_ok, sizeof(h_ok), "%s", ui_get_text(g_hint_err_text));
    } else if (g_exec_status == EXEC_STATUS_CALIBRATING) {
        // No labels
    } else if (g_exec_status == EXEC_STATUS_SUCCESS || g_exec_status == EXEC_STATUS_FAIL) {
        strcpy(h_ok, "OK 确定");
    } else if (g_act_sel == 0) {
        /* Zero: no edit/adjust, only start or back */
        strcpy(h_ok, ok ? "OK 开始校准" : "OK 不可用");
        strcpy(h_esc, "ESC 返回");
    } else {
        if (g_exec_dig_state == DIG_STATE_BROWSE) {
            strcpy(h_up, "▲ 编辑");
            strcpy(h_dn, "▼ 编辑");
            strcpy(h_ok, "OK 开始校准");
            strcpy(h_esc, "ESC 返回");
        } else if (g_exec_dig_state == DIG_STATE_SELECT) {
            strcpy(h_up, "▲ 选位");
            strcpy(h_dn, "▼ 选位");
            strcpy(h_ok, "OK 编辑");
            strcpy(h_esc, "ESC 放弃");
        } else if (g_exec_dig_state == DIG_STATE_EDIT) {
            strcpy(h_up, "▲ +1");
            strcpy(h_dn, "▼ -1");
            strcpy(h_ok, "OK 下一位");
            strcpy(h_esc, "ESC 返回");
        } else {
            strcpy(h_ok, "OK 保存");
            strcpy(h_esc, "ESC 继续改");
        }
    }

    if (g_hintbar) {
        lv_obj_del(g_hintbar);
    }
    g_hintbar = ui_hintbar_create(g_parent_screen, h_up, h_dn, h_ok, h_esc);

    /* Keep live-refresh hint state in sync so the timer does not rebuild the bar next tick */
    if (g_act_sel == 0 && g_exec_status == EXEC_STATUS_IDLE && !g_show_hint_err) {
        g_live_hint_ok = ok;
        g_live_hint_valid = true;
    }
}

static void calib_redraw(void)
{
    if (g_state == CALIB_ST_EXECUTION) {
        for (int i = 0; i < VISIBLE_ROWS; i++) {
            lv_obj_add_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (g_exec_cnt) {
            lv_obj_remove_flag(g_exec_cnt, LV_OBJ_FLAG_HIDDEN);
        }

        if (g_parent_screen) {
            lv_obj_t *topbar = lv_obj_get_child(g_parent_screen, 0);
            if (topbar && lv_obj_get_child_count(topbar) > 1) {
                lv_obj_t *lbl_title = lv_obj_get_child(topbar, 1);
                if (lbl_title) {
                    lv_label_set_text(lbl_title, ui_get_text(ACTION_NAMES[g_act_sel]));
                }
            }
        }

        redraw_exec_ui();
        return;
    }

    if (g_exec_cnt) {
        lv_obj_add_flag(g_exec_cnt, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_parent_screen) {
        lv_obj_t *topbar = lv_obj_get_child(g_parent_screen, 0);
        if (topbar && lv_obj_get_child_count(topbar) > 1) {
            lv_obj_t *lbl_title = lv_obj_get_child(topbar, 1);
            if (lbl_title) {
                lv_label_set_text(lbl_title, ui_get_text("浓度校准"));
            }
        }
    }

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

    char h_ok[32];
    if (g_state == CALIB_ST_GAS_LIST) {
        strcpy(h_ok, "OK 进入");
    } else {
        strcpy(h_ok, "OK 确认");
    }

    if (g_hintbar) {
        lv_obj_del(g_hintbar);
    }
    g_hintbar = ui_hintbar_create(g_parent_screen, "▲▼ 选择", "", h_ok, "ESC 返回");
}

void ui_calib_key(ui_key_t key)
{
    if (key == KEY_LEFT) key = KEY_ESC;
    else if (key == KEY_RIGHT) key = KEY_OK;

    if (g_state == CALIB_ST_EXECUTION) {
        if (g_exec_status == EXEC_STATUS_SUCCESS || g_exec_status == EXEC_STATUS_FAIL) {
            g_exec_status = EXEC_STATUS_IDLE;
            start_live_timer();
            redraw_exec_ui();
            return;
        }
        if (g_exec_status == EXEC_STATUS_CALIBRATING) {
            return;
        }

        if (g_exec_status == EXEC_STATUS_IDLE) {
            /* ---- Zero calibration: read-only, OK starts, ESC back ---- */
            if (g_act_sel == 0) {
                if (key == KEY_ESC) {
                    stop_live_timer();
                    g_state = CALIB_ST_ACTION_LIST;
                    calib_redraw();
                } else if (key == KEY_OK) {
                    float cal_lo, cal_hi, ok_lo, ok_hi;
                    get_calib_ranges(&cal_lo, &cal_hi, &ok_lo, &ok_hi);
                    float v = g_gas[g_gas_sel].value;
                    if (v >= cal_lo && v <= cal_hi) {
                        stop_live_timer();
                        g_exec_status = EXEC_STATUS_CALIBRATING;
                        redraw_exec_ui();
                        g_calib_timer = lv_timer_create(calib_timer_cb, 900, NULL);
                    } else {
                        g_show_hint_err = true;
                        strcpy(g_hint_err_text, "超出范围,无法校准");
                        redraw_exec_ui();
                        if (g_hint_err_timer) {
                            lv_timer_del(g_hint_err_timer);
                        }
                        g_hint_err_timer = lv_timer_create(hint_err_timer_cb, 1200, NULL);
                    }
                }
                /* UP/DOWN intentionally ignored — no manual adjust */
                return;
            }

            /* ---- Level 1/2 Calibration: digit parameter style editing ---- */
            if (g_exec_dig_state == DIG_STATE_BROWSE) {
                if (key == KEY_UP || key == KEY_DOWN) {
                    g_exec_dig_state = DIG_STATE_SELECT;
                    g_active_bit = (g_act_sel == 1) ? 1 : 0;
                    memcpy(g_orig_digits, g_digits, sizeof(g_digits));
                    redraw_exec_ui();
                } else if (key == KEY_OK) {
                    float cal_lo, cal_hi, ok_lo, ok_hi;
                    get_calib_ranges(&cal_lo, &cal_hi, &ok_lo, &ok_hi);
                    float v = g_gas[g_gas_sel].value;
                    if (v >= cal_lo && v <= cal_hi) {
                        stop_live_timer();
                        g_exec_status = EXEC_STATUS_CALIBRATING;
                        redraw_exec_ui();
                        g_calib_timer = lv_timer_create(calib_timer_cb, 900, NULL);
                    } else {
                        g_show_hint_err = true;
                        strcpy(g_hint_err_text, "超出范围,无法校准");
                        redraw_exec_ui();
                        if (g_hint_err_timer) {
                            lv_timer_del(g_hint_err_timer);
                        }
                        g_hint_err_timer = lv_timer_create(hint_err_timer_cb, 1200, NULL);
                    }
                } else if (key == KEY_ESC) {
                    stop_live_timer();
                    g_state = CALIB_ST_ACTION_LIST;
                    calib_redraw();
                }
            } else if (g_exec_dig_state == DIG_STATE_SELECT) {
                if (key == KEY_UP) {
                    int min_idx = (g_act_sel == 1) ? 1 : 0;
                    if (g_active_bit > min_idx) g_active_bit--;
                    redraw_exec_ui();
                } else if (key == KEY_DOWN) {
                    if (g_active_bit < 3) g_active_bit++;
                    redraw_exec_ui();
                } else if (key == KEY_OK) {
                    g_exec_dig_state = DIG_STATE_EDIT;
                    redraw_exec_ui();
                } else if (key == KEY_ESC) {
                    memcpy(g_digits, g_orig_digits, sizeof(g_digits));
                    g_exec_dig_state = DIG_STATE_BROWSE;
                    redraw_exec_ui();
                }
            } else if (g_exec_dig_state == DIG_STATE_EDIT) {
                if (key == KEY_UP) {
                    g_digits[g_active_bit] = (g_digits[g_active_bit] + 1) % 10;
                    redraw_exec_ui();
                } else if (key == KEY_DOWN) {
                    g_digits[g_active_bit] = (g_digits[g_active_bit] + 9) % 10;
                    redraw_exec_ui();
                } else if (key == KEY_OK) {
                    if (g_active_bit < 3) {
                        g_active_bit++;
                    } else {
                        g_exec_dig_state = DIG_STATE_CONFIRM;
                    }
                    redraw_exec_ui();
                } else if (key == KEY_ESC) {
                    g_exec_dig_state = DIG_STATE_SELECT;
                    redraw_exec_ui();
                }
            } else if (g_exec_dig_state == DIG_STATE_CONFIRM) {
                if (key == KEY_OK) {
                    g_exec_dig_state = DIG_STATE_BROWSE;
                    redraw_exec_ui();
                } else if (key == KEY_ESC) {
                    g_active_bit = 3;
                    g_exec_dig_state = DIG_STATE_EDIT;
                    redraw_exec_ui();
                }
            }
        }
        return;
    }

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
            g_state = CALIB_ST_EXECUTION;
            init_exec_digits();
            calib_redraw();
        }
        break;
    case KEY_ESC:
        if (g_state == CALIB_ST_ACTION_LIST) {
            g_state = CALIB_ST_GAS_LIST;
            calib_redraw();
        } else {
            if (g_calib_timer) {
                lv_timer_del(g_calib_timer);
                g_calib_timer = NULL;
            }
            if (g_hint_err_timer) {
                lv_timer_del(g_hint_err_timer);
                g_hint_err_timer = NULL;
            }
            stop_live_timer();
            ui_goto(PAGE_MENU);
        }
        break;
    }
}

void ui_page_calib_create(lv_obj_t *parent)
{
    g_parent_screen = parent;
    g_state = CALIB_ST_GAS_LIST;
    g_gas_sel = 0;
    g_gas_top = 0;
    g_act_sel = 0;
    g_exec_status = EXEC_STATUS_IDLE;
    g_exec_focus = EXEC_FOCUS_READING;
    g_hintbar = NULL;
    g_calib_timer = NULL;
    g_hint_err_timer = NULL;
    g_live_timer = NULL;
    g_val_cnt = NULL;
    g_zero_tip_lbl = NULL;

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

    create_execution_ui(parent);
    calib_redraw();
}
