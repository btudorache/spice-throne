/* Run state, randomness, input, text formatting and the top-level loop. */
#include <stdarg.h>
#include "game.h"

Run R;

/* ---------------------------------------------------------------- rng */
static uint32_t rng = 1;

uint16_t rnd16(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (uint16_t)(rng >> 8);
}

uint8_t rnd(uint8_t n) { return (uint8_t)(((rnd16() & 0xFF) * n) >> 8); }

void rng_seed(uint32_t s) { rng = s ? s : 1; }

/* ------------------------------------------------------------- input */
/* lane 0 = LEFT, 1 = POWER (middle), 2 = RIGHT; a tap fires on release,
 * a hold fires once after 450 ms */
static const uint8_t lane_btn[3] = { BTN_LEFT, BTN_POWER, BTN_RIGHT };
static uint16_t down_t[3];
static uint8_t state[3];

uint8_t input_poll(void)
{
    uint8_t e = hal_btn_take(), h = hal_btn_held();
    uint16_t now = hal_ms();
    for (uint8_t l = 0; l < 3; l++)
        if (e & lane_btn[l]) { state[l] = 1; down_t[l] = now; }
    for (uint8_t l = 0; l < 3; l++) {
        uint8_t b = lane_btn[l];
        if (state[l] == 1) {
            if (!(h & b)) { state[l] = 0; return (uint8_t)(EV_TAP | l); }
            if ((uint16_t)(now - down_t[l]) >= 450) { state[l] = 2; return (uint8_t)(EV_HOLD | l); }
        } else if (state[l] == 2 && !(h & b)) {
            state[l] = 0;
        }
    }
    return EV_NONE;
}

/* forget pending presses; a button still held will not fire on release */
void input_flush(void)
{
    uint8_t h = hal_btn_held();
    hal_btn_take();
    for (uint8_t l = 0; l < 3; l++) state[l] = (h & lane_btn[l]) ? 2 : 0;
}

/* ------------------------------------------------------------ strings */
static const char names_tbl[] PROGMEM =
    "Aren\0Ilya\0Corin\0Dara\0Ezran\0Mira\0Talen\0Sora\0Vashti\0Orin\0Nyla\0Rhun\0Isel\0Joren\0Kira\0Lazar";
static const char houses_tbl[] PROGMEM =
    "Vantor\0Merrow\0Castell\0Korvath\0Sarkand\0Ilmaren\0Tessar\0Halloran";
static const char units_tbl[] PROGMEM = "Levy\0Trooper\0Rifleman\0Sandrider\0Thopter";

static uint8_t copy_nth(char *dst, const char *tbl, uint8_t n)
{
    while (n) if (RD8(tbl++) == 0) n--;
    uint8_t k = 0;
    char c;
    while ((c = (char)RD8(tbl++)) != 0) dst[k++] = c;
    dst[k] = 0;
    return k;
}

uint8_t name_ruler(char *dst) { return copy_nth(dst, names_tbl, R.name); }
uint8_t name_house(char *dst) { return copy_nth(dst, houses_tbl, R.house); }

/* printf-lite over PROGMEM formats: %u unsigned, %s RAM string,
 * %S flash string, %U unit name, %% percent */
static void vfmt(char *b, const char *f, va_list ap)
{
    char c;
    while ((c = (char)RD8(f++)) != 0) {
        if (c != '%') { *b++ = c; continue; }
        c = (char)RD8(f++);
        if (c == 'u') {
            unsigned v = va_arg(ap, unsigned);
            char t[6], *p = t + 5;
            *p = 0;
            do { *--p = (char)('0' + v % 10); v /= 10; } while (v);
            while (*p) *b++ = *p++;
        } else if (c == 's' || c == 'S') {
            const char *s = va_arg(ap, const char *);
            char d;
            while ((d = c == 's' ? *s : (char)RD8(s)) != 0) { *b++ = d; s++; }
        } else if (c == 'U') {
            b += copy_nth(b, units_tbl, (uint8_t)va_arg(ap, int));
        } else {
            *b++ = c;
        }
    }
    *b = 0;
}

char *fmt(char *b, const char *f_P, ...)
{
    va_list ap;
    va_start(ap, f_P);
    vfmt(b, f_P, ap);
    va_end(ap);
    return b;
}

void say(uint8_t x, uint8_t y, uint16_t fg, const char *f_P, ...)
{
    char b[28];
    va_list ap;
    va_start(ap, f_P);
    vfmt(b, f_P, ap);
    va_end(ap);
    uint8_t w = (uint8_t)(gfx_strlen(b) * FONT_W);
    if (x == SAY_C) x = (uint8_t)((LCD_W - w + 1) / 2);
    else if (x == SAY_R) x = (uint8_t)(LCD_W - 2 - w);
    gfx_text(x, y, b, fg, NO_SHADOW, 1);
}

uint16_t bg_ink(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_INK;
}

/* 8x8 2bpp unit sprite: 1 team colour, 2 dark, 3 light, 0 background */
void sprite(uint8_t u, uint8_t frame, uint8_t x, uint8_t y, uint16_t team, uint8_t flip, uint8_t scale)
{
    const uint8_t *p = unit_spr + (uint16_t)(u * 2 + frame) * 16;
    uint8_t n = (uint8_t)(8 * scale);
    lcd_window(x, y, (uint8_t)(x + n - 1), (uint8_t)(y + n - 1));
    for (uint8_t j = 0; j < n; j++) {
        uint8_t r = (uint8_t)(j / scale);
        if (flip) r = (uint8_t)(7 - r);
        uint16_t bits = (uint16_t)((RD8(p + r * 2) << 8) | RD8(p + r * 2 + 1));
        for (uint8_t i = 0; i < n; i++) {
            uint8_t v = (uint8_t)((bits >> (14 - 2 * (i / scale))) & 3);
            lcd_px(v == 1 ? team : v == 2 ? RGB(40, 24, 16) : v == 3 ? RGB(255, 244, 220)
                                                           : gfx_bg((uint8_t)(x + i), (uint8_t)(y + j)));
        }
    }
}

/* -------------------------------------------------------------- game */
static void run_new(void)
{
    memset(&R, 0, sizeof R);
    R.wave = 1;
    R.hp = R.hp_max = 100;
    R.deck = (1 << U_LEVY) | (1 << U_TROOPER);
    for (uint8_t u = 0; u < NUNITS; u++) R.cost[u] = UI8(u, cost);
    R.name = rnd(16);
    R.house = rnd(8);
}

void game_main(void)
{
    for (;;) {
        title_screen();
        run_new();
        while (field_wave()) {
            R.wave++;
            perk_pick();
        }
        end_screen();
    }
}
