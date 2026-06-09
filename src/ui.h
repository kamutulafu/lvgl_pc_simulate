#ifndef UI_H
#define UI_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Screen size ─────────────────────────────────── */
#define SCR_W  320
#define SCR_H  240

LV_FONT_DECLARE(ui_font_CJK_24)
LV_FONT_DECLARE(ui_font_CJK_14)
#define UI_FONT_CJK_24 (&ui_font_CJK_24)
#define UI_FONT_CJK_14 (&ui_font_CJK_14)

/* ── Colour palette (dark theme) ────────────────── */
#define C_BG_SCREEN   lv_color_hex(0x000000)
#define C_BG_TOPBAR   lv_color_hex(0x222222)
#define C_BG_CARD     lv_color_hex(0x222222)
#define C_BG_CARD_ALT lv_color_hex(0x441111)  /* alarm card */

#define C_BORDER      lv_color_hex(0x555555)
#define C_BORDER_ALARM lv_color_hex(0xAA3333)

#define C_TEXT_PRI    lv_color_hex(0xFFFFFF)
#define C_TEXT_SEC    lv_color_hex(0xCCCCCC)
#define C_TEXT_HINT   lv_color_hex(0xAAAAAA)
#define C_TEXT_DATE   lv_color_hex(0xCCCCCC)
#define C_TEXT_TEMP   lv_color_hex(0xFFCC00)

#define C_OK          lv_color_hex(0x00FF00)   /* normal / green */
#define C_WARN        lv_color_hex(0xFFFF00)   /* attention / yellow */
#define C_ALARM       lv_color_hex(0xFF0000)   /* alarm / red */
#define C_ACCENT      lv_color_hex(0x00FFFF)   /* title accent */

#define C_BAR_BG      lv_color_hex(0x444444)

#define C_BADGE_OK_BG    lv_color_hex(0x004400)
#define C_BADGE_WARN_BG  lv_color_hex(0x444400)
#define C_BADGE_ALARM_BG lv_color_hex(0x440000)

#define C_PW_ACTIVE   lv_color_hex(0x444444)
#define C_PW_BORDER   lv_color_hex(0x00FFFF)
#define C_PW_FILLED   lv_color_hex(0x888888)
#define C_PW_EMPTY    lv_color_hex(0x555555)

/* ── Gas status enum ─────────────────────────────── */
typedef enum {
    GAS_NORMAL = 0,
    GAS_WARN,
    GAS_ALARM
} gas_status_t;

/* ── Gas channel descriptor ──────────────────────── */
typedef struct {
    const char   *name;       /* e.g. "CO  一氧化碳" */
    const char   *symbol;     /* e.g. "CO" */
    const char   *unit;       /* e.g. "ppm" */
    float         value;
    float         alarm_lo;   /* warn threshold  */
    float         alarm_hi;   /* alarm threshold */
    float         range_max;
    gas_status_t  status;
} gas_ch_t;

/* ── Global gas data (fill from sensor driver) ───── */
extern gas_ch_t g_gas[5];
extern int      g_gas_count;   /* 1‥5 */

/* ── Page create functions ───────────────────────── */
void ui_page_home_create(lv_obj_t *parent);
void ui_page_password_create(lv_obj_t *parent);
void ui_page_menu_create(lv_obj_t *parent);
void ui_page_param_create(lv_obj_t *parent);
void ui_page_curve_create(lv_obj_t *parent);

/* ── Key handler (call from your BSP IRQ/task) ───── */
typedef enum { KEY_UP, KEY_DOWN, KEY_OK, KEY_ESC } ui_key_t;
void ui_key_event(ui_key_t key);

/* ── Periodic refresh (call every ~500 ms) ───────── */
void ui_refresh_home(void);

/* ── Language translation system ─────────────────── */
typedef enum {
    LANG_CHINESE = 0,
    LANG_ENGLISH = 1
} ui_lang_t;

extern ui_lang_t g_lang;
const char *ui_get_text(const char *key);

/* ── Screen manager ──────────────────────────────── */
typedef enum { PAGE_HOME, PAGE_PASSWORD, PAGE_MENU, PAGE_PARAM, PAGE_CURVE } ui_page_t;
extern ui_page_t cur_page;
void ui_goto(ui_page_t page);
void ui_refresh_current_page(void);
void ui_init(void);
void ui_destroy(void);

extern int g_selected_gas;
void ui_home_key(ui_key_t key);
void ui_curve_key(ui_key_t key);

/* Shared helpers used by the page modules. */
void ui_style_screen(lv_obj_t *scr);
lv_obj_t *ui_topbar_create(lv_obj_t *parent,
                           const char *title,
                           const char *badge_text,
                           lv_color_t badge_bg,
                           lv_color_t badge_col);
lv_obj_t *ui_hintbar_create(lv_obj_t *parent,
                            const char *h_up,
                            const char *h_dn,
                            const char *h_ok,
                            const char *h_esc);
lv_obj_t *ui_card_create(lv_obj_t *parent, int x, int y, int w, int h,
                         lv_color_t bg, lv_color_t border);
lv_color_t ui_status_color(gas_status_t s);
lv_color_t ui_status_badge_bg(gas_status_t s);
const char *ui_status_text(gas_status_t s);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_H */
