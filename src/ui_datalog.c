#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── States ──────────────────────────────────────── */
typedef enum {
    DL_STATE_MENU,
    DL_STATE_QUERY_GAS,
    DL_STATE_QUERY_LIST,
    DL_STATE_DELETE,
    DL_STATE_EXPORT,
    DL_STATE_STORAGE
} datalog_state_t;

static datalog_state_t dl_state = DL_STATE_MENU;

/* ── Menu state variables ────────────────────────── */
static int menu_sel = 0;
#define SUBMENU_COUNT 4
static const char *SUBMENU_ITEMS[SUBMENU_COUNT] = {
    "数据查询",
    "数据删除",
    "数据导出",
    "存储设置"
};

/* ── Gas selection state variables ───────────────── */
static int gas_sel = 0;

/* ── Data query list state variables ─────────────── */
static int page_cur = 0;
#define RECORDS_PER_PAGE 5
#define PAGE_MAX 10
#define TOTAL_RECORDS (RECORDS_PER_PAGE * PAGE_MAX)

typedef struct {
    int id;
    char time[16];
    float value;
    float temp;
} log_record_t;

static log_record_t mock_records[5][TOTAL_RECORDS];
static int list_row_sel = 0;

/* ── Data delete state variables ─────────────────── */
static int delete_sel = 0; // 0: Cancel, 1: Confirm
static bool is_deleted = false;

/* ── Data export state variables ─────────────────── */
static int export_sel = 0; // 0: USB, 1: Serial, 2: Cancel
static int export_progress = -1; // -1: not started, 0..100: progress
static lv_timer_t *export_timer = NULL;

/* ── Storage settings state variables ────────────── */
typedef enum {
    SS_BROWSE,
    SS_EDIT_INTERVAL,
    SS_EDIT_OVERWRITE,
    SS_CONFIRM_INTERVAL,
    SS_CONFIRM_OVERWRITE
} storage_state_t;

static storage_state_t s_storage_state = SS_BROWSE;
static int s_temp_interval = 10;
static bool s_temp_overwrite = true;

static int storage_sel = 0; // 0: interval, 1: overwrite, 2: save
static int storage_interval = 10; // 5, 10, 30, 60, 300 seconds
static bool storage_overwrite = true;
static bool settings_saved = false;

/* ── Widget handles ──────────────────────────────── */
static lv_obj_t *dl_parent = NULL;
static lv_obj_t *topbar = NULL;
static lv_obj_t *hintbar = NULL;

// Sub-screen containers
static lv_obj_t *cnt_menu = NULL;
static lv_obj_t *cnt_query_gas = NULL;
static lv_obj_t *cnt_query_list = NULL;
static lv_obj_t *cnt_delete = NULL;
static lv_obj_t *cnt_export = NULL;
static lv_obj_t *cnt_storage = NULL;

// Dynamic labels and elements
static lv_obj_t *menu_rows[SUBMENU_COUNT];
static lv_obj_t *menu_lbls[SUBMENU_COUNT];

static lv_obj_t *gas_rows[5];
static lv_obj_t *gas_lbls[5];

static lv_obj_t *list_rows[RECORDS_PER_PAGE];
static lv_obj_t *list_lbl_id[RECORDS_PER_PAGE];
static lv_obj_t *list_lbl_time[RECORDS_PER_PAGE];
static lv_obj_t *list_lbl_val[RECORDS_PER_PAGE];
static lv_obj_t *list_lbl_temp[RECORDS_PER_PAGE];
static lv_obj_t *lbl_list_info = NULL;

static lv_obj_t *lbl_delete_prompt = NULL;
static lv_obj_t *btn_delete_confirm = NULL;
static lv_obj_t *btn_delete_cancel = NULL;

static lv_obj_t *btn_export_confirm = NULL;
static lv_obj_t *btn_export_cancel = NULL;
static lv_obj_t *export_progress_bar = NULL;
static lv_obj_t *lbl_export_status = NULL;

static lv_obj_t *row_storage_interval = NULL;
static lv_obj_t *lbl_storage_interval_val = NULL;
static lv_obj_t *row_storage_overwrite = NULL;
static lv_obj_t *lbl_storage_overwrite_val = NULL;
static lv_obj_t *btn_storage_save = NULL;
static lv_obj_t *lbl_storage_save = NULL;

/* ── Forward Declarations ────────────────────────── */
static void dl_render(void);
static void generate_mock_data(void);

/* ── Timer Callback for Export Simulation ────────── */
static void export_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (export_progress < 100) {
        export_progress += 20;
        if (export_progress > 100) export_progress = 100;
        dl_render();
    } else {
        lv_timer_del(export_timer);
        export_timer = NULL;
    }
}

/* ── Mock Data Generation ────────────────────────── */
static void generate_mock_data(void)
{
    // Generate realistic timestamps and data points around base values
    for (int g = 0; g < g_gas_count; g++) {
        float base_val = g_gas[g].value;
        float base_temp = 24.3f;
        
        for (int i = 0; i < TOTAL_RECORDS; i++) {
            mock_records[g][i].id = TOTAL_RECORDS - i;
            
            // Generate simple timestamps going backwards
            int mins_back = i * 2;
            int hour = (9 - (mins_back / 60) + 24) % 24;
            int min = (10 - (mins_back % 60) + 60) % 60;
            snprintf(mock_records[g][i].time, sizeof(mock_records[g][i].time), "06-10 %02d:%02d:00", hour, min);
            
            // Value fluctuates slightly
            float val_change = ((float)(rand() % 200 - 100) / 100.0f) * (base_val * 0.1f);
            mock_records[g][i].value = base_val + val_change;
            if (mock_records[g][i].value < 0) mock_records[g][i].value = 0;
            
            // Temperature fluctuates slightly
            float temp_change = ((float)(rand() % 100 - 50) / 100.0f);
            mock_records[g][i].temp = base_temp + temp_change;
        }
    }
}

static void ui_datalog_click_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    
    if (dl_state == DL_STATE_MENU) {
        for (int i = 0; i < SUBMENU_COUNT; i++) {
            if (target == menu_rows[i]) {
                menu_sel = i;
                ui_datalog_key(KEY_OK);
                return;
            }
        }
    } else if (dl_state == DL_STATE_QUERY_GAS) {
        for (int i = 0; i < g_gas_count; i++) {
            if (target == gas_rows[i]) {
                gas_sel = i;
                ui_datalog_key(KEY_OK);
                return;
            }
        }
    } else if (dl_state == DL_STATE_QUERY_LIST) {
        for (int i = 0; i < RECORDS_PER_PAGE; i++) {
            if (target == list_rows[i]) {
                list_row_sel = i;
                dl_render();
                return;
            }
        }
    } else if (dl_state == DL_STATE_DELETE) {
        if (!is_deleted) {
            if (target == btn_delete_cancel) {
                delete_sel = 0;
                ui_datalog_key(KEY_OK);
            } else if (target == btn_delete_confirm) {
                delete_sel = 1;
                ui_datalog_key(KEY_OK);
            }
        } else {
            if (target == btn_delete_cancel) {
                ui_datalog_key(KEY_OK);
            }
        }
    } else if (dl_state == DL_STATE_EXPORT) {
        if (export_progress < 0) {
            if (target == btn_export_cancel) {
                dl_state = DL_STATE_MENU;
                dl_render();
            } else if (target == btn_export_confirm) {
                export_progress = 0;
                export_timer = lv_timer_create(export_timer_cb, 500, NULL);
                dl_render();
            }
        } else if (export_progress == 100) {
            dl_state = DL_STATE_MENU;
            dl_render();
        }
    } else if (dl_state == DL_STATE_STORAGE) {
        if (target == row_storage_interval) {
            storage_sel = 0;
            storage_interval = (storage_interval == 5) ? 10 : (storage_interval == 10) ? 30 : (storage_interval == 30) ? 60 : 5;
            settings_saved = false;
            dl_render();
        } else if (target == row_storage_overwrite) {
            storage_sel = 1;
            storage_overwrite = !storage_overwrite;
            settings_saved = false;
            dl_render();
        } else if (target == btn_storage_save) {
            storage_sel = 2;
            ui_datalog_key(KEY_OK);
        }
    }
}

/* ── Screen Creation ─────────────────────────────── */
void ui_page_datalog_create(lv_obj_t *parent)
{
    dl_parent = parent;
    dl_state = DL_STATE_MENU;
    menu_sel = 0;
    gas_sel = 0;
    page_cur = 0;
    list_row_sel = 0;
    delete_sel = 0;
    is_deleted = false;
    export_sel = 0;
    export_progress = -1;
    storage_sel = 0;
    settings_saved = false;
    
    if (export_timer) {
        lv_timer_del(export_timer);
        export_timer = NULL;
    }

    generate_mock_data();

    // Create containers for all screens (all children of parent)
    int content_h = SCR_H - 30 - 22; // 188 px

    // Top and Bottom Bars (will be dynamically created/rendered in dl_render)
    topbar = NULL;
    hintbar = NULL;

    // 1. Menu Container
    cnt_menu = lv_obj_create(parent);
    lv_obj_set_pos(cnt_menu, 0, 30);
    lv_obj_set_size(cnt_menu, SCR_W, content_h);
    ui_style_screen(cnt_menu);
    lv_obj_clear_flag(cnt_menu, LV_OBJ_FLAG_SCROLLABLE);

    int row_h_px = content_h / 5; // 37 px row height to match system menus
    for (int i = 0; i < SUBMENU_COUNT; i++) {
        lv_obj_t *row = lv_obj_create(cnt_menu);
        lv_obj_set_pos(row, 0, i * row_h_px);
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
        lv_obj_add_event_cb(row, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *cursor = lv_label_create(row);
        lv_label_set_text(cursor, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(cursor, C_ACCENT, 0);
        lv_obj_set_style_text_font(cursor, &lv_font_montserrat_14, 0);
        lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, ui_get_text(SUBMENU_ITEMS[i]));
        lv_obj_set_style_text_color(lbl, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 40, 0);
        menu_lbls[i] = lbl;

        lv_obj_t *chev = lv_label_create(row);
        lv_label_set_text(chev, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(chev, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(chev, &lv_font_montserrat_14, 0);
        lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -10, 0);
    }

    // 2. Gas Selection Container
    cnt_query_gas = lv_obj_create(parent);
    lv_obj_set_pos(cnt_query_gas, 0, 30);
    lv_obj_set_size(cnt_query_gas, SCR_W, content_h);
    ui_style_screen(cnt_query_gas);
    lv_obj_clear_flag(cnt_query_gas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cnt_query_gas, LV_OBJ_FLAG_HIDDEN);

    int gas_row_h = content_h / 5;
    for (int i = 0; i < 5; i++) {
        lv_obj_t *row = lv_obj_create(cnt_query_gas);
        lv_obj_set_pos(row, 0, i * gas_row_h);
        lv_obj_set_size(row, SCR_W, gas_row_h);
        lv_obj_set_style_bg_color(row, C_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        gas_rows[i] = row;
        lv_obj_add_event_cb(row, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *cursor = lv_label_create(row);
        lv_label_set_text(cursor, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(cursor, C_ACCENT, 0);
        lv_obj_set_style_text_font(cursor, &lv_font_montserrat_14, 0);
        lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *lbl = lv_label_create(row);
        if (i < g_gas_count) {
            lv_label_set_text(lbl, ui_get_text(g_gas[i].name));
        } else {
            lv_label_set_text(lbl, "");
        }
        lv_obj_set_style_text_color(lbl, C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_24, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 40, 0);
        gas_lbls[i] = lbl;
    }

    // 3. Query List Container
    cnt_query_list = lv_obj_create(parent);
    lv_obj_set_pos(cnt_query_list, 0, 30);
    lv_obj_set_size(cnt_query_list, SCR_W, content_h);
    ui_style_screen(cnt_query_list);
    lv_obj_clear_flag(cnt_query_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cnt_query_list, LV_OBJ_FLAG_HIDDEN);

    // Create table header (Height: 22 px)
    lv_obj_t *header = lv_obj_create(cnt_query_list);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, SCR_W, 22);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x0A1118), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(header, C_BORDER, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Header labels
    const char *headers[4] = {"序号", "时间", "浓度值", "温度"};
    int col_xs[4] = {4, 45, 175, 255};
    int col_ws[4] = {38, 125, 75, 60};
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t *h_lbl = lv_label_create(header);
        lv_label_set_text(h_lbl, ui_get_text(headers[i]));
        lv_obj_set_style_text_color(h_lbl, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(h_lbl, UI_FONT_CJK_14, 0);
        lv_obj_set_width(h_lbl, col_ws[i]);
        if (i >= 2) {
            lv_obj_set_style_text_align(h_lbl, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_align(h_lbl, LV_ALIGN_RIGHT_MID, -(SCR_W - col_xs[i] - col_ws[i]), 0);
        } else {
            lv_obj_set_style_text_align(h_lbl, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_align(h_lbl, LV_ALIGN_LEFT_MID, col_xs[i], 0);
        }
    }

    // List Rows (Remaining height: 188 - 22 = 166. Dividing by 5 rows is 28 px each, with 26 px usable)
    int query_row_h = 28;
    for (int i = 0; i < RECORDS_PER_PAGE; i++) {
        lv_obj_t *row = lv_obj_create(cnt_query_list);
        lv_obj_set_pos(row, 0, 22 + i * query_row_h);
        lv_obj_set_size(row, SCR_W, query_row_h);
        lv_obj_set_style_bg_color(row, C_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        list_rows[i] = row;
        lv_obj_add_event_cb(row, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

        // ID Label
        list_lbl_id[i] = lv_label_create(row);
        lv_obj_set_style_text_color(list_lbl_id[i], C_TEXT_PRI, 0);
        lv_obj_set_style_text_font(list_lbl_id[i], &lv_font_montserrat_12, 0);
        lv_obj_set_width(list_lbl_id[i], col_ws[0]);
        lv_obj_set_style_text_align(list_lbl_id[i], LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(list_lbl_id[i], LV_ALIGN_LEFT_MID, col_xs[0], 0);

        // Time Label
        list_lbl_time[i] = lv_label_create(row);
        lv_obj_set_style_text_color(list_lbl_time[i], C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(list_lbl_time[i], &lv_font_montserrat_12, 0);
        lv_obj_set_width(list_lbl_time[i], col_ws[1]);
        lv_obj_set_style_text_align(list_lbl_time[i], LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(list_lbl_time[i], LV_ALIGN_LEFT_MID, col_xs[1], 0);

        // Value Label
        list_lbl_val[i] = lv_label_create(row);
        lv_obj_set_style_text_color(list_lbl_val[i], C_OK, 0);
        lv_obj_set_style_text_font(list_lbl_val[i], &lv_font_montserrat_12, 0);
        lv_obj_set_width(list_lbl_val[i], col_ws[2]);
        lv_obj_set_style_text_align(list_lbl_val[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(list_lbl_val[i], LV_ALIGN_RIGHT_MID, -(SCR_W - col_xs[2] - col_ws[2]), 0);

        // Temp Label
        list_lbl_temp[i] = lv_label_create(row);
        lv_obj_set_style_text_color(list_lbl_temp[i], C_TEXT_TEMP, 0);
        lv_obj_set_style_text_font(list_lbl_temp[i], &lv_font_montserrat_12, 0);
        lv_obj_set_width(list_lbl_temp[i], col_ws[3]);
        lv_obj_set_style_text_align(list_lbl_temp[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(list_lbl_temp[i], LV_ALIGN_RIGHT_MID, -(SCR_W - col_xs[3] - col_ws[3]), 0);
    }

    // List Page Info Label at the bottom of content (188 - 26 = 162px, height 26)
    lbl_list_info = lv_label_create(cnt_query_list);
    lv_obj_set_style_text_color(lbl_list_info, C_TEXT_HINT, 0);
    lv_obj_set_style_text_font(lbl_list_info, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_list_info, LV_ALIGN_BOTTOM_MID, 0, -2);

    // 4. Data Delete Container
    cnt_delete = lv_obj_create(parent);
    lv_obj_set_pos(cnt_delete, 0, 30);
    lv_obj_set_size(cnt_delete, SCR_W, content_h);
    ui_style_screen(cnt_delete);
    lv_obj_clear_flag(cnt_delete, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cnt_delete, LV_OBJ_FLAG_HIDDEN);

    lbl_delete_prompt = lv_label_create(cnt_delete);
    lv_label_set_text(lbl_delete_prompt, "确认删除所有历史数据记录吗？");
    lv_obj_set_style_text_color(lbl_delete_prompt, C_TEXT_PRI, 0);
    lv_obj_set_style_text_font(lbl_delete_prompt, UI_FONT_CJK_24, 0);
    lv_obj_align(lbl_delete_prompt, LV_ALIGN_TOP_MID, 0, 30);

    btn_delete_cancel = lv_obj_create(cnt_delete);
    lv_obj_set_size(btn_delete_cancel, 120, 36);
    lv_obj_align(btn_delete_cancel, LV_ALIGN_TOP_MID, -70, 90);
    lv_obj_set_style_border_width(btn_delete_cancel, 1, 0);
    lv_obj_set_style_radius(btn_delete_cancel, 4, 0);
    lv_obj_clear_flag(btn_delete_cancel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_delete_cancel, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_del_can = lv_label_create(btn_delete_cancel);
    lv_label_set_text(lbl_del_can, "取消");
    lv_obj_set_style_text_font(lbl_del_can, UI_FONT_CJK_24, 0);
    lv_obj_center(lbl_del_can);

    btn_delete_confirm = lv_obj_create(cnt_delete);
    lv_obj_set_size(btn_delete_confirm, 120, 36);
    lv_obj_align(btn_delete_confirm, LV_ALIGN_TOP_MID, 70, 90);
    lv_obj_set_style_border_width(btn_delete_confirm, 1, 0);
    lv_obj_set_style_radius(btn_delete_confirm, 4, 0);
    lv_obj_clear_flag(btn_delete_confirm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_delete_confirm, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_del_conf = lv_label_create(btn_delete_confirm);
    lv_label_set_text(lbl_del_conf, "确认删除");
    lv_obj_set_style_text_font(lbl_del_conf, UI_FONT_CJK_24, 0);
    lv_obj_center(lbl_del_conf);

    // 5. Data Export Container
    cnt_export = lv_obj_create(parent);
    lv_obj_set_pos(cnt_export, 0, 30);
    lv_obj_set_size(cnt_export, SCR_W, content_h);
    ui_style_screen(cnt_export);
    lv_obj_clear_flag(cnt_export, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cnt_export, LV_OBJ_FLAG_HIDDEN);

    lbl_export_status = lv_label_create(cnt_export);
    lv_label_set_text(lbl_export_status, "请与电脑进行连接接口");
    lv_obj_set_style_text_color(lbl_export_status, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(lbl_export_status, UI_FONT_CJK_24, 0);
    lv_obj_align(lbl_export_status, LV_ALIGN_TOP_MID, 0, 30);

    btn_export_cancel = lv_obj_create(cnt_export);
    lv_obj_set_size(btn_export_cancel, 120, 36);
    lv_obj_align(btn_export_cancel, LV_ALIGN_TOP_MID, -70, 90);
    lv_obj_set_style_border_width(btn_export_cancel, 1, 0);
    lv_obj_set_style_radius(btn_export_cancel, 4, 0);
    lv_obj_clear_flag(btn_export_cancel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_export_cancel, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_exp_can = lv_label_create(btn_export_cancel);
    lv_label_set_text(lbl_exp_can, "取消");
    lv_obj_set_style_text_font(lbl_exp_can, UI_FONT_CJK_24, 0);
    lv_obj_center(lbl_exp_can);

    btn_export_confirm = lv_obj_create(cnt_export);
    lv_obj_set_size(btn_export_confirm, 120, 36);
    lv_obj_align(btn_export_confirm, LV_ALIGN_TOP_MID, 70, 90);
    lv_obj_set_style_border_width(btn_export_confirm, 1, 0);
    lv_obj_set_style_radius(btn_export_confirm, 4, 0);
    lv_obj_clear_flag(btn_export_confirm, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_export_confirm, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *lbl_exp_conf = lv_label_create(btn_export_confirm);
    lv_label_set_text(lbl_exp_conf, "确认");
    lv_obj_set_style_text_font(lbl_exp_conf, UI_FONT_CJK_24, 0);
    lv_obj_center(lbl_exp_conf);

    export_progress_bar = lv_bar_create(cnt_export);
    lv_obj_set_size(export_progress_bar, 260, 10);
    lv_obj_align(export_progress_bar, LV_ALIGN_TOP_MID, 0, 145);
    lv_obj_set_style_bg_color(export_progress_bar, C_BAR_BG, 0);
    lv_obj_set_style_bg_color(export_progress_bar, C_OK, LV_PART_INDICATOR);
    lv_obj_add_flag(export_progress_bar, LV_OBJ_FLAG_HIDDEN);

    // 6. Storage Settings Container
    cnt_storage = lv_obj_create(parent);
    lv_obj_set_pos(cnt_storage, 0, 30);
    lv_obj_set_size(cnt_storage, SCR_W, content_h);
    ui_style_screen(cnt_storage);
    lv_obj_clear_flag(cnt_storage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cnt_storage, LV_OBJ_FLAG_HIDDEN);

    // Row 1: Interval
    row_storage_interval = lv_obj_create(cnt_storage);
    lv_obj_set_pos(row_storage_interval, 0, 15);
    lv_obj_set_size(row_storage_interval, SCR_W, 35);
    lv_obj_set_style_bg_color(row_storage_interval, C_BG_CARD, 0);
    lv_obj_set_style_bg_opa(row_storage_interval, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row_storage_interval, C_BORDER, 0);
    lv_obj_set_style_border_width(row_storage_interval, 1, 0);
    lv_obj_set_style_radius(row_storage_interval, 0, 0);
    lv_obj_set_style_pad_all(row_storage_interval, 0, 0);
    lv_obj_clear_flag(row_storage_interval, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(row_storage_interval, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_interval_title = lv_label_create(row_storage_interval);
    lv_label_set_text(lbl_interval_title, "存储间隔");
    lv_obj_set_style_text_color(lbl_interval_title, C_TEXT_PRI, 0);
    lv_obj_set_style_text_font(lbl_interval_title, UI_FONT_CJK_24, 0);
    lv_obj_align(lbl_interval_title, LV_ALIGN_LEFT_MID, 15, 0);

    lbl_storage_interval_val = lv_label_create(row_storage_interval);
    lv_obj_set_style_text_color(lbl_storage_interval_val, C_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_storage_interval_val, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_storage_interval_val, LV_ALIGN_RIGHT_MID, -20, 0);

    // Row 2: Overwrite mode
    row_storage_overwrite = lv_obj_create(cnt_storage);
    lv_obj_set_pos(row_storage_overwrite, 0, 60);
    lv_obj_set_size(row_storage_overwrite, SCR_W, 35);
    lv_obj_set_style_bg_color(row_storage_overwrite, C_BG_CARD, 0);
    lv_obj_set_style_bg_opa(row_storage_overwrite, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row_storage_overwrite, C_BORDER, 0);
    lv_obj_set_style_border_width(row_storage_overwrite, 1, 0);
    lv_obj_set_style_radius(row_storage_overwrite, 0, 0);
    lv_obj_set_style_pad_all(row_storage_overwrite, 0, 0);
    lv_obj_clear_flag(row_storage_overwrite, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(row_storage_overwrite, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_overwrite_title = lv_label_create(row_storage_overwrite);
    lv_label_set_text(lbl_overwrite_title, "循环覆盖");
    lv_obj_set_style_text_color(lbl_overwrite_title, C_TEXT_PRI, 0);
    lv_obj_set_style_text_font(lbl_overwrite_title, UI_FONT_CJK_24, 0);
    lv_obj_align(lbl_overwrite_title, LV_ALIGN_LEFT_MID, 15, 0);

    lbl_storage_overwrite_val = lv_label_create(row_storage_overwrite);
    lv_obj_set_style_text_color(lbl_storage_overwrite_val, C_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_storage_overwrite_val, UI_FONT_CJK_24, 0);
    lv_obj_align(lbl_storage_overwrite_val, LV_ALIGN_RIGHT_MID, -20, 0);

    // Row 3: Save button
    btn_storage_save = lv_obj_create(cnt_storage);
    lv_obj_set_pos(btn_storage_save, 20, 110);
    lv_obj_set_size(btn_storage_save, SCR_W - 40, 36);
    lv_obj_set_style_border_width(btn_storage_save, 1, 0);
    lv_obj_set_style_radius(btn_storage_save, 4, 0);
    lv_obj_clear_flag(btn_storage_save, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_storage_save, ui_datalog_click_cb, LV_EVENT_CLICKED, NULL);

    lbl_storage_save = lv_label_create(btn_storage_save);
    lv_label_set_text(lbl_storage_save, "保存设置");
    lv_obj_set_style_text_font(lbl_storage_save, UI_FONT_CJK_24, 0);
    lv_obj_center(lbl_storage_save);

    dl_render();
}

/* ── Render State Updates ────────────────────────── */
static void dl_render(void)
{
    // Recreate topbar and hintbar based on state to avoid index crashes
    if (topbar) {
        lv_obj_delete(topbar);
        topbar = NULL;
    }
    if (hintbar) {
        lv_obj_delete(hintbar);
        hintbar = NULL;
    }

    char title_buf[64] = "";
    if (dl_state == DL_STATE_MENU) {
        snprintf(title_buf, sizeof(title_buf), "%s", ui_get_text("数据记录"));
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        hintbar = ui_hintbar_create(dl_parent, "▲▼ 选择", "", "OK 进入", "ESC 返回");
    } else if (dl_state == DL_STATE_QUERY_GAS) {
        snprintf(title_buf, sizeof(title_buf), "%s", ui_get_text("选择气体"));
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        hintbar = ui_hintbar_create(dl_parent, "▲▼ 选择", "", "OK 确定", "ESC 返回");
    } else if (dl_state == DL_STATE_QUERY_LIST) {
        snprintf(title_buf, sizeof(title_buf), "%s - %s", ui_get_text("数据查询"), g_gas[gas_sel].symbol);
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        hintbar = ui_hintbar_create(dl_parent, "▲▼ 翻页", "", "", "◀ 返回");
    } else if (dl_state == DL_STATE_DELETE) {
        snprintf(title_buf, sizeof(title_buf), "%s", ui_get_text("数据删除"));
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        if (is_deleted) {
            hintbar = ui_hintbar_create(dl_parent, "", "", "OK 返回", "ESC 返回");
        } else {
            hintbar = ui_hintbar_create(dl_parent, "▲▼ 选择", "", "OK 确定", "ESC 返回");
        }
    } else if (dl_state == DL_STATE_EXPORT) {
        snprintf(title_buf, sizeof(title_buf), "%s", ui_get_text("数据导出"));
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        if (export_progress >= 0) {
            if (export_progress == 100) {
                hintbar = ui_hintbar_create(dl_parent, "", "", "OK 返回", "ESC 返回");
            } else {
                hintbar = ui_hintbar_create(dl_parent, "", "", "", "ESC 取消");
            }
        } else {
            hintbar = ui_hintbar_create(dl_parent, "▲▼ 选择", "", "OK 确定", "ESC 返回");
        }
    } else if (dl_state == DL_STATE_STORAGE) {
        snprintf(title_buf, sizeof(title_buf), "%s", ui_get_text("存储设置"));
        topbar = ui_topbar_create(dl_parent, title_buf, "已验证", C_BADGE_OK_BG, C_OK);
        if (s_storage_state == SS_BROWSE) {
            hintbar = ui_hintbar_create(dl_parent, "▲▼ 选择", "", (storage_sel == 2) ? "OK 保存" : "OK 编辑", "ESC 返回");
        } else if (s_storage_state == SS_EDIT_INTERVAL || s_storage_state == SS_EDIT_OVERWRITE) {
            hintbar = ui_hintbar_create(dl_parent, "▲▼ 更改", "", "OK 确认", "ESC 放弃");
        } else {
            hintbar = ui_hintbar_create(dl_parent, "", "", "OK 保存", "ESC 返回");
        }
    }

    // Hide all containers first
    lv_obj_add_flag(cnt_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cnt_query_gas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cnt_query_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cnt_delete, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cnt_export, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cnt_storage, LV_OBJ_FLAG_HIDDEN);

    if (dl_state == DL_STATE_MENU) {
        lv_obj_remove_flag(cnt_menu, LV_OBJ_FLAG_HIDDEN);
        
        // Update menu list styles
        for (int i = 0; i < SUBMENU_COUNT; i++) {
            bool selected = (i == menu_sel);
            lv_obj_set_style_bg_color(menu_rows[i], selected ? C_PW_ACTIVE : C_BG_CARD, 0);
            lv_obj_set_style_border_color(menu_rows[i], selected ? C_PW_BORDER : C_BORDER, 0);
            
            // Show/Hide selection cursor
            lv_obj_t *cursor = lv_obj_get_child(menu_rows[i], 0);
            if (selected) {
                lv_obj_remove_flag(cursor, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(cursor, LV_OBJ_FLAG_HIDDEN);
            }
        }
        
    } else if (dl_state == DL_STATE_QUERY_GAS) {
        lv_obj_remove_flag(cnt_query_gas, LV_OBJ_FLAG_HIDDEN);
        
        for (int i = 0; i < 5; i++) {
            if (i < g_gas_count) {
                lv_obj_remove_flag(gas_rows[i], LV_OBJ_FLAG_HIDDEN);
                bool selected = (i == gas_sel);
                lv_obj_set_style_bg_color(gas_rows[i], selected ? C_PW_ACTIVE : C_BG_CARD, 0);
                lv_obj_set_style_border_color(gas_rows[i], selected ? C_PW_BORDER : C_BORDER, 0);
                
                lv_obj_t *cursor = lv_obj_get_child(gas_rows[i], 0);
                if (selected) {
                    lv_obj_remove_flag(cursor, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(cursor, LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                lv_obj_add_flag(gas_rows[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

    } else if (dl_state == DL_STATE_QUERY_LIST) {
        lv_obj_remove_flag(cnt_query_list, LV_OBJ_FLAG_HIDDEN);

        // Draw list rows
        int start_idx = page_cur * RECORDS_PER_PAGE;
        
        for (int i = 0; i < RECORDS_PER_PAGE; i++) {
            int current_idx = start_idx + i;
            bool selected = false;
            
            lv_obj_set_style_bg_color(list_rows[i], selected ? C_PW_ACTIVE : C_BG_CARD, 0);
            lv_obj_set_style_border_color(list_rows[i], selected ? C_PW_BORDER : C_BORDER, 0);
            
            if (is_deleted) {
                lv_label_set_text(list_lbl_id[i], "-");
                lv_label_set_text(list_lbl_time[i], "--:--:--");
                lv_label_set_text(list_lbl_val[i], "-");
                lv_label_set_text(list_lbl_temp[i], "-");
            } else {
                log_record_t *rec = &mock_records[gas_sel][current_idx];
                
                char id_buf[16];
                snprintf(id_buf, sizeof(id_buf), "%d", rec->id);
                lv_label_set_text(list_lbl_id[i], id_buf);
                
                lv_label_set_text(list_lbl_time[i], rec->time);
                
                char val_buf[32];
                snprintf(val_buf, sizeof(val_buf), "%.1f %s", rec->value, g_gas[gas_sel].unit);
                lv_label_set_text(list_lbl_val[i], val_buf);
                
                char temp_buf[32];
                snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", rec->temp);
                lv_label_set_text(list_lbl_temp[i], temp_buf);
            }
        }

        // Page info
        char info_buf[64];
        if (is_deleted) {
            snprintf(info_buf, sizeof(info_buf), "%s: 0/0", ui_get_text("页码"));
        } else {
            snprintf(info_buf, sizeof(info_buf), "%s: %d/%d", ui_get_text("页码"), page_cur + 1, PAGE_MAX);
        }
        lv_label_set_text(lbl_list_info, info_buf);

    } else if (dl_state == DL_STATE_DELETE) {
        lv_obj_remove_flag(cnt_delete, LV_OBJ_FLAG_HIDDEN);

        if (is_deleted) {
            lv_label_set_text(lbl_delete_prompt, "历史数据记录已全部清空！");
            lv_obj_add_flag(btn_delete_confirm, LV_OBJ_FLAG_HIDDEN);
            
            // Centering Cancel (Back) button
            lv_obj_align(btn_delete_cancel, LV_ALIGN_TOP_MID, 0, 90);
            lv_obj_set_style_bg_color(btn_delete_cancel, C_PW_ACTIVE, 0);
            lv_obj_set_style_border_color(btn_delete_cancel, C_PW_BORDER, 0);
            
            lv_obj_t *lbl = lv_obj_get_child(btn_delete_cancel, 0);
            lv_label_set_text(lbl, "返回");
            lv_obj_set_style_text_color(lbl, C_TEXT_PRI, 0);
        } else {
            lv_label_set_text(lbl_delete_prompt, "确认删除所有历史数据？");
            lv_obj_remove_flag(btn_delete_confirm, LV_OBJ_FLAG_HIDDEN);
            
            lv_obj_align(btn_delete_cancel, LV_ALIGN_TOP_MID, -70, 90);
            lv_obj_align(btn_delete_confirm, LV_ALIGN_TOP_MID, 70, 90);
            
            lv_obj_t *lbl_can = lv_obj_get_child(btn_delete_cancel, 0);
            lv_label_set_text(lbl_can, "取消");

            lv_obj_t *lbl_conf = lv_obj_get_child(btn_delete_confirm, 0);
            lv_label_set_text(lbl_conf, "确认");

            bool confirm_selected = (delete_sel == 1);
            if (confirm_selected) {
                lv_obj_set_style_text_color(lbl_can, lv_color_hex(0xD0DFEE), 0);
                lv_obj_set_style_bg_color(btn_delete_cancel, C_BG_CARD, 0);
                lv_obj_set_style_border_color(btn_delete_cancel, C_BORDER, 0);
                
                lv_obj_set_style_text_color(lbl_conf, C_TEXT_PRI, 0);
                lv_obj_set_style_bg_color(btn_delete_confirm, C_ALARM, 0);
                lv_obj_set_style_border_color(btn_delete_confirm, C_ALARM, 0);
            } else {
                lv_obj_set_style_text_color(lbl_can, C_TEXT_PRI, 0);
                lv_obj_set_style_bg_color(btn_delete_cancel, C_PW_ACTIVE, 0);
                lv_obj_set_style_border_color(btn_delete_cancel, C_PW_BORDER, 0);
                
                lv_obj_set_style_text_color(lbl_conf, lv_color_hex(0xD0DFEE), 0);
                lv_obj_set_style_bg_color(btn_delete_confirm, C_BG_CARD, 0);
                lv_obj_set_style_border_color(btn_delete_confirm, C_BORDER, 0);
            }
        }

    } else if (dl_state == DL_STATE_EXPORT) {
        lv_obj_remove_flag(cnt_export, LV_OBJ_FLAG_HIDDEN);

        if (export_progress >= 0) {
            lv_obj_add_flag(btn_export_confirm, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(btn_export_cancel, LV_OBJ_FLAG_HIDDEN);
            
            lv_obj_remove_flag(export_progress_bar, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(export_progress_bar, export_progress, LV_ANIM_ON);
            
            char status_buf[64];
            if (export_progress < 100) {
                snprintf(status_buf, sizeof(status_buf), "%s %d%%", ui_get_text("正在导出..."), export_progress);
                lv_obj_set_style_text_color(lbl_export_status, C_WARN, 0);
            } else {
                snprintf(status_buf, sizeof(status_buf), "%s", ui_get_text("导出成功!"));
                lv_obj_set_style_text_color(lbl_export_status, C_OK, 0);
            }
            lv_label_set_text(lbl_export_status, status_buf);
        } else {
            lv_obj_remove_flag(btn_export_confirm, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(btn_export_cancel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(export_progress_bar, LV_OBJ_FLAG_HIDDEN);
            
            lv_label_set_text(lbl_export_status, ui_get_text("请与电脑进行连接接口"));
            lv_obj_set_style_text_color(lbl_export_status, lv_color_hex(0x88AACC), 0);

            lv_obj_t *lbl_can = lv_obj_get_child(btn_export_cancel, 0);
            lv_obj_t *lbl_conf = lv_obj_get_child(btn_export_confirm, 0);
            
            bool confirm_selected = (export_sel == 1);
            if (confirm_selected) {
                lv_obj_set_style_bg_color(btn_export_cancel, C_BG_CARD, 0);
                lv_obj_set_style_border_color(btn_export_cancel, C_BORDER, 0);
                lv_obj_set_style_text_color(lbl_can, lv_color_hex(0xD0DFEE), 0);

                lv_obj_set_style_bg_color(btn_export_confirm, C_PW_ACTIVE, 0);
                lv_obj_set_style_border_color(btn_export_confirm, C_PW_BORDER, 0);
                lv_obj_set_style_text_color(lbl_conf, C_TEXT_PRI, 0);
            } else {
                lv_obj_set_style_bg_color(btn_export_cancel, C_PW_ACTIVE, 0);
                lv_obj_set_style_border_color(btn_export_cancel, C_PW_BORDER, 0);
                lv_obj_set_style_text_color(lbl_can, C_TEXT_PRI, 0);

                lv_obj_set_style_bg_color(btn_export_confirm, C_BG_CARD, 0);
                lv_obj_set_style_border_color(btn_export_confirm, C_BORDER, 0);
                lv_obj_set_style_text_color(lbl_conf, lv_color_hex(0xD0DFEE), 0);
            }
        }

    } else if (dl_state == DL_STATE_STORAGE) {
        lv_obj_remove_flag(cnt_storage, LV_OBJ_FLAG_HIDDEN);

        char int_buf[64];
        char ov_buf[64];
        
        lv_color_t col_int = C_ACCENT;
        lv_color_t col_ov = C_ACCENT;
        
        lv_color_t bg_int = C_BG_CARD;
        lv_color_t border_int = C_BORDER;
        
        lv_color_t bg_ov = C_BG_CARD;
        lv_color_t border_ov = C_BORDER;

        if (s_storage_state == SS_BROWSE) {
            snprintf(int_buf, sizeof(int_buf), "%d s", storage_interval);
            snprintf(ov_buf, sizeof(ov_buf), "%s", storage_overwrite ? ui_get_text("开启") : ui_get_text("关闭"));
            col_ov = storage_overwrite ? C_OK : C_ALARM;
            
            if (storage_sel == 0) {
                bg_int = C_PW_ACTIVE;
                border_int = C_PW_BORDER;
            } else if (storage_sel == 1) {
                bg_ov = C_PW_ACTIVE;
                border_ov = C_PW_BORDER;
            }
        } else if (s_storage_state == SS_EDIT_INTERVAL) {
            snprintf(int_buf, sizeof(int_buf), "%d s", s_temp_interval);
            snprintf(ov_buf, sizeof(ov_buf), "%s", storage_overwrite ? ui_get_text("开启") : ui_get_text("关闭"));
            col_ov = storage_overwrite ? C_OK : C_ALARM;
            
            col_int = C_WARN;
            bg_int = lv_color_hex(0x2A1F0F);
            border_int = C_WARN;
        } else if (s_storage_state == SS_CONFIRM_INTERVAL) {
            snprintf(int_buf, sizeof(int_buf), "%d s (%s)", s_temp_interval, ui_get_text("确认?"));
            snprintf(ov_buf, sizeof(ov_buf), "%s", storage_overwrite ? ui_get_text("开启") : ui_get_text("关闭"));
            col_ov = storage_overwrite ? C_OK : C_ALARM;
            
            col_int = C_ALARM;
            bg_int = lv_color_hex(0x2F1313);
            border_int = C_ALARM;
        } else if (s_storage_state == SS_EDIT_OVERWRITE) {
            snprintf(int_buf, sizeof(int_buf), "%d s", storage_interval);
            snprintf(ov_buf, sizeof(ov_buf), "%s", s_temp_overwrite ? ui_get_text("开启") : ui_get_text("关闭"));
            
            col_ov = C_WARN;
            bg_ov = lv_color_hex(0x2A1F0F);
            border_ov = C_WARN;
        } else if (s_storage_state == SS_CONFIRM_OVERWRITE) {
            snprintf(int_buf, sizeof(int_buf), "%d s", storage_interval);
            snprintf(ov_buf, sizeof(ov_buf), "%s (%s)", s_temp_overwrite ? ui_get_text("开启") : ui_get_text("关闭"), ui_get_text("确认?"));
            
            col_ov = C_ALARM;
            bg_ov = lv_color_hex(0x2F1313);
            border_ov = C_ALARM;
        }

        lv_label_set_text(lbl_storage_interval_val, int_buf);
        lv_obj_set_style_text_color(lbl_storage_interval_val, col_int, 0);
        
        lv_label_set_text(lbl_storage_overwrite_val, ov_buf);
        lv_obj_set_style_text_color(lbl_storage_overwrite_val, col_ov, 0);

        lv_obj_set_style_bg_color(row_storage_interval, bg_int, 0);
        lv_obj_set_style_border_color(row_storage_interval, border_int, 0);

        lv_obj_set_style_bg_color(row_storage_overwrite, bg_ov, 0);
        lv_obj_set_style_border_color(row_storage_overwrite, border_ov, 0);

        bool is_save_sel = (storage_sel == 2 && s_storage_state == SS_BROWSE);
        lv_obj_set_style_bg_color(btn_storage_save, is_save_sel ? (settings_saved ? C_BADGE_OK_BG : C_PW_ACTIVE) : C_BG_CARD, 0);
        lv_obj_set_style_border_color(btn_storage_save, is_save_sel ? (settings_saved ? C_OK : C_PW_BORDER) : C_BORDER, 0);
        
        if (settings_saved) {
            lv_label_set_text(lbl_storage_save, "已保存");
            lv_obj_set_style_text_color(lbl_storage_save, C_OK, 0);
        } else {
            lv_label_set_text(lbl_storage_save, "保存设置");
            lv_obj_set_style_text_color(lbl_storage_save, C_TEXT_PRI, 0);
        }
    }
}

/* ── Key Press Handler ───────────────────────────── */
void ui_datalog_key(ui_key_t key)
{
    if (dl_state == DL_STATE_MENU) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) key = KEY_OK;
    } else if (dl_state == DL_STATE_QUERY_GAS) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) key = KEY_OK;
    } else if (dl_state == DL_STATE_DELETE) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) key = KEY_OK;
    } else if (dl_state == DL_STATE_EXPORT) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) key = KEY_OK;
    } else if (dl_state == DL_STATE_QUERY_LIST) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) return; // Right key does nothing
    } else if (dl_state == DL_STATE_STORAGE) {
        if (key == KEY_LEFT) key = KEY_ESC;
        else if (key == KEY_RIGHT) key = KEY_OK;
    }

    if (dl_state == DL_STATE_MENU) {
        if (key == KEY_UP) {
            menu_sel = (menu_sel - 1 + SUBMENU_COUNT) % SUBMENU_COUNT;
            dl_render();
        } else if (key == KEY_DOWN) {
            menu_sel = (menu_sel + 1) % SUBMENU_COUNT;
            dl_render();
        } else if (key == KEY_OK) {
            if (menu_sel == 0) {
                dl_state = DL_STATE_QUERY_GAS;
                gas_sel = 0;
            } else if (menu_sel == 1) {
                dl_state = DL_STATE_DELETE;
                delete_sel = 0;
            } else if (menu_sel == 2) {
                dl_state = DL_STATE_EXPORT;
                export_sel = 0;
                export_progress = -1;
            } else if (menu_sel == 3) {
                dl_state = DL_STATE_STORAGE;
                storage_sel = 0;
                settings_saved = false;
            }
            dl_render();
        } else if (key == KEY_ESC) {
            ui_goto(PAGE_MENU);
        }
        
    } else if (dl_state == DL_STATE_QUERY_GAS) {
        if (key == KEY_UP) {
            gas_sel = (gas_sel - 1 + g_gas_count) % g_gas_count;
            dl_render();
        } else if (key == KEY_DOWN) {
            gas_sel = (gas_sel + 1) % g_gas_count;
            dl_render();
        } else if (key == KEY_OK) {
            dl_state = DL_STATE_QUERY_LIST;
            page_cur = 0;
            list_row_sel = 0;
            dl_render();
        } else if (key == KEY_ESC) {
            dl_state = DL_STATE_MENU;
            dl_render();
        }

    } else if (dl_state == DL_STATE_QUERY_LIST) {
        if (key == KEY_UP) {
            if (page_cur > 0) {
                page_cur--;
                dl_render();
            }
        } else if (key == KEY_DOWN) {
            if (page_cur < PAGE_MAX - 1) {
                page_cur++;
                dl_render();
            }
        } else if (key == KEY_ESC) {
            dl_state = DL_STATE_QUERY_GAS;
            dl_render();
        }

    } else if (dl_state == DL_STATE_DELETE) {
        if (is_deleted) {
            if (key == KEY_OK || key == KEY_ESC) {
                dl_state = DL_STATE_MENU;
                dl_render();
            }
        } else {
            if (key == KEY_UP || key == KEY_DOWN) {
                delete_sel = (delete_sel == 0) ? 1 : 0;
                dl_render();
            } else if (key == KEY_OK) {
                if (delete_sel == 1) {
                    is_deleted = true;
                } else {
                    dl_state = DL_STATE_MENU;
                }
                dl_render();
            } else if (key == KEY_ESC) {
                dl_state = DL_STATE_MENU;
                dl_render();
            }
        }

    } else if (dl_state == DL_STATE_EXPORT) {
        if (export_progress >= 0) {
            if (export_progress == 100 && (key == KEY_OK || key == KEY_ESC)) {
                dl_state = DL_STATE_MENU;
                dl_render();
            } else if (export_progress < 100 && key == KEY_ESC) {
                if (export_timer) {
                    lv_timer_del(export_timer);
                    export_timer = NULL;
                }
                export_progress = -1;
                dl_render();
            }
        } else {
            if (key == KEY_UP || key == KEY_DOWN) {
                export_sel = (export_sel == 0) ? 1 : 0;
                dl_render();
            } else if (key == KEY_OK) {
                if (export_sel == 1) {
                    export_progress = 0;
                    export_timer = lv_timer_create(export_timer_cb, 500, NULL);
                } else {
                    dl_state = DL_STATE_MENU;
                }
                dl_render();
            } else if (key == KEY_ESC) {
                dl_state = DL_STATE_MENU;
                dl_render();
            }
        }

    } else if (dl_state == DL_STATE_STORAGE) {
        if (s_storage_state == SS_BROWSE) {
            if (key == KEY_UP) {
                storage_sel = (storage_sel - 1 + 3) % 3;
                settings_saved = false;
                dl_render();
            } else if (key == KEY_DOWN) {
                storage_sel = (storage_sel + 1) % 3;
                settings_saved = false;
                dl_render();
            } else if (key == KEY_OK) {
                if (storage_sel == 0) {
                    s_temp_interval = storage_interval;
                    s_storage_state = SS_EDIT_INTERVAL;
                } else if (storage_sel == 1) {
                    s_temp_overwrite = storage_overwrite;
                    s_storage_state = SS_EDIT_OVERWRITE;
                } else if (storage_sel == 2) {
                    settings_saved = true;
                }
                dl_render();
            } else if (key == KEY_ESC) {
                dl_state = DL_STATE_MENU;
                dl_render();
            }
        } else if (s_storage_state == SS_EDIT_INTERVAL) {
            if (key == KEY_UP) {
                if (s_temp_interval == 5) s_temp_interval = 10;
                else if (s_temp_interval == 10) s_temp_interval = 30;
                else if (s_temp_interval == 30) s_temp_interval = 60;
                else if (s_temp_interval == 60) s_temp_interval = 300;
                dl_render();
            } else if (key == KEY_DOWN) {
                if (s_temp_interval == 300) s_temp_interval = 60;
                else if (s_temp_interval == 60) s_temp_interval = 30;
                else if (s_temp_interval == 30) s_temp_interval = 10;
                else if (s_temp_interval == 10) s_temp_interval = 5;
                dl_render();
            } else if (key == KEY_OK) {
                s_storage_state = SS_CONFIRM_INTERVAL;
                dl_render();
            } else if (key == KEY_ESC) {
                s_storage_state = SS_BROWSE;
                dl_render();
            }
        } else if (s_storage_state == SS_CONFIRM_INTERVAL) {
            if (key == KEY_OK) {
                storage_interval = s_temp_interval;
                s_storage_state = SS_BROWSE;
                settings_saved = false;
                dl_render();
            } else if (key == KEY_ESC) {
                s_storage_state = SS_EDIT_INTERVAL;
                dl_render();
            }
        } else if (s_storage_state == SS_EDIT_OVERWRITE) {
            if (key == KEY_UP || key == KEY_DOWN) {
                s_temp_overwrite = !s_temp_overwrite;
                dl_render();
            } else if (key == KEY_OK) {
                s_storage_state = SS_CONFIRM_OVERWRITE;
                dl_render();
            } else if (key == KEY_ESC) {
                s_storage_state = SS_BROWSE;
                dl_render();
            }
        } else if (s_storage_state == SS_CONFIRM_OVERWRITE) {
            if (key == KEY_OK) {
                storage_overwrite = s_temp_overwrite;
                s_storage_state = SS_BROWSE;
                settings_saved = false;
                dl_render();
            } else if (key == KEY_ESC) {
                s_storage_state = SS_EDIT_OVERWRITE;
                dl_render();
            }
        }
    }
}
