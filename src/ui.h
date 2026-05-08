#ifndef UI_H
#define UI_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Screen size ─────────────────────────────────── */
#define SCR_W  320
#define SCR_H  240

LV_FONT_DECLARE(ui_font_CJK_14)
#define UI_FONT_CJK_14 (&ui_font_CJK_14)

/* ── Colour palette (dark theme) ────────────────── */
#define C_BG_SCREEN   lv_color_hex(0x0E1620)
#define C_BG_TOPBAR   lv_color_hex(0x131F2E)
#define C_BG_CARD     lv_color_hex(0x131F2E)
#define C_BG_CARD_ALT lv_color_hex(0x1A1020)  /* alarm card */

#define C_BORDER      lv_color_hex(0x1E3A52)
#define C_BORDER_ALARM lv_color_hex(0x5A1A1A)

#define C_TEXT_PRI    lv_color_hex(0xF0F0F0)
#define C_TEXT_SEC    lv_color_hex(0x7EA8CC)
#define C_TEXT_HINT   lv_color_hex(0x445A6A)
#define C_TEXT_DATE   lv_color_hex(0x7EA8CC)
#define C_TEXT_TEMP   lv_color_hex(0xE8A730)

#define C_OK          lv_color_hex(0x4ECB71)   /* normal / green */
#define C_WARN        lv_color_hex(0xE8A730)   /* attention / amber */
#define C_ALARM       lv_color_hex(0xE05A5A)   /* alarm / red */
#define C_ACCENT      lv_color_hex(0x7ECFFF)   /* title accent */

#define C_BAR_BG      lv_color_hex(0x1E3040)

#define C_BADGE_OK_BG    lv_color_hex(0x1A3020)
#define C_BADGE_WARN_BG  lv_color_hex(0x2A1E10)
#define C_BADGE_ALARM_BG lv_color_hex(0x2A1010)

#define C_PW_ACTIVE   lv_color_hex(0x162840)
#define C_PW_BORDER   lv_color_hex(0x7ECFFF)
#define C_PW_FILLED   lv_color_hex(0x3A6A9A)
#define C_PW_EMPTY    lv_color_hex(0x1E3A52)

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

/* ── Key handler (call from your BSP IRQ/task) ───── */
typedef enum { KEY_UP, KEY_DOWN, KEY_OK, KEY_ESC } ui_key_t;
void ui_key_event(ui_key_t key);

/* ── Periodic refresh (call every ~500 ms) ───────── */
void ui_refresh_home(void);

/* ── Screen manager ──────────────────────────────── */
typedef enum { PAGE_HOME, PAGE_PASSWORD, PAGE_MENU, PAGE_PARAM } ui_page_t;
void ui_goto(ui_page_t page);
void ui_init(void);
void ui_destroy(void);

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
