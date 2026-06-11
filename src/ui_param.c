/**
 * ui_param.c
 * 参数设置界面 – 支持两种编辑模式
 *
 * 模式一：数值递增/递减  (type = PARAM_NUM)
 *   浏览 → 编辑(UP/DN调整) → 确认 → 写入
 *
 * 模式二：逐位编辑        (type = PARAM_DIGIT)
 *   浏览 → 位选择(UP/DN换位) → 位编辑(UP/DN改单位) → 确认 → 写入
 *
 * 按键映射（4个实体按键）:
 *   UP   – 上移光标 / 值+1 / 位光标左
 *   DOWN – 下移光标 / 值-1 / 位光标右
 *   OK   – 进入编辑 / 下一步 / 确认保存
 *   ESC  – 取消 / 返回上一层
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

/* ══════════════════════════════════════════════════
 *  参数类型与描述表
 * ══════════════════════════════════════════════════ */
typedef enum { PARAM_NUM, PARAM_DIGIT } param_type_t;

typedef struct {
    const char   *name;
    const char   *unit;
    param_type_t  type;
    /* PARAM_NUM */
    float         val;
    float         val_min;
    float         val_max;
    float         step;
    /* PARAM_DIGIT */
    uint8_t       digits[8];
    uint8_t       digit_count;
} param_t;

static param_t g_params[] = {
    { "CO  报警阈值",  "ppm", PARAM_NUM,    50.0f,   0, 150,  1.0f, {0}, 0 },
    { "H2S 报警阈值", "ppm", PARAM_NUM,    10.0f,   0,  50,  0.5f, {0}, 0 },
    { "采样周期",      "ms",  PARAM_NUM,   500.0f, 100,5000,100.0f, {0}, 0 },
    { "报警延迟",      "s",   PARAM_NUM,     3.0f,   0,  30,  1.0f, {0}, 0 },
    { "背光亮度",      "%",   PARAM_NUM,    80.0f,  10, 100, 10.0f, {0}, 0 },
    { "访问密码",      "",    PARAM_DIGIT,   0,      0,   0,  0,    {0,8,5,2,1}, 5 },
    { "设备编号",      "",    PARAM_DIGIT,   0,      0,   0,  0,    {0,0,0,4,2}, 5 },
    { "语言选择",      "",    PARAM_NUM,     0.0f,   0,   1,  1.0f, {0}, 0 },
};
#define PARAM_COUNT  ((int)(sizeof(g_params)/sizeof(g_params[0])))

/* ══════════════════════════════════════════════════
 *  状态机
 * ══════════════════════════════════════════════════ */
typedef enum {
    PS_BROWSE,
    PS_NUM_EDIT,
    PS_NUM_CONFIRM,
    PS_DIG_SELECT,
    PS_DIG_EDIT,
    PS_DIG_CONFIRM,
} param_state_t;

static param_state_t  g_state      = PS_BROWSE;
static int            g_cursor     = 0;
static float          g_edit_val   = 0;
static float          g_orig_val   = 0;
static uint8_t        g_edit_dig[8];
static uint8_t        g_orig_dig[8];
static int            g_active_bit = 0;
static int            g_top_idx    = 0;

/* ══════════════════════════════════════════════════
 *  布局常量
 * ══════════════════════════════════════════════════ */
#define TOPBAR_H   30
#define HINTBAR_H  22
#define CONTENT_Y  TOPBAR_H
#define CONTENT_H  (SCR_H - TOPBAR_H - HINTBAR_H)
#define VISIBLE_ROWS 5
#define ROW_H      (CONTENT_H / VISIBLE_ROWS)

#define CELL_W     36
#define CELL_H     48
#define CELL_GAP    6

/* ══════════════════════════════════════════════════
 *  Widget handles
 * ══════════════════════════════════════════════════ */
static lv_obj_t *list_rows[PARAM_COUNT];
static lv_obj_t *list_name[PARAM_COUNT];
static lv_obj_t *list_val [PARAM_COUNT];
static lv_obj_t *list_mark[PARAM_COUNT];

static lv_obj_t *num_overlay    = NULL;
static lv_obj_t *num_lbl_name   = NULL;
static lv_obj_t *num_lbl_val    = NULL;
static lv_obj_t *num_bar        = NULL;
static lv_obj_t *num_lbl_range  = NULL;
static lv_obj_t *num_lbl_orig   = NULL;
static lv_obj_t *num_badge_bg   = NULL;
static lv_obj_t *num_badge_lbl  = NULL;

static lv_obj_t *dig_overlay  = NULL;
static lv_obj_t *dig_lbl_name = NULL;
static lv_obj_t *dig_cells[8];
static lv_obj_t *dig_digit[8];
static lv_obj_t *dig_arrow_u[8];
static lv_obj_t *dig_arrow_d[8];
static lv_obj_t *dig_lbl_hint = NULL;
static lv_obj_t *dig_badge_bg  = NULL;
static lv_obj_t *dig_badge_lbl = NULL;
static lv_obj_t *dig_confirm_bar = NULL;
static lv_obj_t *dig_confirm_lbl = NULL;

static lv_obj_t *hint_slots[4];
static lv_obj_t *topbadge_lbl  = NULL;

#define FN10 (&lv_font_montserrat_10)
#define FN12 (&lv_font_montserrat_12)
#define FN14 (&lv_font_montserrat_14)
#define FN20 (&lv_font_montserrat_20)
#define FN32 (&lv_font_montserrat_32)

static void align_hint_slot(int i, const char *txt, bool has_up, bool has_dn)
{
    if (!hint_slots[i]) return;

    if (!txt || txt[0] == '\0') {
        lv_obj_set_width(hint_slots[i], 1);
        return;
    }

    if (i == 3) {
        lv_obj_set_width(hint_slots[i], 90);
        lv_obj_set_style_text_align(hint_slots[i], LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(hint_slots[i], LV_ALIGN_LEFT_MID, 4, 0);
    } else if (i == 2) {
        lv_obj_set_width(hint_slots[i], 100);
        lv_obj_set_style_text_align(hint_slots[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(hint_slots[i], LV_ALIGN_RIGHT_MID, -4, 0);
    } else {
        lv_obj_set_width(hint_slots[i], has_up && has_dn ? 70 : 120);
        lv_obj_set_style_text_align(hint_slots[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(hint_slots[i], LV_ALIGN_CENTER, has_up && has_dn && i == 0 ? -36 :
                                                has_up && has_dn && i == 1 ?  36 : 0, 0);
    }
}

/* ══════════════════════════════════════════════════
 *  Helpers
 * ══════════════════════════════════════════════════ */
static void fmt_val(char *buf, size_t sz, const param_t *p, float v)
{
    if (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0) {
        if (v == 0.0f) snprintf(buf, sz, "%s", ui_get_text("中文"));
        else           snprintf(buf, sz, "English");
    } else {
        if (p->step < 1.0f) snprintf(buf, sz, "%.1f %s", v, p->unit);
        else                snprintf(buf, sz, "%.0f %s", v, p->unit);
    }
}

static void update_bar(lv_obj_t *bar, const param_t *p, float v)
{
    if (!bar) return;
    int range = (int)((p->val_max - p->val_min) * 10);
    if (range <= 0) range = 1;
    lv_bar_set_range(bar, 0, range);
    lv_bar_set_value(bar, (int)((v - p->val_min) * 10), LV_ANIM_ON);
}

static void set_topbadge(const char *txt, lv_color_t bg, lv_color_t col)
{
    if (!topbadge_lbl) return;
    lv_label_set_text(topbadge_lbl, txt);
    lv_obj_set_style_text_color(topbadge_lbl, col, 0);
    if (lv_obj_get_parent(topbadge_lbl))
        lv_obj_set_style_bg_color(lv_obj_get_parent(topbadge_lbl), bg, 0);
}

/* ══════════════════════════════════════════════════
 *  HINTS
 * ══════════════════════════════════════════════════ */
static void update_hints(void)
{
    param_t *p = &g_params[g_cursor];
    bool is_lang = (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0);

    static const char *tbl[6][4] = {
        { "▲▼ 选择", "",        "OK 编辑",    "ESC 返回"  },  /* BROWSE       */
        { "▲ +",    "▼ -",     "OK 确认",    "ESC 放弃"  },  /* NUM_EDIT     */
        { "",        "",        "OK 保存",    "ESC 继续改"},  /* NUM_CONFIRM  */
        { "▲▼ 换位", "",        "OK 锁定位",  "ESC 退出"  },  /* DIG_SELECT   */
        { "▲ +1",   "▼ -1",    "OK 下一位",  "ESC 返回"  },  /* DIG_EDIT     */
        { "",        "",        "OK 保存",    "ESC 继续改"},  /* DIG_CONFIRM  */
    };

    const char *h0 = tbl[g_state][0];
    const char *h1 = tbl[g_state][1];
    const char *h2 = tbl[g_state][2];
    const char *h3 = tbl[g_state][3];

    if (is_lang) {
        if (g_state == PS_NUM_EDIT) {
            h0 = "▲▼ 更改";
            h1 = "";
            h2 = "OK 确认";
            h3 = "ESC 放弃";
        } else if (g_state == PS_NUM_CONFIRM) {
            h0 = "";
            h1 = "";
            h2 = "OK 保存";
            h3 = "ESC 返回";
        }
    }

    for (int i = 0; i < 4; i++) {
        const char *txt = (i == 0) ? h0 : (i == 1) ? h1 : (i == 2) ? h2 : h3;
        if (hint_slots[i]) lv_label_set_text(hint_slots[i], ui_get_text(txt));
    }
    bool has_up = h0[0] != '\0';
    bool has_dn = h1[0] != '\0';
    for (int i = 0; i < 4; i++) {
        const char *txt = (i == 0) ? h0 : (i == 1) ? h1 : (i == 2) ? h2 : h3;
        if (hint_slots[i]) align_hint_slot(i, ui_get_text(txt), has_up, has_dn);
    }

    static const struct { const char *txt; uint32_t bg; uint32_t col; } badge_tbl[] = {
        { "浏览",   0x1E4A2A, 0x4ECB71 },
        { "编辑中", 0x3A2A10, 0xE8A730 },
        { "确认?",  0x3A1A1A, 0xE05A5A },
        { "选择位", 0x201040, 0xAA88FF },
        { "编辑位", 0x1A3010, 0x7ACC40 },
        { "确认?",  0x3A1A1A, 0xE05A5A },
    };
    set_topbadge(ui_get_text(badge_tbl[g_state].txt),
                 lv_color_hex(badge_tbl[g_state].bg),
                 lv_color_hex(badge_tbl[g_state].col));
}

/* ══════════════════════════════════════════════════
 *  LIST RENDER
 * ══════════════════════════════════════════════════ */
static void render_list(void)
{
    lv_obj_add_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dig_overlay, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < PARAM_COUNT; i++) {
        if (i >= g_top_idx && i < g_top_idx + VISIBLE_ROWS) {
            lv_obj_remove_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(list_rows[i], CONTENT_Y + (i - g_top_idx) * ROW_H);
        } else {
            lv_obj_add_flag(list_rows[i], LV_OBJ_FLAG_HIDDEN);
        }

        bool sel = (i == g_cursor);
        
        lv_color_t bg_col = C_BG_CARD;
        lv_color_t border_col = C_BORDER;
        lv_color_t name_col = C_TEXT_SEC;
        lv_color_t val_col = lv_color_hex(0x7ECFFF);
        
        if (sel) {
            if (g_state == PS_BROWSE) {
                bg_col = C_PW_ACTIVE;
                border_col = C_PW_BORDER;
                name_col = C_TEXT_PRI;
                val_col = C_TEXT_PRI;
            } else if (g_state == PS_NUM_EDIT) {
                bg_col = lv_color_hex(0x2A1F0F);
                border_col = C_WARN;
                name_col = C_TEXT_PRI;
                val_col = C_WARN;
            } else if (g_state == PS_NUM_CONFIRM) {
                bg_col = lv_color_hex(0x2F1313);
                border_col = C_ALARM;
                name_col = C_TEXT_PRI;
                val_col = C_ALARM;
            }
        }

        lv_obj_set_style_bg_color(list_rows[i], bg_col, 0);
        lv_obj_set_style_border_color(list_rows[i], border_col, LV_PART_MAIN);
        lv_obj_set_style_border_side(list_rows[i], LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(list_rows[i], 1, 0);
        
        lv_obj_set_style_text_color(list_name[i], name_col, 0);
        lv_label_set_text(list_name[i], ui_get_text(g_params[i].name));
        
        lv_label_set_text(list_mark[i], (sel && g_state == PS_BROWSE) ? LV_SYMBOL_RIGHT : " ");

        /* 值字符串 */
        param_t *p = &g_params[i];
        char buf[32];
        if (p->type == PARAM_DIGIT) {
            int off = 0;
            for (int d = 0; d < p->digit_count; d++)
                off += snprintf(buf+off, sizeof(buf)-off, d ? "-%d":"%d", p->digits[d]);
        } else {
            bool is_lang = (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0);
            float val_to_show = (sel && (g_state == PS_NUM_EDIT || g_state == PS_NUM_CONFIRM)) ? g_edit_val : p->val;
            
            char vb[24];
            fmt_val(vb, sizeof(vb), p, val_to_show);
            
            if (sel && is_lang && g_state == PS_NUM_EDIT) {
                snprintf(buf, sizeof(buf), "< %s >", vb);
            } else if (sel && is_lang && g_state == PS_NUM_CONFIRM) {
                snprintf(buf, sizeof(buf), "[ %s ] (%s)", vb, ui_get_text("确认?"));
            } else if (sel && g_state == PS_NUM_CONFIRM) {
                snprintf(buf, sizeof(buf), "%s (%s)", vb, ui_get_text("确认?"));
            } else {
                snprintf(buf, sizeof(buf), "%s", vb);
            }
        }
        lv_label_set_text(list_val[i], buf);
        lv_obj_set_style_text_color(list_val[i], val_col, 0);
    }
}

/* ══════════════════════════════════════════════════
 *  NUM OVERLAY RENDER
 * ══════════════════════════════════════════════════ */
static void render_num(void)
{
    lv_obj_remove_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dig_overlay,   LV_OBJ_FLAG_HIDDEN);

    param_t *p = &g_params[g_cursor];
    bool confirm = (g_state == PS_NUM_CONFIRM);
    lv_color_t vcol = confirm ? C_ALARM : C_WARN;

    lv_label_set_text(num_lbl_name, ui_get_text(p->name));
    lv_obj_set_style_text_color(num_lbl_val, vcol, 0);

    char buf[48];
    if (confirm) {
        char vb[24]; fmt_val(vb, sizeof(vb), p, g_edit_val);
        snprintf(buf, sizeof(buf), "%s  %s", ui_get_text("确认?"), vb);
    } else {
        fmt_val(buf, sizeof(buf), p, g_edit_val);
    }
    lv_label_set_text(num_lbl_val, buf);

    if (num_bar) {
        lv_obj_set_style_bg_color(num_bar, vcol, LV_PART_INDICATOR);
        update_bar(num_bar, p, g_edit_val);
    }

    bool is_lang = (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0);

    if (num_lbl_range) {
        if (is_lang) {
            lv_obj_add_flag(num_lbl_range, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(num_lbl_range, LV_OBJ_FLAG_HIDDEN);
            char rb[64];
            snprintf(rb, sizeof(rb), ui_get_text("范围 %.0f - %.0f  步进 %.1f"),
                     p->val_min, p->val_max, p->step);
            lv_label_set_text(num_lbl_range, rb);
            lv_obj_set_style_text_color(num_lbl_range,
                confirm ? C_ALARM : lv_color_hex(0x88AACC), 0);
        }
    }

    if (num_lbl_orig) {
        if (is_lang) {
            lv_obj_add_flag(num_lbl_orig, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(num_lbl_orig, LV_OBJ_FLAG_HIDDEN);
            char ob[64];
            char vb[24];
            fmt_val(vb, sizeof(vb), p, g_orig_val);
            snprintf(ob, sizeof(ob), "%s %s", ui_get_text("原值:"), vb);
            lv_label_set_text(num_lbl_orig, ob);
            lv_obj_set_style_text_color(num_lbl_orig,
                confirm ? C_ALARM : lv_color_hex(0x88AACC), 0);
        }
    }

    /* Badge */
    if (num_badge_lbl) {
        const char *bt = confirm ? ui_get_text("确认保存?") : ui_get_text("数值编辑");
        lv_color_t  bc = confirm ? C_ALARM : C_WARN;
        lv_color_t  bb = confirm ? lv_color_hex(0x3A1A1A) : lv_color_hex(0x3A2A10);
        lv_label_set_text(num_badge_lbl, bt);
        lv_obj_set_style_text_color(num_badge_lbl, bc, 0);
        if (num_badge_bg) lv_obj_set_style_bg_color(num_badge_bg, bb, 0);
    }
}

/* ══════════════════════════════════════════════════
 *  DIGIT OVERLAY RENDER
 * ══════════════════════════════════════════════════ */
static void render_dig(void)
{
    lv_obj_remove_flag(dig_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(num_overlay,   LV_OBJ_FLAG_HIDDEN);

    param_t *p = &g_params[g_cursor];
    bool confirm = (g_state == PS_DIG_CONFIRM);
    bool sel_mode= (g_state == PS_DIG_SELECT);
    bool edit_mode=(g_state == PS_DIG_EDIT);

    char title[64];
    snprintf(title, sizeof(title), ui_get_text("编辑: %s"), ui_get_text(p->name));
    lv_label_set_text(dig_lbl_name, title);

    for (int i = 0; i < (int)p->digit_count; i++) {
        bool active = (i == g_active_bit) && !confirm;

        lv_color_t border, bg, tcol;
        if (confirm) {
            border = lv_color_hex(0x8A5A20);
            bg     = lv_color_hex(0x2A1A10);
            tcol   = C_WARN;
        } else if (active && edit_mode) {
            border = C_OK;
            bg     = lv_color_hex(0x1A3010);
            tcol   = C_OK;
        } else if (active && sel_mode) {
            border = lv_color_hex(0xAA88FF);
            bg     = lv_color_hex(0x10203A);
            tcol   = lv_color_hex(0xAA88FF);
        } else {
            border = lv_color_hex(0x2A4A6A);
            bg     = lv_color_hex(0x1A2A3A);
            tcol   = C_ACCENT;
        }

        if (dig_cells[i]) {
            lv_obj_set_style_border_color(dig_cells[i], border, 0);
            lv_obj_set_style_bg_color(dig_cells[i], bg, 0);
        }
        if (dig_digit[i]) {
            char db[4]; snprintf(db, sizeof(db), "%d", g_edit_dig[i]);
            lv_label_set_text(dig_digit[i], db);
            lv_obj_set_style_text_color(dig_digit[i], tcol, 0);
        }

        lv_opa_t opa  = (active && edit_mode && !confirm) ? LV_OPA_COVER : LV_OPA_TRANSP;
        lv_color_t ac = C_OK;
        if (dig_arrow_u[i]) {
            lv_obj_set_style_opa(dig_arrow_u[i], opa, 0);
            lv_obj_set_style_text_color(dig_arrow_u[i], ac, 0);
        }
        if (dig_arrow_d[i]) {
            lv_obj_set_style_opa(dig_arrow_d[i], opa, 0);
            lv_obj_set_style_text_color(dig_arrow_d[i], ac, 0);
        }
    }

    if (dig_lbl_hint) {
        if (confirm) {
            char hb[32];
            int off = snprintf(hb, sizeof(hb), "%s", ui_get_text("值: "));
            for (int i = 0; i < (int)p->digit_count && off < (int)sizeof(hb); i++)
                off += snprintf(hb + off, sizeof(hb) - off, "%d", g_edit_dig[i]);
            lv_label_set_text(dig_lbl_hint, hb);
        } else {
            const char *ht = sel_mode ? ui_get_text("UP/DOWN 选择位   OK 进入编辑该位")
                                      : ui_get_text("UP/DOWN 修改当前位数字   OK 移到下一位");
            lv_label_set_text(dig_lbl_hint, ht);
        }
    }

    if (dig_confirm_bar) {
        if (confirm) lv_obj_remove_flag(dig_confirm_bar, LV_OBJ_FLAG_HIDDEN);
        else         lv_obj_add_flag(dig_confirm_bar, LV_OBJ_FLAG_HIDDEN);
    }

    /* Badge */
    if (dig_badge_lbl) {
        const char *bt; lv_color_t bc, bb;
        if (confirm)       { bt=ui_get_text("确认?");     bc=C_WARN;              bb=lv_color_hex(0x3A2A10); }
        else if (sel_mode) { bt=ui_get_text("选择位");    bc=lv_color_hex(0xAA88FF); bb=lv_color_hex(0x201040); }
        else               { bt=ui_get_text("编辑位");    bc=lv_color_hex(0x7ACC40); bb=lv_color_hex(0x1A3010); }
        lv_label_set_text(dig_badge_lbl, bt);
        lv_obj_set_style_text_color(dig_badge_lbl, bc, 0);
        if (dig_badge_bg) lv_obj_set_style_bg_color(dig_badge_bg, bb, 0);
    }
}

/* ── Master render ── */
static void param_render(void)
{
    param_t *p = &g_params[g_cursor];
    bool is_lang = (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0);

    if (g_state == PS_BROWSE || (is_lang && (g_state == PS_NUM_EDIT || g_state == PS_NUM_CONFIRM))) {
        render_list();
    } else {
        switch (g_state) {
        case PS_NUM_EDIT:
        case PS_NUM_CONFIRM:
            render_num(); break;
        default:
            render_dig(); break;
        }
    }
    update_hints();
}

/* ══════════════════════════════════════════════════
 *  KEY HANDLER
 * ══════════════════════════════════════════════════ */
void ui_param_key(ui_key_t key)
{
    if (key == KEY_LEFT) key = KEY_ESC;
    else if (key == KEY_RIGHT) key = KEY_OK;

    param_t *p = &g_params[g_cursor];

    switch (g_state) {

    case PS_BROWSE:
        if      (key == KEY_UP)   {
            g_cursor = (g_cursor > 0) ? g_cursor-1 : 0;
            if (g_cursor < g_top_idx) g_top_idx = g_cursor;
        }
        else if (key == KEY_DOWN) {
            g_cursor = (g_cursor < PARAM_COUNT-1) ? g_cursor+1 : PARAM_COUNT-1;
            if (g_cursor >= g_top_idx + VISIBLE_ROWS) g_top_idx = g_cursor - VISIBLE_ROWS + 1;
        }
        else if (key == KEY_OK) {
            if (p->type == PARAM_NUM) {
                g_orig_val = p->val; g_edit_val = p->val; g_state = PS_NUM_EDIT;
            } else {
                memcpy(g_orig_dig, p->digits, p->digit_count);
                memcpy(g_edit_dig, p->digits, p->digit_count);
                g_active_bit = 0; g_state = PS_DIG_SELECT;
            }
        } else if (key == KEY_ESC) {
            g_cursor = 0;
            g_top_idx = 0;
            ui_goto(PAGE_MENU);
            return;
        }
        break;

    case PS_NUM_EDIT:
        if (key == KEY_UP) {
            g_edit_val += p->step;
            if (g_edit_val > p->val_max) g_edit_val = p->val_max;
        } else if (key == KEY_DOWN) {
            g_edit_val -= p->step;
            if (g_edit_val < p->val_min) g_edit_val = p->val_min;
        } else if (key == KEY_OK)  g_state = PS_NUM_CONFIRM;
        else if (key == KEY_ESC) { g_edit_val = g_orig_val; g_state = PS_BROWSE; }
        break;

    case PS_NUM_CONFIRM:
        if (key == KEY_OK) {
            p->val = g_edit_val;
            g_state = PS_BROWSE;
            if (strcmp(p->name, "语言选择") == 0 || strcmp(p->name, "Language") == 0) {
                g_lang = (ui_lang_t)p->val;
                ui_refresh_current_page();
                return;
            }
        }
        else if (key == KEY_ESC) g_state = PS_NUM_EDIT;
        break;

    case PS_DIG_SELECT:
        if      (key == KEY_UP)   g_active_bit = (g_active_bit > 0) ? g_active_bit-1 : 0;
        else if (key == KEY_DOWN) g_active_bit = (g_active_bit < p->digit_count-1) ? g_active_bit+1 : p->digit_count-1;
        else if (key == KEY_OK)   g_state = PS_DIG_EDIT;
        else if (key == KEY_ESC) { memcpy(p->digits, g_orig_dig, p->digit_count); g_state = PS_BROWSE; }
        break;

    case PS_DIG_EDIT:
        if      (key == KEY_UP)   g_edit_dig[g_active_bit] = (g_edit_dig[g_active_bit]+1)%10;
        else if (key == KEY_DOWN) g_edit_dig[g_active_bit] = (g_edit_dig[g_active_bit]+9)%10;
        else if (key == KEY_OK) {
            if (g_active_bit < p->digit_count-1) { g_active_bit++; }
            else g_state = PS_DIG_CONFIRM;
        } else if (key == KEY_ESC) g_state = PS_DIG_SELECT;
        break;

    case PS_DIG_CONFIRM:
        if (key == KEY_OK) { memcpy(p->digits, g_edit_dig, p->digit_count); g_state = PS_BROWSE; }
        else if (key == KEY_ESC) { g_active_bit = p->digit_count-1; g_state = PS_DIG_EDIT; }
        break;
    }
    param_render();
}

/* ══════════════════════════════════════════════════
 *  BUILD PHASE
 * ══════════════════════════════════════════════════ */
static lv_obj_t *make_badge(lv_obj_t *parent, int align_x, const char *txt,
                              lv_color_t bg, lv_color_t col,
                              lv_obj_t **out_bg, lv_obj_t **out_lbl)
{
    lv_obj_t *bb = lv_obj_create(parent);
    lv_obj_set_size(bb, LV_SIZE_CONTENT, 16);
    lv_obj_align(bb, LV_ALIGN_TOP_RIGHT, align_x, 5);
    lv_obj_set_style_bg_color(bb, bg, 0);
    lv_obj_set_style_bg_opa(bb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bb, 0, 0);
    lv_obj_set_style_radius(bb, 8, 0);
    lv_obj_set_style_pad_hor(bb, 5, 0);
    lv_obj_set_style_pad_ver(bb, 0, 0);
    lv_obj_clear_flag(bb, LV_OBJ_FLAG_SCROLLABLE);
    if (out_bg) *out_bg = bb;

    lv_obj_t *bl = lv_label_create(bb);
    lv_label_set_text(bl, txt);
    lv_obj_set_style_text_color(bl, col, 0);
    lv_obj_set_style_text_font(bl, UI_FONT_CJK_14, 0);
    lv_obj_center(bl);
    if (out_lbl) *out_lbl = bl;
    return bb;
}

static void build_chrome(lv_obj_t *parent)
{
    /* 顶栏 */
    lv_obj_t *tb = lv_obj_create(parent);
    lv_obj_set_size(tb, SCR_W, TOPBAR_H);
    lv_obj_align(tb, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(tb, lv_color_hex(0x0A1118), 0);
    lv_obj_set_style_bg_opa(tb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tb, 1, 0);
    lv_obj_set_style_border_side(tb, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(tb, lv_color_hex(0x1E3A52), 0);
    lv_obj_set_style_pad_hor(tb, 8, 0);
    lv_obj_set_style_pad_ver(tb, 0, 0);
    lv_obj_clear_flag(tb, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(tb);
    char tb_title_buf[64];
    snprintf(tb_title_buf, sizeof(tb_title_buf), "%s %s", LV_SYMBOL_SETTINGS, ui_get_text("参数设置"));
    lv_label_set_text(tl, tb_title_buf);
    lv_obj_set_style_text_color(tl, C_ACCENT, 0);
    lv_obj_set_style_text_font(tl, UI_FONT_CJK_14, 0);
    lv_obj_align(tl, LV_ALIGN_LEFT_MID, 0, 0);

    /* 顶栏徽标 */
    lv_obj_t *tbadge = lv_obj_create(tb);
    lv_obj_set_size(tbadge, LV_SIZE_CONTENT, 16);
    lv_obj_align(tbadge, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(tbadge, lv_color_hex(0x1E4A2A), 0);
    lv_obj_set_style_bg_opa(tbadge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tbadge, 0, 0);
    lv_obj_set_style_radius(tbadge, 8, 0);
    lv_obj_set_style_pad_hor(tbadge, 5, 0);
    lv_obj_set_style_pad_ver(tbadge, 0, 0);
    lv_obj_clear_flag(tbadge, LV_OBJ_FLAG_SCROLLABLE);

    topbadge_lbl = lv_label_create(tbadge);
    lv_label_set_text(topbadge_lbl, ui_get_text("浏览"));
    lv_obj_set_style_text_color(topbadge_lbl, C_OK, 0);
    lv_obj_set_style_text_font(topbadge_lbl, UI_FONT_CJK_14, 0);
    lv_obj_center(topbadge_lbl);

    /* 提示栏 */
    lv_obj_t *hb = lv_obj_create(parent);
    lv_obj_set_size(hb, SCR_W, HINTBAR_H);
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
        lv_label_set_long_mode(hint_slots[i], LV_LABEL_LONG_CLIP);
        align_hint_slot(i, "", false, false);
    }
}

static void build_list(lv_obj_t *parent)
{
    for (int i = 0; i < PARAM_COUNT; i++) {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_size(row, SCR_W, ROW_H);
        lv_obj_set_pos(row, 0, CONTENT_Y + i * ROW_H);
        lv_obj_set_style_bg_color(row, C_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, C_BORDER, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        list_rows[i] = row;

        /* 光标 */
        list_mark[i] = lv_label_create(row);
        lv_label_set_text(list_mark[i], " ");
        lv_obj_set_style_text_color(list_mark[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(list_mark[i], &lv_font_montserrat_14, 0);
        lv_obj_align(list_mark[i], LV_ALIGN_LEFT_MID, 10, 0);

        /* 参数名 */
        list_name[i] = lv_label_create(row);
        lv_label_set_text(list_name[i], g_params[i].name);
        lv_obj_set_style_text_color(list_name[i], C_TEXT_SEC, 0);
        lv_obj_set_style_text_font(list_name[i], UI_FONT_CJK_24, 0);
        lv_obj_align(list_name[i], LV_ALIGN_LEFT_MID, 30, 0);

        /* 当前值 */
        list_val[i] = lv_label_create(row);
        lv_obj_set_style_text_color(list_val[i], lv_color_hex(0x7ECFFF), 0);
        lv_obj_set_style_text_font(list_val[i], UI_FONT_CJK_24, 0);
        lv_obj_align(list_val[i], LV_ALIGN_RIGHT_MID, -6, 0);
    }
}

static void build_num_overlay(lv_obj_t *parent)
{
    num_overlay = lv_obj_create(parent);
    lv_obj_set_size(num_overlay, SCR_W, CONTENT_H);
    lv_obj_set_pos(num_overlay, 0, CONTENT_Y);
    lv_obj_set_style_bg_color(num_overlay, lv_color_hex(0x0D1A28), 0);
    lv_obj_set_style_bg_opa(num_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(num_overlay, 0, 0);
    lv_obj_set_style_pad_all(num_overlay, 0, 0);
    lv_obj_clear_flag(num_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(num_overlay, LV_OBJ_FLAG_HIDDEN);

    /* 参数名 */
    num_lbl_name = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_name, "");
    lv_obj_set_style_text_color(num_lbl_name, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(num_lbl_name, UI_FONT_CJK_24, 0);
    lv_obj_align(num_lbl_name, LV_ALIGN_TOP_MID, 0, 10);

    /* 上箭头装饰 */
    lv_obj_t *ua = lv_label_create(num_overlay);
    lv_label_set_text(ua, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(ua, lv_color_hex(0x2A5A8A), 0);
    lv_obj_set_style_text_font(ua, FN14, 0);
    lv_obj_align(ua, LV_ALIGN_TOP_MID, 0, 30);

    /* 大值标签 */
    num_lbl_val = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_val, "---");
    lv_obj_set_style_text_color(num_lbl_val, C_WARN, 0);
    lv_obj_set_style_text_font(num_lbl_val, UI_FONT_CJK_24, 0);
    lv_obj_align(num_lbl_val, LV_ALIGN_TOP_MID, 0, 48);

    /* 下箭头装饰 */
    lv_obj_t *da = lv_label_create(num_overlay);
    lv_label_set_text(da, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(da, lv_color_hex(0x2A5A8A), 0);
    lv_obj_set_style_text_font(da, FN14, 0);
    lv_obj_align(da, LV_ALIGN_TOP_MID, 0, 92);

    /* 进度条 */
    num_bar = lv_bar_create(num_overlay);
    lv_obj_set_size(num_bar, SCR_W - 40, 8);
    lv_obj_align(num_bar, LV_ALIGN_TOP_MID, 0, 112);
    lv_obj_set_style_bg_color(num_bar, C_BAR_BG, 0);
    lv_obj_set_style_bg_color(num_bar, C_WARN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(num_bar, 4, 0);
    lv_obj_set_style_radius(num_bar, 4, LV_PART_INDICATOR);

    /* 范围说明 */
    num_lbl_range = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_range, "");
    lv_obj_set_style_text_color(num_lbl_range, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(num_lbl_range, UI_FONT_CJK_14, 0);
    lv_obj_align(num_lbl_range, LV_ALIGN_TOP_MID, 0, 128);

    /* 原值标签 */
    num_lbl_orig = lv_label_create(num_overlay);
    lv_label_set_text(num_lbl_orig, ui_get_text("原值:"));
    lv_obj_set_style_text_color(num_lbl_orig, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(num_lbl_orig, UI_FONT_CJK_14, 0);
    lv_obj_align(num_lbl_orig, LV_ALIGN_TOP_LEFT, 20, 148);

    /* 状态徽标 */
    make_badge(num_overlay, -4, ui_get_text("数值编辑"),
               lv_color_hex(0x3A2A10), C_WARN,
               &num_badge_bg, &num_badge_lbl);
}

static void build_dig_overlay(lv_obj_t *parent)
{
    dig_overlay = lv_obj_create(parent);
    lv_obj_set_size(dig_overlay, SCR_W, CONTENT_H);
    lv_obj_set_pos(dig_overlay, 0, CONTENT_Y);
    lv_obj_set_style_bg_color(dig_overlay, lv_color_hex(0x0D1A28), 0);
    lv_obj_set_style_bg_opa(dig_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dig_overlay, 0, 0);
    lv_obj_set_style_pad_all(dig_overlay, 0, 0);
    lv_obj_clear_flag(dig_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dig_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *title_bar = lv_obj_create(dig_overlay);
    lv_obj_set_size(title_bar, SCR_W, 26);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_bar, 1, 0);
    lv_obj_set_style_border_side(title_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(title_bar, lv_color_hex(0x1E3A5A), 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 参数名 */
    dig_lbl_name = lv_label_create(title_bar);
    lv_label_set_text(dig_lbl_name, "");
    lv_obj_set_style_text_color(dig_lbl_name, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(dig_lbl_name, UI_FONT_CJK_24, 0);
    lv_obj_align(dig_lbl_name, LV_ALIGN_LEFT_MID, 10, 0);

    /* 5个位格 */
    int total_w = 5 * CELL_W + 4 * CELL_GAP;
    int start_x = (SCR_W - total_w) / 2;

    for (int i = 0; i < 5; i++) {
        int cx = start_x + i * (CELL_W + CELL_GAP);
        int cy = 70;

        /* 上箭头 */
        dig_arrow_u[i] = lv_label_create(dig_overlay);
        lv_label_set_text(dig_arrow_u[i], LV_SYMBOL_UP);
        lv_obj_set_style_text_color(dig_arrow_u[i], C_OK, 0);
        lv_obj_set_style_text_font(dig_arrow_u[i], FN10, 0);
        lv_obj_set_pos(dig_arrow_u[i], cx + CELL_W/2 - 5, cy - 12);
        lv_obj_set_style_opa(dig_arrow_u[i], LV_OPA_TRANSP, 0);

        /* 格子 */
        dig_cells[i] = lv_obj_create(dig_overlay);
        lv_obj_set_size(dig_cells[i], CELL_W, CELL_H);
        lv_obj_set_pos(dig_cells[i], cx, cy);
        lv_obj_set_style_bg_color(dig_cells[i], lv_color_hex(0x1A2A3A), 0);
        lv_obj_set_style_bg_opa(dig_cells[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(dig_cells[i], lv_color_hex(0x2A4A6A), 0);
        lv_obj_set_style_border_width(dig_cells[i], 2, 0);
        lv_obj_set_style_radius(dig_cells[i], 4, 0);
        lv_obj_set_style_pad_all(dig_cells[i], 0, 0);
        lv_obj_clear_flag(dig_cells[i], LV_OBJ_FLAG_SCROLLABLE);

        /* 数字 */
        dig_digit[i] = lv_label_create(dig_cells[i]);
        lv_label_set_text(dig_digit[i], "0");
        lv_obj_set_style_text_color(dig_digit[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(dig_digit[i], FN20, 0);
        lv_obj_center(dig_digit[i]);

        /* 位序号 */
        lv_obj_t *pl = lv_label_create(dig_overlay);
        char pb[16]; snprintf(pb, sizeof(pb), ui_get_text("位%d"), i+1);
        lv_label_set_text(pl, pb);
        lv_obj_set_style_text_color(pl, lv_color_hex(0x88AACC), 0);
        lv_obj_set_style_text_font(pl, UI_FONT_CJK_14, 0);
        lv_obj_align_to(pl, dig_cells[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

        /* 下箭头 */
        dig_arrow_d[i] = lv_label_create(dig_overlay);
        lv_label_set_text(dig_arrow_d[i], LV_SYMBOL_DOWN);
        lv_obj_set_style_text_color(dig_arrow_d[i], C_OK, 0);
        lv_obj_set_style_text_font(dig_arrow_d[i], FN10, 0);
        lv_obj_set_pos(dig_arrow_d[i], cx + CELL_W/2 - 5, cy + CELL_H + 2);
        lv_obj_set_style_opa(dig_arrow_d[i], LV_OPA_TRANSP, 0);
    }

    /* 操作提示 */
    dig_lbl_hint = lv_label_create(dig_overlay);
    lv_label_set_text(dig_lbl_hint, "");
    lv_obj_set_style_text_color(dig_lbl_hint, lv_color_hex(0x88AACC), 0);
    lv_obj_set_style_text_font(dig_lbl_hint, UI_FONT_CJK_14, 0);
    lv_obj_align(dig_lbl_hint, LV_ALIGN_TOP_MID, 0, 152);

    dig_confirm_bar = lv_obj_create(dig_overlay);
    lv_obj_set_size(dig_confirm_bar, SCR_W, 24);
    lv_obj_align(dig_confirm_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(dig_confirm_bar, lv_color_hex(0x2A1A10), 0);
    lv_obj_set_style_bg_opa(dig_confirm_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dig_confirm_bar, 1, 0);
    lv_obj_set_style_border_side(dig_confirm_bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(dig_confirm_bar, lv_color_hex(0x6A3A1A), 0);
    lv_obj_set_style_radius(dig_confirm_bar, 0, 0);
    lv_obj_set_style_pad_all(dig_confirm_bar, 0, 0);
    lv_obj_clear_flag(dig_confirm_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dig_confirm_bar, LV_OBJ_FLAG_HIDDEN);

    dig_confirm_lbl = lv_label_create(dig_confirm_bar);
    lv_label_set_text(dig_confirm_lbl, ui_get_text("OK 保存   ESC 取消"));
    lv_obj_set_style_text_color(dig_confirm_lbl, C_WARN, 0);
    lv_obj_set_style_text_font(dig_confirm_lbl, UI_FONT_CJK_14, 0);
    lv_obj_center(dig_confirm_lbl);

}

/* ══════════════════════════════════════════════════
 *  PUBLIC: 创建参数设置页
 * ══════════════════════════════════════════════════ */
void ui_page_param_create(lv_obj_t *parent)
{
    g_state      = PS_BROWSE;
    g_active_bit = 0;

    memset(list_rows, 0, sizeof(list_rows));
    memset(list_name, 0, sizeof(list_name));
    memset(list_val, 0, sizeof(list_val));
    memset(list_mark, 0, sizeof(list_mark));
    num_overlay = NULL;
    num_lbl_name = NULL;
    num_lbl_val = NULL;
    num_bar = NULL;
    num_lbl_range = NULL;
    num_lbl_orig = NULL;
    num_badge_bg = NULL;
    num_badge_lbl = NULL;
    dig_overlay = NULL;
    dig_lbl_name = NULL;
    memset(dig_cells, 0, sizeof(dig_cells));
    memset(dig_digit, 0, sizeof(dig_digit));
    memset(dig_arrow_u, 0, sizeof(dig_arrow_u));
    memset(dig_arrow_d, 0, sizeof(dig_arrow_d));
    dig_lbl_hint = NULL;
    dig_badge_bg = NULL;
    dig_badge_lbl = NULL;
    dig_confirm_bar = NULL;
    dig_confirm_lbl = NULL;
    memset(hint_slots, 0, sizeof(hint_slots));
    topbadge_lbl = NULL;

    build_chrome(parent);
    build_list(parent);
    build_num_overlay(parent);
    build_dig_overlay(parent);

    param_render();
}
