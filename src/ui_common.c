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
static lv_obj_t *scr_param;
static lv_obj_t *scr_curve;
static lv_obj_t *scr_datalog;

ui_page_t cur_page = PAGE_HOME;

ui_lang_t g_lang = LANG_CHINESE;

const char *ui_get_text(const char *key)
{
    if (!key) return "";
    
    if (g_lang == LANG_CHINESE) {
        if (strcmp(key, "Current Value") == 0) return "当前值";
        if (strcmp(key, "Alarm Limit") == 0) return "报警限值";
        if (strcmp(key, "Data Points: 1 - 10") == 0) return "数据点: 1 - 10";
        if (strcmp(key, "Channel: %d/%d") == 0) return "通道: %d/%d";
        if (strcmp(key, "ESC Back") == 0) return "ESC 返回";
        if (strcmp(key, "%s Trend") == 0) return "%s 趋势";
        return key;
    } else {
        if (strcmp(key, "气体检测仪") == 0) return "Gas Detector";
        if (strcmp(key, "运行中") == 0) return "Running";
        if (strcmp(key, "最低") == 0) return "Min";
        if (strcmp(key, "最高") == 0) return "Max";
        if (strcmp(key, "报警值") == 0) return "Alarm";
        
        if (strcmp(key, "正常") == 0) return "Normal";
        if (strcmp(key, "注意") == 0) return "Warn";
        if (strcmp(key, "报警") == 0) return "Alarm";
        
        if (strcmp(key, "CO  一氧化碳") == 0) return "CO  Carbon Monox";
        if (strcmp(key, "H2S 硫化氢") == 0) return "H2S Hydrogen Sul";
        if (strcmp(key, "O2  氧气") == 0) return "O2  Oxygen";
        if (strcmp(key, "CH4 甲烷") == 0) return "CH4 Methane";
        if (strcmp(key, "NO2 二氧化氮") == 0) return "NO2 Nitrogen Dio";
        
        if (strcmp(key, "输入密码") == 0) return "Password";
        if (strcmp(key, "系统设置  访问验证") == 0) return "Access Validation";
        if (strcmp(key, "请输入密码") == 0) return "Enter Password";
        if (strcmp(key, "连续错误 3 次，请等待") == 0) return "3 failed, locked";
        if (strcmp(key, "密码错误，还剩 %d 次") == 0) return "Wrong PW, %d left";
        if (strcmp(key, "验证成功!") == 0) return "Success!";
        
        if (strcmp(key, "OK 菜单") == 0) return "OK Menu";
        if (strcmp(key, "▲ +1") == 0) return "▲ +1";
        if (strcmp(key, "▼ −1") == 0) return "▼ -1";
        if (strcmp(key, "OK 下一位") == 0) return "OK Next";
        if (strcmp(key, "ESC 返回") == 0) return "ESC Back";
        
        if (strcmp(key, "系统菜单") == 0) return "System Menu";
        if (strcmp(key, "已验证") == 0) return "Verified";
        if (strcmp(key, "▲▼ 选择") == 0) return "▲▼ Select";
        if (strcmp(key, "OK 进入") == 0) return "OK Enter";
        
        if (strcmp(key, "参数设置") == 0) return "Settings";
        if (strcmp(key, "报警阈值") == 0) return "Alarm Thresh";
        if (strcmp(key, "传感器校准") == 0) return "Calibration";
        if (strcmp(key, "数据记录") == 0) return "Data Log";
        if (strcmp(key, "系统信息") == 0) return "System Info";
        if (strcmp(key, "数据查询") == 0) return "Query Data";
        if (strcmp(key, "数据删除") == 0) return "Delete Data";
        if (strcmp(key, "数据导出") == 0) return "Export Data";
        if (strcmp(key, "存储设置") == 0) return "Storage Set";
        if (strcmp(key, "选择气体") == 0) return "Select Gas";
        if (strcmp(key, "确认删除吗？") == 0) return "Confirm Delete?";
        if (strcmp(key, "正在导出...") == 0) return "Exporting...";
        if (strcmp(key, "导出成功!") == 0) return "Export Success!";
        if (strcmp(key, "存储间隔") == 0) return "Store Interval";
        if (strcmp(key, "循环覆盖") == 0) return "Overwrite";
        if (strcmp(key, "开启") == 0) return "ON";
        if (strcmp(key, "关闭") == 0) return "OFF";
        
        if (strcmp(key, "▲ 上移") == 0) return "▲ Up";
        if (strcmp(key, "▼ 下移") == 0) return "▼ Down";
        if (strcmp(key, "OK 编辑") == 0) return "OK Edit";
        if (strcmp(key, "▲ +") == 0) return "▲ +";
        if (strcmp(key, "▼ -") == 0) return "▼ -";
        if (strcmp(key, "OK 确认") == 0) return "OK Confirm";
        if (strcmp(key, "ESC 放弃") == 0) return "ESC Cancel";
        if (strcmp(key, "OK 保存") == 0) return "OK Save";
        if (strcmp(key, "ESC 继续改") == 0) return "ESC Edit";
        if (strcmp(key, "▲▼ 换位") == 0) return "▲▼ Shift";
        if (strcmp(key, "OK 锁定位") == 0) return "OK Lock";
        if (strcmp(key, "ESC 退出") == 0) return "ESC Exit";
        
        if (strcmp(key, "浏览") == 0) return "Browse";
        if (strcmp(key, "编辑中") == 0) return "Editing";
        if (strcmp(key, "确认?") == 0) return "Confirm?";
        if (strcmp(key, "选择位") == 0) return "Sel Bit";
        if (strcmp(key, "编辑位") == 0) return "Edit Bit";
        if (strcmp(key, "数值") == 0) return "Num";
        if (strcmp(key, "位码") == 0) return "Digit";
        if (strcmp(key, "位%d") == 0) return "Bit %d";
        
        if (strcmp(key, "UP/DOWN 选择位   OK 进入编辑该位") == 0) return "UP/DOWN: Select   OK: Edit";
        if (strcmp(key, "UP/DOWN 修改当前位数字   OK 移到下一位") == 0) return "UP/DOWN: Modify   OK: Next";
        if (strcmp(key, "值: ") == 0) return "Value: ";
        if (strcmp(key, "OK 保存   ESC 取消") == 0) return "OK: Save   ESC: Cancel";
        
        if (strcmp(key, "CO  报警阈值") == 0) return "CO Alarm Limit";
        if (strcmp(key, "H2S 报警阈值") == 0) return "H2S Alarm Limit";
        if (strcmp(key, "采样周期") == 0) return "Sample Cycle";
        if (strcmp(key, "报警延迟") == 0) return "Alarm Delay";
        if (strcmp(key, "背光亮度") == 0) return "Backlight";
        if (strcmp(key, "访问密码") == 0) return "Password";
        if (strcmp(key, "设备编号") == 0) return "Device ID";
        if (strcmp(key, "语言选择") == 0) return "Language";
        if (strcmp(key, "中文") == 0) return "Chinese";
        
        if (strcmp(key, "范围 %.0f - %.0f  步进 %.1f") == 0) return "Range %.0f - %.0f  Step %.1f";
        if (strcmp(key, "原值:") == 0) return "Orig:";
        if (strcmp(key, "数值编辑") == 0) return "Edit Value";
        if (strcmp(key, "确认保存?") == 0) return "Confirm Save?";
        if (strcmp(key, "编辑: %s") == 0) return "Edit: %s";
        
        return key;
    }
}

static void ui_key_catcher_event_cb(lv_event_t *e);

static void ui_hint_label_align(lv_obj_t *lbl, int slot, int hint_count,
                                bool has_up, bool has_dn)
{
    if (hint_count == 1) {
        lv_obj_set_width(lbl, SCR_W - 8);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    if (slot == 3) {
        lv_obj_set_width(lbl, 90);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);
    } else if (slot == 2) {
        lv_obj_set_width(lbl, 100);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, -4, 0);
    } else {
        lv_obj_set_width(lbl, has_up && has_dn ? 70 : 120);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, has_up && has_dn && slot == 0 ? -36 :
                                          has_up && has_dn && slot == 1 ?  36 : 0, 0);
    }
}

static lv_obj_t *ui_create_page(ui_page_t page)
{
    lv_obj_t **slot = NULL;
    void (*create_cb)(lv_obj_t *parent) = NULL;

    if      (page == PAGE_HOME)     { slot = &scr_home;     create_cb = ui_page_home_create; }
    else if (page == PAGE_PASSWORD) { slot = &scr_password; create_cb = ui_page_password_create; }
    else if (page == PAGE_MENU)     { slot = &scr_menu;     create_cb = ui_page_menu_create; }
    else if (page == PAGE_PARAM)    { slot = &scr_param;    create_cb = ui_page_param_create; }
    else if (page == PAGE_CURVE)    { slot = &scr_curve;    create_cb = ui_page_curve_create; }
    else if (page == PAGE_DATALOG)  { slot = &scr_datalog;  create_cb = ui_page_datalog_create; }

    if ((slot == NULL) || (create_cb == NULL)) return NULL;

    if (*slot == NULL) {
        *slot = lv_obj_create(NULL);
        ui_style_screen(*slot);
        lv_obj_add_event_cb(*slot, ui_key_catcher_event_cb, LV_EVENT_KEY, NULL);

        lv_group_t *group = lv_group_get_default();
        if (group) lv_group_add_obj(group, *slot);

        create_cb(*slot);
    }

    return *slot;
}

static void ui_release_inactive_pages(lv_obj_t *keep)
{
    lv_obj_t **screens[] = { &scr_home, &scr_password, &scr_menu, &scr_param, &scr_curve, &scr_datalog };

    for (uint32_t i = 0; i < sizeof(screens) / sizeof(screens[0]); i++) {
        if ((*screens[i] != NULL) && (*screens[i] != keep)) {
            lv_obj_delete_async(*screens[i]);
            *screens[i] = NULL;
        }
    }
}

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
        ui_key_event(KEY_RIGHT);
        break;
    case LV_KEY_ENTER:
        ui_key_event(KEY_OK);
        break;
    case LV_KEY_LEFT:
        ui_key_event(KEY_LEFT);
        break;
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
    (void)badge_text;
    (void)badge_bg;
    (void)badge_col;

    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCR_W, 30);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0A1118), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x1E3A52), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_pad_hor(bar, 8, 0);
    lv_obj_set_style_pad_ver(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    bool home_status_only = (title && (strcmp(title, "气体检测仪") == 0 || strcmp(title, "Gas Detector") == 0));

    /* Status cluster: home = time | temp | icons, other pages = title | temp | icons */
    lv_obj_t *lbl_temp = lv_label_create(bar);
    lv_label_set_text(lbl_temp, "24.3°C");
    lv_obj_set_style_text_color(lbl_temp, lv_color_hex(0xFFDD00), 0);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_12, 0);

    if (home_status_only) {
        lv_obj_t *lbl_time = lv_label_create(bar);
        lv_label_set_text(lbl_time, "12:00");
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_obj_t *lbl_title = lv_label_create(bar);
        lv_label_set_text(lbl_title, ui_get_text(title));
        lv_obj_set_style_text_color(lbl_title, C_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_title, UI_FONT_CJK_14, 0);
        lv_obj_set_width(lbl_title, 120);
        lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_CLIP);  
        lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t *lbl_net = lv_label_create(bar);
    lv_label_set_text(lbl_net, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(lbl_net, C_OK, 0);
    lv_obj_set_style_text_font(lbl_net, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_net, LV_ALIGN_RIGHT_MID, -50, 0);

    lv_obj_t *lbl_bt = lv_label_create(bar);
    lv_label_set_text(lbl_bt, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_color(lbl_bt, C_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_bt, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_bt, LV_ALIGN_RIGHT_MID, -29, 0);

    lv_obj_t *lbl_bat = lv_label_create(bar);
    lv_label_set_text(lbl_bat, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_color(lbl_bat, C_OK, 0);
    lv_obj_set_style_text_font(lbl_bat, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl_bat, LV_ALIGN_RIGHT_MID, -4, 0);

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
    int hint_count = 0;
    for (int i = 0; i < 4; i++) {
        if (hints[i] && hints[i][0] != '\0') hint_count++;
    }
    bool has_up = h_up && h_up[0] != '\0';
    bool has_dn = h_dn && h_dn[0] != '\0';

    for (int i = 0; i < 4; i++) {
        if (!hints[i] || hints[i][0] == '\0') continue;
        lv_obj_t *lbl = lv_label_create(bar);
        lv_label_set_text(lbl, ui_get_text(hints[i]));
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xDFE9F0), 0);
        lv_obj_set_style_text_font(lbl, UI_FONT_CJK_14, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        ui_hint_label_align(lbl, i, hint_count, has_up, has_dn);
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
    if (s == GAS_ALARM) return ui_get_text("报警");
    if (s == GAS_WARN)  return ui_get_text("注意");
    return ui_get_text("正常");
}

/* ─────────────────────────────────────────────────
 *  SCREEN MANAGER
 * ───────────────────────────────────────────────── */

void ui_goto(ui_page_t page)
{
    lv_obj_t *target = ui_create_page(page);

    if (target) {
        lv_screen_load(target);
        ui_release_inactive_pages(target);
        lv_obj_invalidate(target);
        lv_group_t *group = lv_group_get_default();
        if (group) lv_group_focus_obj(target);
        cur_page = page;
    }
}

void ui_refresh_current_page(void)
{
    ui_page_t page = cur_page;
    lv_obj_t *old_scr = NULL;
    if      (page == PAGE_HOME)     { old_scr = scr_home;     scr_home     = NULL; }
    else if (page == PAGE_PASSWORD) { old_scr = scr_password; scr_password = NULL; }
    else if (page == PAGE_MENU)     { old_scr = scr_menu;     scr_menu     = NULL; }
    else if (page == PAGE_PARAM)    { old_scr = scr_param;    scr_param    = NULL; }
    else if (page == PAGE_CURVE)    { old_scr = scr_curve;    scr_curve    = NULL; }
    else if (page == PAGE_DATALOG)  { old_scr = scr_datalog;  scr_datalog  = NULL; }

    ui_goto(page);
    if (old_scr) lv_obj_delete_async(old_scr);
}

/* ─────────────────────────────────────────────────
 *  TOP-LEVEL INIT  (call once from main)
 * ───────────────────────────────────────────────── */

void ui_init(void)
{
    scr_home     = NULL;
    scr_password = NULL;
    scr_menu     = NULL;
    scr_param    = NULL;
    scr_curve    = NULL;
    scr_datalog  = NULL;
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
    if (scr_param) {
        lv_obj_delete(scr_param);
        scr_param = NULL;
    }
    if (scr_curve) {
        lv_obj_delete(scr_curve);
        scr_curve = NULL;
    }
    if (scr_datalog) {
        lv_obj_delete(scr_datalog);
        scr_datalog = NULL;
    }
}
