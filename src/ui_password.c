/**
 * ui_password.c
 * 5-digit password entry screen.
 *
 * Correct password: 0 8 5 2 1  (change PW_CORRECT below)
 * - UP / DOWN  : change active digit (0‥9, wraps)
 * - OK         : advance to next digit; on last digit → validate
 * - ESC        : return to home screen
 *
 * Security:
 *   3 wrong attempts → 30-second lockout with countdown.
 */

#include "ui.h"
#include <string.h>
#include <stdio.h>

/* ── Password config ─────────────────────────────── */
#define PW_DIGITS     5
#define PW_MAX_FAIL   3
#define PW_LOCK_SEC   5

static const uint8_t PW_CORRECT[PW_DIGITS] = { 0, 0, 0, 0, 1 };

/* ── State ───────────────────────────────────────── */
static uint8_t  pw_digits[PW_DIGITS];
static int      pw_pos        = 0;
static int      pw_fail_count = 0;
static bool     pw_locked     = false;
static int      pw_lock_secs  = 0;

typedef enum { PW_MODE_INPUT, PW_MODE_LOCKED } pw_mode_t;
static pw_mode_t pw_mode = PW_MODE_INPUT;

/* ── Widget handles ──────────────────────────────── */
#define DIGIT_W  40
#define DIGIT_H  52
#define DIGIT_GAP 8

static lv_obj_t *digit_cells[PW_DIGITS];  /* outer box */
static lv_obj_t *digit_lbls [PW_DIGITS];  /* digit text */
static lv_obj_t *arrow_up   [PW_DIGITS];
static lv_obj_t *arrow_dn   [PW_DIGITS];
static lv_obj_t *lbl_status;              /* feedback line */
static lv_obj_t *lbl_countdown;           /* lockout countdown */
static lv_obj_t *lock_icon;
static lv_obj_t *digit_row;               /* parent for shake anim */

static lv_timer_t *lock_timer_obj = NULL;
static lv_timer_t *success_timer_obj = NULL;
static int digit_row_base_x = 0;

/* ── Font shortcuts ──────────────────────────────── */
#define FN10 (&lv_font_montserrat_10)
#define FN12 (&lv_font_montserrat_12)
#define FN24 (&lv_font_montserrat_24)
#define FN28 (&lv_font_montserrat_28)

/* ─────────────────────────────────────────────────
 *  Redraw a single digit cell to reflect current state
 * ───────────────────────────────────────────────── */
static void pw_redraw_cell(int i)
{
    bool is_active = (i == pw_pos) && !pw_locked;
    bool is_done   = (i < pw_pos);

    lv_color_t border_col = is_active ? C_PW_BORDER
                          : is_done   ? C_PW_FILLED
                          :             C_PW_EMPTY;
    lv_color_t bg_col     = is_active ? C_PW_ACTIVE
                          :             lv_color_hex(0x0E1820);
    lv_color_t text_col   = is_active ? lv_color_hex(0xFFFFFF)
                          : is_done   ? C_PW_BORDER
                          :             lv_color_hex(0x2A4A6A);

    lv_obj_set_style_border_color(digit_cells[i], border_col, 0);
    lv_obj_set_style_bg_color(digit_cells[i], bg_col, 0);

    /* Filled slots show "●" (hidden), active shows digit, empty shows "·" */
    if (is_done) {
        lv_label_set_text(digit_lbls[i], LV_SYMBOL_BULLET);
    } else if (is_active) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", pw_digits[i]);
        lv_label_set_text(digit_lbls[i], buf);
    } else {
        lv_label_set_text(digit_lbls[i], "·");
    }
    lv_obj_set_style_text_color(digit_lbls[i], text_col, 0);

    /* Show arrows only on active cell */
    lv_obj_set_style_opa(arrow_up[i], is_active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(arrow_dn[i], is_active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

static void pw_redraw_all(void)
{
    for (int i = 0; i < PW_DIGITS; i++)
        pw_redraw_cell(i);
}

/* ─────────────────────────────────────────────────
 *  Status label helpers
 * ───────────────────────────────────────────────── */
static void pw_set_status(const char *txt, lv_color_t col)
{
    if (!lbl_status) return;
    lv_label_set_text(lbl_status, txt);
    lv_obj_set_style_text_color(lbl_status, col, 0);
}

/* ─────────────────────────────────────────────────
 *  Shake animation on wrong password
 * ───────────────────────────────────────────────── */
static void pw_shake_anim_cb(void *var, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)var, digit_row_base_x + v);
}

static void pw_shake_completed_cb(lv_anim_t *a)
{
    lv_obj_set_x((lv_obj_t *)a->var, digit_row_base_x);
}

static void pw_shake(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, digit_row);
    lv_anim_set_exec_cb(&a, pw_shake_anim_cb);
    lv_anim_set_values(&a, 0, 6);
    lv_anim_set_time(&a, 60);
    lv_anim_set_playback_time(&a, 60);
    lv_anim_set_repeat_count(&a, 3);
    lv_anim_set_completed_cb(&a, pw_shake_completed_cb);
    lv_anim_start(&a);
}

static void pw_success_timer_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    success_timer_obj = NULL;
    ui_goto(PAGE_MENU);
}

/* ─────────────────────────────────────────────────
 *  Lockout timer callback (fires every 1 second)
 * ───────────────────────────────────────────────── */
static void pw_lock_timer_cb(lv_timer_t *t)
{
    (void)t;
    pw_lock_secs--;
    if (pw_lock_secs <= 0) {
        /* Unlock */
        pw_locked = false;
        pw_fail_count = 0;
        pw_mode = PW_MODE_INPUT;
        memset(pw_digits, 0, sizeof(pw_digits));
        pw_pos = 0;

        lv_obj_add_flag(lock_icon,       LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lbl_countdown,   LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(digit_row,    LV_OBJ_FLAG_HIDDEN);
        pw_set_status("请输入密码", C_TEXT_HINT);

        lv_timer_del(lock_timer_obj);
        lock_timer_obj = NULL;
        pw_redraw_all();
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d s", pw_lock_secs);
        lv_label_set_text(lbl_countdown, buf);
    }
}

/* ─────────────────────────────────────────────────
 *  Trigger lockout
 * ───────────────────────────────────────────────── */
static void pw_start_lock(void)
{
    pw_locked   = true;
    pw_mode     = PW_MODE_LOCKED;
    pw_lock_secs = PW_LOCK_SEC;

    /* Show lock icon + countdown, hide digit row */
    lv_obj_remove_flag(lock_icon,     LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lbl_countdown, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(digit_row,        LV_OBJ_FLAG_HIDDEN);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d s", pw_lock_secs);
    lv_label_set_text(lbl_countdown, buf);
    pw_set_status("连续错误 3 次，请等待", C_ALARM);

    lock_timer_obj = lv_timer_create(pw_lock_timer_cb, 1000, NULL);
}

/* ─────────────────────────────────────────────────
 *  Validate password
 * ───────────────────────────────────────────────── */
static void pw_validate(void)
{
    bool ok = (memcmp(pw_digits, PW_CORRECT, PW_DIGITS) == 0);
    if (ok) {
        pw_fail_count = 0;
        pw_set_status("验证成功!", C_OK);
        /* Short delay then switch to menu */
        if (success_timer_obj) lv_timer_del(success_timer_obj);
        success_timer_obj = lv_timer_create(pw_success_timer_cb, 500, NULL);
        lv_timer_set_repeat_count(success_timer_obj, 1);
    } else {
        pw_fail_count++;
        pw_shake();
        /* Clear digits, back to pos 0 */
        memset(pw_digits, 0, sizeof(pw_digits));
        pw_pos = 0;

        if (pw_fail_count >= PW_MAX_FAIL) {
            pw_start_lock();
        } else {
            char buf[40];
            snprintf(buf, sizeof(buf), "密码错误，还剩 %d 次", PW_MAX_FAIL - pw_fail_count);
            pw_set_status(buf, C_WARN);
            pw_redraw_all();
        }
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: key handler (called from dispatcher)
 * ───────────────────────────────────────────────── */
void ui_pw_key(ui_key_t key)
{
    if (pw_locked) return;  /* ignore input during lockout */

    switch (key) {
    case KEY_UP:
        pw_digits[pw_pos] = (pw_digits[pw_pos] + 1) % 10;
        pw_redraw_cell(pw_pos);
        break;

    case KEY_DOWN:
        pw_digits[pw_pos] = (pw_digits[pw_pos] + 9) % 10;
        pw_redraw_cell(pw_pos);
        break;

    case KEY_OK:
        if (pw_pos < PW_DIGITS - 1) {
            pw_pos++;
            pw_redraw_all();
        } else {
            pw_validate();
        }
        break;

    case KEY_ESC:
        /* Reset and return home */
        memset(pw_digits, 0, sizeof(pw_digits));
        pw_pos = 0;
        pw_fail_count = 0;
        pw_redraw_all();
        pw_set_status("请输入密码", C_TEXT_HINT);
        ui_goto(PAGE_HOME);
        break;
    }
}

/* ─────────────────────────────────────────────────
 *  PUBLIC: create password screen
 * ───────────────────────────────────────────────── */
void ui_page_password_create(lv_obj_t *parent)
{
    memset(pw_digits, 0, sizeof(pw_digits));
    pw_pos        = 0;
    pw_fail_count = 0;
    pw_locked     = false;
    pw_mode       = PW_MODE_INPUT;

    /* Top bar */
    ui_topbar_create(parent, "输入密码", "●●●●●",
                     lv_color_hex(0x1A1A3A), lv_color_hex(0x7A7ACC));

    /* ── Prompt label ─────────────────────────────── */
    lv_obj_t *prompt = lv_label_create(parent);
    lv_label_set_text(prompt, "系统设置  访问验证");
    lv_obj_set_style_text_color(prompt, C_TEXT_SEC, 0);
    lv_obj_set_style_text_font(prompt, UI_FONT_CJK_14, 0);
    lv_obj_align(prompt, LV_ALIGN_TOP_MID, 0, 46);

    /* ── Digit row container (used for shake animation) ── */
    int row_total_w = PW_DIGITS * DIGIT_W + (PW_DIGITS - 1) * DIGIT_GAP;
    int row_x = (SCR_W - row_total_w) / 2;
    digit_row_base_x = row_x;

    digit_row = lv_obj_create(parent);
    lv_obj_set_size(digit_row, row_total_w, DIGIT_H + 24);  /* +24 for arrows */
    lv_obj_align(digit_row, LV_ALIGN_TOP_LEFT, row_x, 72);
    lv_obj_set_style_bg_opa(digit_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(digit_row, 0, 0);
    lv_obj_set_style_pad_all(digit_row, 0, 0);
    lv_obj_clear_flag(digit_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < PW_DIGITS; i++) {
        int cx = i * (DIGIT_W + DIGIT_GAP);

        /* Up arrow */
        arrow_up[i] = lv_label_create(digit_row);
        lv_label_set_text(arrow_up[i], LV_SYMBOL_UP);
        lv_obj_set_style_text_color(arrow_up[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(arrow_up[i], &lv_font_montserrat_10, 0);
        lv_obj_set_pos(arrow_up[i], cx + DIGIT_W/2 - 5, 0);

        /* Cell box */
        digit_cells[i] = lv_obj_create(digit_row);
        lv_obj_set_size(digit_cells[i], DIGIT_W, DIGIT_H);
        lv_obj_set_pos(digit_cells[i], cx, 12);
        lv_obj_set_style_bg_color(digit_cells[i], lv_color_hex(0x0E1820), 0);
        lv_obj_set_style_bg_opa(digit_cells[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(digit_cells[i], C_PW_EMPTY, 0);
        lv_obj_set_style_border_width(digit_cells[i], 2, 0);
        lv_obj_set_style_radius(digit_cells[i], 5, 0);
        lv_obj_set_style_pad_all(digit_cells[i], 0, 0);
        lv_obj_clear_flag(digit_cells[i], LV_OBJ_FLAG_SCROLLABLE);

        /* Digit label inside cell */
        digit_lbls[i] = lv_label_create(digit_cells[i]);
        lv_label_set_text(digit_lbls[i], "·");
        lv_obj_set_style_text_color(digit_lbls[i], lv_color_hex(0x2A4A6A), 0);
        lv_obj_set_style_text_font(digit_lbls[i], FN24, 0);
        lv_obj_center(digit_lbls[i]);

        /* Down arrow */
        arrow_dn[i] = lv_label_create(digit_row);
        lv_label_set_text(arrow_dn[i], LV_SYMBOL_DOWN);
        lv_obj_set_style_text_color(arrow_dn[i], C_ACCENT, 0);
        lv_obj_set_style_text_font(arrow_dn[i], &lv_font_montserrat_10, 0);
        lv_obj_set_pos(arrow_dn[i], cx + DIGIT_W/2 - 5, 12 + DIGIT_H + 2);

        /* Position label (bit N) */
        lv_obj_t *pos_lbl = lv_label_create(digit_row);
        char pb[6];
        snprintf(pb, sizeof(pb), "位%d", i+1);
        lv_label_set_text(pos_lbl, pb);
        lv_obj_set_style_text_color(pos_lbl, C_TEXT_HINT, 0);
        lv_obj_set_style_text_font(pos_lbl, UI_FONT_CJK_14, 0);
        lv_obj_set_pos(pos_lbl, cx + 5, 12 + DIGIT_H + 14);
    }

    /* Active / filled styling applied via redraw */
    pw_redraw_all();

    /* ── Status / fail message ────────────────────── */
    lbl_status = lv_label_create(parent);
    lv_label_set_text(lbl_status, "请输入密码");
    lv_obj_set_style_text_color(lbl_status, C_TEXT_HINT, 0);
    lv_obj_set_style_text_font(lbl_status, UI_FONT_CJK_14, 0);
    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 162);

    /* ── Lock icon (hidden by default) ───────────── */
    lock_icon = lv_obj_create(parent);
    lv_obj_set_size(lock_icon, 44, 44);
    lv_obj_align(lock_icon, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(lock_icon, lv_color_hex(0x0E1820), 0);
    lv_obj_set_style_bg_opa(lock_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(lock_icon, C_ALARM, 0);
    lv_obj_set_style_border_width(lock_icon, 3, 0);
    lv_obj_set_style_radius(lock_icon, 22, 0);
    lv_obj_set_style_pad_all(lock_icon, 0, 0);
    lv_obj_clear_flag(lock_icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lock_icon, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lock_lbl = lv_label_create(lock_icon);
    lv_label_set_text(lock_lbl, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(lock_lbl, C_ALARM, 0);
    lv_obj_set_style_text_font(lock_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(lock_lbl);

    /* ── Countdown label (hidden by default) ─────── */
    lbl_countdown = lv_label_create(parent);
    lv_label_set_text(lbl_countdown, "30 s");
    lv_obj_set_style_text_color(lbl_countdown, C_ALARM, 0);
    lv_obj_set_style_text_font(lbl_countdown, &lv_font_montserrat_28, 0);
    lv_obj_align(lbl_countdown, LV_ALIGN_TOP_MID, 0, 126);
    lv_obj_add_flag(lbl_countdown, LV_OBJ_FLAG_HIDDEN);

    /* ── Bottom hint bar ─────────────────────────── */
    ui_hintbar_create(parent, "▲ +1", "▼ −1", "OK 下一位", "ESC 返回");
}
