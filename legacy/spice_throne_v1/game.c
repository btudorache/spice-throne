/* SPICE THRONE - a Reigns-like chronicle of the desert world Kessar.
 *
 * Screen (portrait 128x160):
 *   0..17   four faction meters (icons that fill from the bottom)
 *   18..22  choice preview marks
 *   24..68  the petition, typed out
 *   70..78  answer ribbon while leaning
 *   81..136 the card (portrait), dunes and stars behind it
 *   141     who is speaking
 *   151     ruler and years of reign
 */
#include <string.h>
#include "hal.h"
#include "gfx.h"
#include "data.h"

/* ------------------------------------------------------------ colours */
#define C_INK    RGB(12, 14, 28)
#define C_LINE   RGB(92, 62, 40)
#define C_SAND   RGB(242, 210, 150)
#define C_TEXT   RGB(246, 234, 210)
#define C_DIM    RGB(120, 106, 140)
#define C_ORANGE RGB(255, 163, 0)
#define C_RED    RGB(255, 70, 60)
#define C_GREEN  RGB(110, 235, 120)
#define C_BLUE   RGB(80, 180, 255)
#define C_FACE   RGB(232, 208, 160)
#define C_FACE2  RGB(196, 164, 112)
#define C_EDGE   RGB(58, 40, 28)
#define C_BACK   RGB(64, 24, 56)
#define C_BACK2  RGB(170, 90, 40)
#define C_EMPTY  RGB(66, 58, 92)
#define C_RIBBON RGB(30, 18, 26)
#define TRANSP   NO_SHADOW

/* ------------------------------------------------------------- layout */
#define MET_Y    2
#define PREV_Y   19
#define TEXT_Y   24
#define TEXT_N   5
#define LINE_H   9
#define RIB_Y    70
#define RIB_H    9
#define CARD_Y   81
#define CW       56
#define CH       56
#define CX       64
#define NAME_Y   142
#define FOOT_Y   151

enum { SCR_TITLE = 1, SCR_CARD, SCR_FATE, SCR_SUCC, SCR_END, SCR_CHRON };

/* --------------------------------------------------------------- save */
#define EE_MAGIC 0
#define EE_BEST  2
#define EE_FATES 4
#define EE_GAMES 6
#define EE_SEED  7
#define MAGIC    0x5A

static struct {
    uint16_t best;
    uint16_t fates;
    uint8_t games;
} save;

static uint16_t ee16(uint16_t a) { return (uint16_t)(hal_ee_read(a) | (hal_ee_read((uint16_t)(a + 1)) << 8)); }
static void ee16w(uint16_t a, uint16_t v) { hal_ee_write(a, (uint8_t)v); hal_ee_write((uint16_t)(a + 1), (uint8_t)(v >> 8)); }

static void save_store(void)
{
    hal_ee_write(EE_MAGIC, MAGIC);
    ee16w(EE_BEST, save.best);
    ee16w(EE_FATES, save.fates);
    hal_ee_write(EE_GAMES, save.games);
}

static void save_load(void)
{
    if (hal_ee_read(EE_MAGIC) != MAGIC) {
        save.best = 0; save.fates = 0; save.games = 0;
        save_store();
    } else {
        save.best = ee16(EE_BEST);
        save.fates = ee16(EE_FATES);
        save.games = hal_ee_read(EE_GAMES);
    }
}

static uint8_t popcount16(uint16_t v)
{
    uint8_t n = 0;
    for (; v; v &= (uint16_t)(v - 1)) n++;
    return n;
}

/* ---------------------------------------------------------------- rng */
static uint32_t rng = 1;

static uint16_t rnd16(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (uint16_t)(rng >> 8);
}

static uint8_t rnd(uint8_t n) { return (uint8_t)(((uint32_t)(rnd16() & 0xFF) * n) >> 8); }

/* -------------------------------------------------------------- state */
#define NBYTES ((NCARDS + 7) / 8)
#define NNAMES 16
#define NHOUSES 8

static struct {
    uint8_t meter[4];
    uint8_t flags[8];
    uint8_t used_game[NBYTES];
    uint8_t used_reign[NBYTES];
    uint8_t recent[8];
    uint8_t recent_i;
    uint8_t lives, presc;
    uint8_t house, name, numeral;
    uint8_t name_count[NNAMES];
    uint8_t age, reign_years, half, reign_cards, rulers;
    uint16_t years;
    uint8_t next;
    uint8_t tutorial;
} st;

static uint8_t shown[4];          /* meter values currently on screen */
static char qbuf[168];
static char lab[2][24];

static const char names_tbl[] PROGMEM =
    "Aren\0Ilya\0Corin\0Dara\0Ezran\0Mira\0Talen\0Sora\0Vashti\0Orin\0Nyla\0Rhun\0Isel\0Joren\0Kira\0Lazar";
static const char houses_tbl[] PROGMEM =
    "Vantor\0Ordane\0Castell\0Merrow\0Sarkand\0Ilmaren\0Tessar\0Korvath";
static const char roman_tbl[] PROGMEM = "I\0II\0III\0IV\0V\0VI\0VII\0VIII\0IX\0X";

static uint8_t copy_nth(char *dst, const char *tbl, uint8_t n)
{
    while (n) if (RD8(tbl++) == 0) n--;
    uint8_t k = 0;
    char c;
    while ((c = (char)RD8(tbl++)) != 0) dst[k++] = c;
    dst[k] = 0;
    return k;
}

uint8_t name_ruler(char *dst)
{
    uint8_t k = copy_nth(dst, names_tbl, st.name);
    dst[k++] = ' ';
    k = (uint8_t)(k + copy_nth(dst + k, roman_tbl, (uint8_t)(st.numeral > 10 ? 9 : st.numeral - 1)));
    return k;
}

uint8_t name_house(char *dst) { return copy_nth(dst, houses_tbl, st.house); }

#ifndef __AVR__
uint8_t sim_meter(uint8_t m) { return st.meter[m]; }
#endif

/* -------------------------------------------------------------- flags */
static uint8_t flag_get(uint8_t f) { return (uint8_t)((st.flags[f >> 3] >> (f & 7)) & 1); }
static void flag_put(uint8_t f, uint8_t v)
{
    if (v) st.flags[f >> 3] |= (uint8_t)(1 << (f & 7));
    else   st.flags[f >> 3] &= (uint8_t)~(1 << (f & 7));
}
static uint8_t bit_get(const uint8_t *b, uint8_t i) { return (uint8_t)((b[i >> 3] >> (i & 7)) & 1); }
static void bit_set(uint8_t *b, uint8_t i) { b[i >> 3] |= (uint8_t)(1 << (i & 7)); }

static uint8_t danger(void)
{
    for (uint8_t m = 0; m < 4; m++)
        if (st.meter[m] <= 15 || st.meter[m] >= 85) return 1;
    return 0;
}

static void update_builtins(void)
{
    flag_put(FLAG_YR5, st.reign_years >= 5);
    flag_put(FLAG_YR10, st.reign_years >= 10);
    flag_put(FLAG_YR20, st.reign_years >= 20);
    flag_put(FLAG_OLD, st.age >= 55);
    flag_put(FLAG_FIRST, st.rulers == 0);
    flag_put(FLAG_HEIR_FULL, st.lives >= 3);
    flag_put(FLAG_HEIR_LAST, st.lives <= 1);
    flag_put(FLAG_SEER, st.presc > 0);
    flag_put(FLAG_NEWREIGN, st.reign_cards == 0);
    flag_put(FLAG_TUTORIAL, st.tutorial);
}

/* --------------------------------------------------------------- LEDs */
static void leds_show(void)
{
    for (uint8_t i = 0; i < 3; i++) {
        if (i >= st.lives) led_set(i, LM_OFF, 0);
        else if (i == 0) led_set(i, danger() ? LM_PULSE : LM_BREATHE, 220);
        else led_set(i, LM_ON, 40);
    }
    static const uint8_t ylvl[4] = { 0, 18, 70, 255 };
    if (st.presc >= 3) led_set(LED_YELLOW, LM_BREATHE, 255);
    else led_set(LED_YELLOW, st.presc ? LM_ON : LM_OFF, ylvl[st.presc]);
}

/* ---------------------------------------------------------- shaders */
static uint8_t star(uint8_t x, uint8_t y)
{
    uint16_t h = (uint16_t)(x * 0x9E37u ^ y * 0x7F4Bu);
    h ^= h >> 7;
    h *= 0x2C1Bu;
    h ^= h >> 9;
    return (uint8_t)h;
}

static uint16_t bg_game(uint8_t x, uint8_t y)
{
    if (y < 22 || y > 139) return C_INK;
    if (y == 22 || y == 139) return C_LINE;
    uint8_t d = RD8(dune_near + x);
    if (y >= d) {
        uint16_t t = (uint16_t)((y - d) * 20);
        return gfx_mix(RGB(150, 82, 48), RGB(60, 28, 22), t > 255 ? 255 : (uint8_t)t, x, y);
    }
    if (y > 72) {
        uint8_t s = star(x, y);
        if (s < 2) return s ? RGB(200, 190, 230) : RGB(120, 110, 150);
    }
    uint16_t t = (uint16_t)((y - 23) * 2 + ((y - 23) >> 2));
    return gfx_mix(RGB(16, 18, 44), RGB(92, 40, 70), t > 255 ? 255 : (uint8_t)t, x, y);
}

static uint16_t bg_fate(uint8_t x, uint8_t y)
{
    return gfx_mix(RGB(70, 14, 24), RGB(10, 4, 8), (uint8_t)(y + (y >> 1) > 255 ? 255 : y + (y >> 1)), x, y);
}

static uint16_t bg_dusk(uint8_t x, uint8_t y)
{
    return gfx_mix(RGB(14, 16, 48), RGB(70, 34, 66), y > 159 ? 255 : (uint8_t)(y + (y >> 1)), x, y);
}

static uint16_t bg_ink(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_INK;
}

/* Title: night sky, twin moons, two dune ranges. */
static uint16_t bg_title(uint8_t x, uint8_t y)
{
    uint8_t near = RD8(dune_near + x), far = RD8(dune_far + x);
    if (y >= near) {
        uint16_t t = (uint16_t)((y - near) * 9);
        return gfx_mix(RGB(84, 40, 26), RGB(18, 8, 8), t > 255 ? 255 : (uint8_t)t, x, y);
    }
    if (y >= far) {
        uint16_t t = (uint16_t)((y - far) * 12);
        return gfx_mix(RGB(186, 92, 46), RGB(110, 44, 34), t > 255 ? 255 : (uint8_t)t, x, y);
    }
    int16_t dx = (int16_t)x - 100, dy = (int16_t)y - 22;
    int16_t d2 = (int16_t)(dx * dx + dy * dy);
    if (d2 < 169) {
        int16_t s = (int16_t)(128 + (dx + dy) * 7);
        if (s < 0) s = 0;
        if (s > 255) s = 255;
        uint16_t c = gfx_mix(RGB(252, 244, 226), RGB(150, 130, 130), (uint8_t)s, x, y);
        if (((x * 7 + y * 3) & 31) == 0 || (dx == -3 && dy == 2) || (dx == 4 && dy == -4)) c = RGB(196, 180, 170);
        return c;
    }
    dx = (int16_t)x - 70; dy = (int16_t)y - 12;
    if (dx * dx + dy * dy < 20) return RGB(190, 214, 255);
    if (y < 80) {
        uint8_t s = star(x, y);
        if (s < 3) return s ? RGB(220, 210, 255) : RGB(255, 240, 200);
    }
    if (y < 60) return gfx_mix(RGB(6, 6, 24), RGB(58, 28, 78), (uint8_t)(y * 4), x, y);
    uint16_t t = (uint16_t)((y - 60) * 6);
    return gfx_mix(RGB(58, 28, 78), RGB(232, 116, 52), t > 255 ? 255 : (uint8_t)t, x, y);
}

/* ------------------------------------------------------------- cards */
typedef struct {
    int16_t cx;     /* centre of the bottom edge */
    int8_t shear;   /* top edge offset relative to bottom */
    uint8_t w;      /* width, < CW while flipping */
    uint8_t who;
    uint8_t back;
} Pose;

typedef struct {
    const uint8_t *p;
    int8_t row;
    uint8_t buf[24];
} Dec;

static Pose top;
static uint8_t under_on;
static uint8_t card_y = CARD_Y;
static int16_t prev_x0 = 200, prev_x1 = -1;
static Dec dec_top, dec_under;
static const Pose back_pose = { CX, 0, CW, 0, 1 };

static void dec_start(Dec *d, uint8_t who)
{
    d->p = portrait_rle + RD16(portrait_off + who);
    d->row = -1;
}

static void dec_row(Dec *d, int8_t r)
{
    while (d->row < r) {
        uint8_t n = 0;
        while (n < 24) {
            uint8_t b = RD8(d->p++);
            uint8_t run = (uint8_t)((b >> 4) + 1), c = (uint8_t)(b & 15);
            while (run-- && n < 24) d->buf[n++] = c;
        }
        d->row++;
    }
}

static uint16_t card_px(const Pose *p, const Dec *d, uint8_t u, uint8_t r)
{
    uint8_t eu = (u == 0 || u == CW - 1), er = (r == 0 || r == CH - 1);
    if (eu && er) return TRANSP;
    if (eu || er) return C_EDGE;
    uint16_t base = p->back ? C_BACK : C_FACE;
    if (u >= 4 && u < 52 && r >= 4 && r < 52) {
        uint8_t idx = d->buf[(u - 4) >> 1];
        return idx ? RD16(palette + idx) : base;
    }
    if (u == 2 || u == CW - 3 || r == 2 || r == CH - 3) return p->back ? C_BACK2 : C_FACE2;
    return base;
}

static int16_t pose_left(const Pose *p, uint8_t r)
{
    return (int16_t)(p->cx - (p->w >> 1) + ((int16_t)p->shear * (int16_t)(CH - 1 - r)) / (CH - 1));
}

static void card_reset(uint8_t y)
{
    card_y = y;
    prev_x0 = 200;
    prev_x1 = -1;
    under_on = 0;
}

/* Redraw the union of the card's previous and current footprint. */
static void card_draw(void)
{
    int16_t nx0 = 200, nx1 = -1;
    if (top.w) {
        int16_t a = pose_left(&top, 0), b = pose_left(&top, CH - 1);
        nx0 = a < b ? a : b;
        nx1 = (int16_t)((a > b ? a : b) + top.w - 1);
    }
    if (under_on) {
        if (nx0 > CX - CW / 2) nx0 = CX - CW / 2;
        if (nx1 < CX + CW / 2 - 1) nx1 = CX + CW / 2 - 1;
    }
    int16_t x0 = nx0 < prev_x0 ? nx0 : prev_x0;
    int16_t x1 = nx1 > prev_x1 ? nx1 : prev_x1;
    prev_x0 = nx0;
    prev_x1 = nx1;
    if (x0 < 0) x0 = 0;
    if (x1 > LCD_W - 1) x1 = LCD_W - 1;
    if (x0 > x1) return;

    uint16_t step = top.w ? (uint16_t)((CW << 8) / top.w) : 0;
    dec_start(&dec_top, top.back ? NCHARS : top.who);
    dec_start(&dec_under, NCHARS);
    lcd_window((uint8_t)x0, card_y, (uint8_t)x1, (uint8_t)(card_y + CH - 1));
    for (uint8_t r = 0; r < CH; r++) {
        uint8_t y = (uint8_t)(card_y + r);
        int16_t l = pose_left(&top, r);
        if (r >= 4 && r < 52) {
            dec_row(&dec_top, (int8_t)((r - 4) >> 1));
            if (under_on) dec_row(&dec_under, (int8_t)((r - 4) >> 1));
        }
        for (int16_t x = x0; x <= x1; x++) {
            uint16_t c = TRANSP;
            int16_t u = (int16_t)(x - l);
            if (u >= 0 && u < top.w) c = card_px(&top, &dec_top, (uint8_t)(((uint16_t)u * step) >> 8), r);
            if (c == TRANSP && under_on) {
                int16_t uu = (int16_t)(x - (CX - CW / 2));
                if (uu >= 0 && uu < CW) c = card_px(&back_pose, &dec_under, (uint8_t)uu, r);
            }
            if (c == TRANSP) c = gfx_bg((uint8_t)x, y);
            lcd_px(c);
        }
    }
}

/* Flip the centred card to show `who` (from its back). */
static void card_flip_in(uint8_t who)
{
    top.cx = CX; top.shear = 0; top.back = 1; top.w = CW;
    card_draw();
    for (int8_t w = CW - 8; w >= 0; w = (int8_t)(w - 8)) {
        top.w = (uint8_t)w;
        card_draw();
        hal_delay(12);
    }
    top.back = 0;
    top.who = who;
    for (uint8_t w = 8; w <= CW; w = (uint8_t)(w + 8)) {
        top.w = w;
        card_draw();
        hal_delay(12);
    }
}

/* ------------------------------------------------------------ meters */
static void meter_draw(uint8_t i, uint8_t val, uint16_t fill)
{
    uint8_t x0 = (uint8_t)(8 + i * 32);
    uint8_t level = (uint8_t)(((uint16_t)val * 16 + 50) / 100);
    const uint8_t *ic = icon_bits + i * 32;
    lcd_window(x0, MET_Y, (uint8_t)(x0 + 15), MET_Y + 15);
    for (uint8_t r = 0; r < 16; r++) {
        uint16_t bits = (uint16_t)((RD8(ic + r * 2) << 8) | RD8(ic + r * 2 + 1));
        uint16_t on = (r >= 16 - level) ? fill : C_EMPTY;
        for (uint8_t c = 0; c < 16; c++) {
            lcd_px((bits & 0x8000) ? on : C_INK);
            bits <<= 1;
        }
    }
}

static uint16_t meter_color(uint8_t v)
{
    return (v <= 15 || v >= 85) ? C_RED : C_SAND;
}

static void meters_draw_all(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        shown[i] = st.meter[i];
        meter_draw(i, shown[i], meter_color(shown[i]));
    }
}

static int8_t eff(const uint8_t *side, uint8_t m)
{
    uint8_t b = RD8(side + (m >> 1));
    int8_t n = (int8_t)((m & 1) ? (b & 0xF0) : (uint8_t)(b << 4)) >> 4;
    return (int8_t)(n * 5);
}

/* Preview marks below the meters: dots sized by magnitude (Reigns-style),
 * or coloured arrows when a spice vision is active. */
static int8_t prev_eff;
static uint8_t prev_vis;

static uint16_t sh_prev(uint8_t x, uint8_t y)
{
    uint8_t dx = (uint8_t)((x & 31) - 8);   /* 0..15 inside the slot  */
    uint8_t dy = (uint8_t)(y - PREV_Y);
    int8_t e = prev_eff;
    if (!e || x < 8) return C_INK;
    uint8_t big = (e >= 15 || e <= -15);
    if (!prev_vis) {
        uint8_t r = big ? 2 : 1;
        if (dx >= 8 - r && dx < 8 + r && dy >= 2 - (r >> 1) && dy < 2 + r) return C_SAND;
        return C_INK;
    }
    uint8_t h = big ? 4 : 3;
    if (dy >= h) return C_INK;
    uint8_t row = e > 0 ? dy : (uint8_t)(h - 1 - dy);     /* arrow tip row 0 */
    int8_t half = (int8_t)row;
    int8_t off = (int8_t)(dx - 7);
    if (off >= -half && off <= half) return e > 0 ? C_GREEN : C_RED;
    return C_INK;
}

static void preview_draw(const uint8_t *side, uint8_t vision)
{
    for (uint8_t m = 0; m < 4; m++) {
        prev_eff = side ? eff(side, m) : 0;
        prev_vis = vision;
        gfx_shade((uint8_t)(8 + m * 32), PREV_Y, 16, 4, sh_prev);
    }
}

/* ------------------------------------------------------------- texts */
static uint16_t sh_ribbon(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_RIBBON;
}

/* align: 0 left, 1 centre, 2 right; s == 0 removes the ribbon */
static void ribbon(const char *s, uint8_t align, uint16_t fg)
{
    if (!s) {
        gfx_clear(0, RIB_Y, LCD_W, RIB_H);
        return;
    }
    gfx_fill(0, RIB_Y, LCD_W, RIB_H, C_RIBBON);
    uint8_t w = (uint8_t)(gfx_strlen(s) * FONT_W);
    uint8_t x = align == 0 ? 2 : align == 2 ? (uint8_t)(LCD_W - 2 - w) : (uint8_t)((LCD_W - w) / 2);
    shader_t keep = gfx_bg;
    gfx_bg = sh_ribbon;
    gfx_text(x, RIB_Y + 1, s, fg, NO_SHADOW, 1);
    gfx_bg = keep;
}

static void ribbon_P(const char *s_P, uint16_t fg)
{
    char b[24];
    uint8_t i = 0;
    while ((b[i] = (char)RD8(s_P + i)) != 0 && i < 22) i++;
    b[i] = 0;
    ribbon(b, 1, fg);
}

static void text_P(uint8_t y, const char *s_P, uint16_t fg, uint8_t scale)
{
    char b[24];
    uint8_t i = 0;
    while ((b[i] = (char)RD8(s_P + i)) != 0 && i < 22) i++;
    b[i] = 0;
    gfx_text_c(y, b, fg, NO_SHADOW, scale);
}

static char *utoa_r(char *end, uint16_t v)
{
    *end = 0;
    do { *--end = (char)('0' + v % 10); v /= 10; } while (v);
    return end;
}

/* builds "<prefix><number><suffix>" in buf */
static char *fmt_num(char *buf, const char *pre_P, uint16_t v, const char *suf_P)
{
    char num[6];
    char *n = utoa_r(num + 5, v);
    uint8_t k = 0;
    char c;
    while ((c = (char)RD8(pre_P++)) != 0) buf[k++] = c;
    while (*n) buf[k++] = *n++;
    while ((c = (char)RD8(suf_P++)) != 0) buf[k++] = c;
    buf[k] = 0;
    return buf;
}

static const char s_empty[] PROGMEM = "";
static const char s_yr[] PROGMEM = "yr ";

static void footer_draw(void)
{
    char b[24];
    gfx_fill(0, FOOT_Y - 1, LCD_W, 9, C_INK);
    name_ruler(b);
    shader_t keep = gfx_bg;
    gfx_bg = bg_ink;
    gfx_text(2, FOOT_Y, b, st.age >= 55 ? C_DIM : C_SAND, NO_SHADOW, 1);
    fmt_num(b, s_yr, st.reign_years, s_empty);
    gfx_text((uint8_t)(LCD_W - 2 - gfx_strlen(b) * FONT_W), FOOT_Y, b, C_DIM, NO_SHADOW, 1);
    gfx_bg = keep;
}

static void speaker_draw(uint8_t who)
{
    char b[24];
    uint16_t off = RD16(char_name + who);
    text_get(&off, b, 22);
    gfx_fill(0, NAME_Y, LCD_W, 8, C_INK);
    shader_t keep = gfx_bg;
    gfx_bg = bg_ink;
    gfx_text_c(NAME_Y, b, C_ORANGE, NO_SHADOW, 1);
    gfx_bg = keep;
}

static void game_screen(void)
{
    HOOK(HOOK_SCREEN, SCR_CARD);
    gfx_bg = bg_game;
    card_reset(CARD_Y);
    gfx_clear(0, 0, LCD_W, LCD_H);
    meters_draw_all();
    footer_draw();
    top.cx = CX; top.shear = 0; top.w = CW; top.back = 1;
    card_draw();
}

/* --------------------------------------------------------- card play */
static void card_lean(int8_t lean, const uint8_t *card, uint8_t vision)
{
    int16_t tx = (int16_t)(CX + lean * 14);
    int8_t ts = (int8_t)(lean * 9);
    for (uint8_t i = 0; i < 4; i++) {
        top.cx = (int16_t)(top.cx + (tx - top.cx) / 2);
        top.shear = (int8_t)(top.shear + (ts - top.shear) / 2);
        if (i == 3) { top.cx = tx; top.shear = ts; }
        card_draw();
        hal_delay(10);
    }
    if (lean) {
        uint8_t side = lean > 0;
        char b[24];
        b[0] = 0;
        if (side) { strcpy(b, lab[1]); strcat(b, " >"); }
        else      { b[0] = '<'; b[1] = ' '; strcpy(b + 2, lab[0]); }
        ribbon(b, side ? 2 : 0, vision ? C_BLUE : C_TEXT);
        preview_draw(card + 7 + side * 5, vision);
    } else {
        ribbon(0, 0, 0);
        preview_draw(0, 0);
    }
}

static void card_swipe(int8_t dir)
{
    under_on = 1;
    int8_t v = 6;
    while (top.cx > -40 && top.cx < LCD_W + 40) {
        top.cx = (int16_t)(top.cx + dir * v);
        if (top.shear > -30 && top.shear < 30) top.shear = (int8_t)(top.shear + dir * 2);
        v = (int8_t)(v + 3);
        card_draw();
        hal_delay(8);
    }
    top = back_pose;
    under_on = 0;
    prev_x0 = CX - CW / 2;
    prev_x1 = CX + CW / 2 - 1;
}

static const char s_vision[] PROGMEM = "~ the spice reveals ~";
static const char s_novision[] PROGMEM = "no visions left";

static void vision_fx(void)
{
    led_set(LED_YELLOW, LM_FLICKER, 255);
    ribbon_P(s_vision, C_BLUE);
    for (uint8_t k = 0; k < 3; k++) {
        for (uint8_t i = 0; i < 4; i++) meter_draw(i, shown[i], C_BLUE);
        hal_delay(120);
        for (uint8_t i = 0; i < 4; i++) meter_draw(i, shown[i], meter_color(shown[i]));
        hal_delay(120);
    }
    hal_delay(300);
    ribbon(0, 0, 0);
    leds_show();
}

static uint8_t play_card(uint8_t ci)
{
    const uint8_t *card = CARD(ci);
    uint16_t off = RD16(card);
    uint8_t who = RD8(card + 2);
    HOOK(HOOK_CARD, ci);

    text_get(&off, qbuf, sizeof qbuf - 1);
    text_get(&off, lab[0], sizeof lab[0] - 4);
    text_get(&off, lab[1], sizeof lab[1] - 4);

    gfx_clear(0, TEXT_Y, LCD_W, TEXT_N * LINE_H);
    ribbon(0, 0, 0);
    preview_draw(0, 0);
    card_flip_in(who);
    speaker_draw(who);
    gfx_wrap(1, TEXT_Y, 21, TEXT_N, LINE_H, qbuf, C_TEXT, 14, 0);

    HOOK(HOOK_SCREEN, SCR_CARD);
    hal_btn_take();
    int8_t lean = 0;
    uint8_t vision = 0;
    for (;;) {
        uint8_t ev = hal_btn_take();
        if (!ev) {
            hal_idle();
            continue;
        }
        int8_t want = lean;
        if (ev & BTN_LEFT) {
            if (lean < 0) break;
            want = lean > 0 ? 0 : -1;
        } else if (ev & BTN_RIGHT) {
            if (lean > 0) break;
            want = lean < 0 ? 0 : 1;
        } else if (ev & BTN_POWER) {
            if (lean) break;
            if (vision) continue;
            if (st.presc) {
                st.presc--;
                vision = 1;
                vision_fx();
            } else {
                ribbon_P(s_novision, C_DIM);
                hal_delay(700);
                ribbon(0, 0, 0);
            }
            continue;
        }
        lean = want;
        card_lean(lean, card, vision);
    }
    ribbon(0, 0, 0);
    preview_draw(0, 0);
    card_swipe(lean);
    return lean > 0;
}

/* animate meters towards st.meter, green when rising, red when falling */
static void meters_animate(void)
{
    uint8_t from[4];
    memcpy(from, shown, 4);
    for (uint8_t s = 1; s <= 8; s++) {
        for (uint8_t m = 0; m < 4; m++) {
            if (from[m] == st.meter[m]) continue;
            shown[m] = (uint8_t)(from[m] + ((int16_t)st.meter[m] - from[m]) * s / 8);
            meter_draw(m, shown[m], st.meter[m] > from[m] ? C_GREEN : C_RED);
        }
        hal_delay(30);
    }
    hal_delay(250);
    for (uint8_t m = 0; m < 4; m++)
        if (from[m] != st.meter[m]) meter_draw(m, shown[m], meter_color(shown[m]));
}

#define FATE_NONE 0xFF

static uint8_t resolve(uint8_t ci, uint8_t side)
{
    const uint8_t *s = CARD(ci) + 7 + side * 5;
    uint8_t fate = FATE_NONE;
    HOOK(HOOK_CHOICE, side);

    for (uint8_t m = 0; m < 4; m++) {
        int16_t v = (int16_t)(st.meter[m] + eff(s, m));
        st.meter[m] = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
    }
    uint8_t fo = RD8(s + 2);
    if (fo) flag_put((uint8_t)(fo & 0x3F), !(fo & 0x40));

    uint8_t op = RD8(s + 3), arg = (uint8_t)(op & 15), nx = RD8(s + 4);
    switch (op >> 4) {
    case DO_PRESC:
        st.presc = (uint8_t)(st.presc + arg > 3 ? 3 : st.presc + arg);
        break;
    case DO_HEIR_UP:
        if (st.lives < 3) {
            led_set(st.lives, LM_FADEIN, 40);
            st.lives++;
            hal_delay(1000);
        }
        break;
    case DO_HEIR_DOWN:
        if (st.lives > 1) {
            st.lives--;
            led_set(st.lives, LM_FADEOUT, 200);
            hal_delay(1000);
        }
        break;
    case DO_FATE:
        fate = arg;
        break;
    case DO_AGE:
        st.age = (uint8_t)(st.age + arg);
        st.reign_years = (uint8_t)(st.reign_years + arg);
        st.years = (uint16_t)(st.years + arg);
        break;
    case DO_CHANCE:
        if (rnd16() & 0x100) nx = CARD_NONE;
        break;
    }
    st.next = nx;

    meters_animate();

    st.half ^= 1;
    if (!st.half) {
        st.reign_years++;
        st.age++;
        st.years++;
        if ((st.reign_years & 7) == 0 && st.presc < 3) st.presc++;
        if (fate == FATE_NONE && st.age >= 75 && rnd(4) == 0) fate = FATE_OLD_AGE;
    }
    footer_draw();

    if (fate == FATE_NONE) {
        for (uint8_t m = 0; m < 4; m++) {
            if (st.meter[m] == 0 || st.meter[m] == 100) {
                fate = (uint8_t)(m * 2 + (st.meter[m] == 100));
                for (uint8_t k = 0; k < 4; k++) {
                    meter_draw(m, st.meter[m], C_RED);
                    hal_delay(150);
                    meter_draw(m, st.meter[m], C_INK);
                    hal_delay(150);
                }
                break;
            }
        }
    }
    leds_show();
    return fate;
}

/* ------------------------------------------------------- card choice */
static uint8_t cond_ok(uint8_t c)
{
    if (!c) return 1;
    if (c & COND_METER) {
        uint8_t v = st.meter[(c >> 5) & 3];
        uint8_t th = (uint8_t)(20 + 5 * (c & 15));
        return (c & 0x10) ? v >= th : v < th;
    }
    uint8_t set = flag_get((uint8_t)(c & 0x3F));
    return (c & COND_FLAG_CLEAR) ? !set : set;
}

static uint8_t weight(uint8_t i, uint8_t strict)
{
    const uint8_t *c = CARD(i);
    uint8_t pr = RD8(c + 6);
    if (pr & CP_CHAIN) return 0;
    if ((pr & CP_ONCE) && bit_get(st.used_game, i)) return 0;
    if ((pr & CP_REIGN_ONCE) && bit_get(st.used_reign, i)) return 0;
    if (!cond_ok(RD8(c + 3)) || !cond_ok(RD8(c + 4)) || !cond_ok(RD8(c + 5))) return 0;
    if (strict)
        for (uint8_t k = 0; k < sizeof st.recent; k++)
            if (st.recent[k] == i) return 0;
    static const uint8_t wmap[4] = { 1, 2, 4, 8 };
    return wmap[pr & 3];
}

static uint8_t pick_card(void)
{
    if (st.next != CARD_NONE) return st.next;
    for (uint8_t i = 0; i < NCARDS; i++)
        if ((RD8(CARD(i) + 6) & CP_PRIORITY) && weight(i, 1)) return i;
    for (uint8_t strict = 1;; strict = 0) {
        uint16_t total = 0;
        for (uint8_t i = 0; i < NCARDS; i++) total = (uint16_t)(total + weight(i, strict));
        if (total) {
            uint16_t r = (uint16_t)(rnd16() % total);
            for (uint8_t i = 0; i < NCARDS; i++) {
                uint8_t w = weight(i, strict);
                if (r < w) return i;
                r = (uint16_t)(r - w);
            }
        }
        if (!strict) return 0;
    }
}

static void mark_played(uint8_t i)
{
    bit_set(st.used_game, i);
    bit_set(st.used_reign, i);
    st.recent[st.recent_i] = i;
    st.recent_i = (uint8_t)((st.recent_i + 1) & 7);
}

/* returns the fate that ended the reign */
static uint8_t reign(void)
{
    for (;;) {
        update_builtins();
        uint8_t ci = pick_card();
        st.next = CARD_NONE;
        mark_played(ci);
        uint8_t side = play_card(ci);
        st.reign_cards++;
        uint8_t fate = resolve(ci, side);
        if (fate != FATE_NONE) return fate;
    }
}

/* ----------------------------------------------------------- screens */
static uint8_t wait_key(uint8_t scr, uint8_t y, const char *hint_P, shader_t bg)
{
    HOOK(HOOK_SCREEN, scr);
    hal_btn_take();
    uint16_t t = hal_ms();
    uint8_t on = 0;
    for (;;) {
        uint8_t e = hal_btn_take();
        if (e) return e;
        if ((uint16_t)(hal_ms() - t) >= 550) {
            t = hal_ms();
            on ^= 1;
            gfx_bg = bg;
            if (on) text_P(y, hint_P, C_DIM, 1);
            else gfx_clear(0, y, LCD_W, 8);
        }
        hal_idle();
    }
}

static const char s_press[] PROGMEM = "- press POWER -";

static void new_ruler(void)
{
    if (st.rulers && rnd(10) < 4) {
        /* keep the old name: the dynasty loves a numeral */
    } else {
        st.name = rnd(NNAMES);
    }
    st.numeral = ++st.name_count[st.name];
    st.age = (uint8_t)(st.rulers ? 15 + rnd(12) : 19 + rnd(9));
    st.reign_years = 0;
    st.half = 0;
    st.reign_cards = 0;
    st.next = CARD_NONE;
    for (uint8_t m = 0; m < 4; m++) st.meter[m] = 50;
    for (uint8_t f = FLAG_REIGN_FIRST; f < 64; f++) flag_put(f, 0);
    memset(st.used_reign, 0, sizeof st.used_reign);
}

static const char s_decree[] PROGMEM = "By Imperial decree";
static const char s_passes[] PROGMEM = "The throne passes to";
static const char s_house[] PROGMEM = "House ";
static const char s_rules[] PROGMEM = "now rules Kessar.";
static const char s_age[] PROGMEM = "age ";
static const char s_blood[] PROGMEM = "bloodline: ";

static void succession_screen(void)
{
    char b[24];
    HOOK(HOOK_SCREEN, SCR_SUCC);
    gfx_bg = bg_dusk;
    gfx_clear(0, 0, LCD_W, LCD_H);
    leds_show();
    card_reset(10);
    card_flip_in(CHAR_HEIR);
    text_P(74, st.rulers ? s_passes : s_decree, C_DIM, 1);
    name_ruler(b);
    gfx_text_c(88, b, C_ORANGE, RGB(40, 10, 10), 2);
    fmt_num(b, s_age, st.age, s_empty);
    gfx_text_c(110, b, C_SAND, NO_SHADOW, 1);
    if (!st.rulers) {
        uint8_t k = 0;
        char c;
        const char *p = s_house;
        while ((c = (char)RD8(p++)) != 0) b[k++] = c;
        name_house(b + k);
        gfx_text_c(122, b, C_TEXT, NO_SHADOW, 1);
        text_P(131, s_rules, C_TEXT, 1);
    } else {
        fmt_num(b, s_blood, st.lives, s_empty);
        gfx_text_c(124, b, C_GREEN, NO_SHADOW, 1);
    }
    wait_key(SCR_SUCC, 150, s_press, bg_dusk);
}

static const char s_ruled[] PROGMEM = " years";
static const char s_yrs[] PROGMEM = " yrs";

static void fate_screen(uint8_t f)
{
    char b[24];
    HOOK(HOOK_FATE, f);
    save.fates |= (uint16_t)(1u << f);
    save_store();
    for (uint8_t i = 0; i < 3; i++)
        if (i < st.lives) led_set(i, LM_BLINK, 200);

    gfx_bg = bg_fate;
    gfx_clear(0, 0, LCD_W, LCD_H);
    card_reset(6);
    card_flip_in(RD8(fate_who + f));

    uint16_t off = RD16(fate_text + f);
    text_get(&off, b, 22);
    gfx_text_c(66, b, f == FATE_GREEN_WORLD ? C_GREEN : C_ORANGE, RGB(20, 0, 0), 1);
    text_get(&off, qbuf, sizeof qbuf - 1);
    gfx_wrap(1, 79, 21, 8, 8, qbuf, C_TEXT, 10, 1);

    {
        char y[8];
        uint8_t k = name_ruler(b);
        b[k++] = ':';
        b[k++] = ' ';
        fmt_num(y, s_empty, st.reign_years, s_yrs);
        strcpy(b + k, y);
        gfx_text_c(144, b, C_SAND, NO_SHADOW, 1);
    }
    if (st.lives) led_set((uint8_t)(st.lives - 1), LM_FADEOUT, 200);
    hal_delay(1000);
    wait_key(SCR_FATE, 152, s_press, bg_fate);
}

static const char s_fallen[] PROGMEM = "has fallen.";
static const char s_bloom[] PROGMEM = "made Kessar bloom!";
static const char s_lasted[] PROGMEM = "endured ";
static const char s_rulers[] PROGMEM = "rulers: ";
static const char s_best[] PROGMEM = "best: ";
static const char s_record[] PROGMEM = "NEW RECORD";

static void dynasty_end(uint8_t victory)
{
    char b[24];
    HOOK(HOOK_DYNASTY_END, st.years);
    uint8_t record = st.years > save.best;
    if (record) save.best = st.years;
    if (save.games < 255) save.games++;
    save_store();

    gfx_bg = victory ? bg_dusk : bg_fate;
    gfx_clear(0, 0, LCD_W, LCD_H);
    text_P(14, s_house, C_DIM, 1);
    name_house(b);
    gfx_text_c(26, b, C_ORANGE, RGB(30, 6, 6), 2);
    text_P(50, victory ? s_bloom : s_fallen, victory ? C_GREEN : C_TEXT, 1);
    gfx_text_c(72, fmt_num(b, s_lasted, st.years, s_ruled), C_SAND, NO_SHADOW, 1);
    gfx_text_c(86, fmt_num(b, s_rulers, st.rulers, s_empty), C_SAND, NO_SHADOW, 1);
    gfx_text_c(100, fmt_num(b, s_best, save.best, s_ruled), C_DIM, NO_SHADOW, 1);
    if (record) text_P(118, s_record, C_GREEN, 2);
    for (uint8_t i = 0; i < 3; i++) led_set(i, record || victory ? LM_BLINK : LM_OFF, 255);
    led_set(LED_YELLOW, record ? LM_BLINK : LM_OFF, 255);
    hal_delay(1200);
    wait_key(SCR_END, 148, s_press, gfx_bg);
}

static const char s_chron[] PROGMEM = "CHRONICLE OF FATES";
static const char s_unknown[] PROGMEM = "? ? ?";
static const char s_games[] PROGMEM = "games ";
static const char s_erase[] PROGMEM = "hold L+R: erase";
static const char s_erased[] PROGMEM = "chronicle erased";

static void chronicle(void)
{
    char b[24];
    HOOK(HOOK_SCREEN, SCR_CHRON);
    gfx_bg = bg_ink;
    gfx_clear(0, 0, LCD_W, LCD_H);
    text_P(2, s_chron, C_ORANGE, 1);
    for (uint8_t f = 0; f < NFATES; f++) {
        uint8_t y = (uint8_t)(13 + f * 8);
        if (save.fates & (1u << f)) {
            uint16_t off = RD16(fate_text + f);
            text_get(&off, b, 21);
            gfx_text_c(y, b, f == FATE_GREEN_WORLD ? C_GREEN : C_SAND, NO_SHADOW, 1);
        } else {
            text_P(y, s_unknown, C_EMPTY, 1);
        }
    }
    char n[12];
    fmt_num(b, s_best, save.best, s_empty);
    strcat(b, "  ");
    strcat(b, fmt_num(n, s_games, save.games, s_empty));
    gfx_text_c(143, b, C_DIM, NO_SHADOW, 1);
    text_P(152, s_erase, C_EMPTY, 1);

    hal_btn_take();
    uint16_t held_since = 0;
    uint8_t holding = 0;
    for (;;) {
        uint8_t e = hal_btn_take();
        uint8_t h = hal_btn_held();
        if ((h & (BTN_LEFT | BTN_RIGHT)) == (BTN_LEFT | BTN_RIGHT)) {
            if (!holding) { holding = 1; held_since = hal_ms(); }
            else if ((uint16_t)(hal_ms() - held_since) > 3000) {
                save.best = 0; save.fates = 0; save.games = 0;
                save_store();
                gfx_fill(0, 150, LCD_W, 10, C_INK);
                text_P(152, s_erased, C_RED, 1);
                hal_delay(1500);
                return;
            }
        } else {
            holding = 0;
            if (e & BTN_POWER) return;
        }
        hal_idle();
    }
}

/* ------------------------------------------------------------- title */
#define WORM_SEGS 8
static int16_t wx[WORM_SEGS], wy[WORM_SEGS];
static uint8_t wr[WORM_SEGS], wvis[WORM_SEGS];

static const uint8_t sin_q[17] PROGMEM = { 0, 25, 50, 74, 98, 121, 142, 162, 180, 197, 212, 225, 236, 244, 250, 254, 255 };

static uint8_t sin_half(uint8_t k) /* sin(pi * k / 32), k = 0..32 */
{
    return RD8(sin_q + (k <= 16 ? k : 32 - k));
}

static uint16_t sh_worm(uint8_t x, uint8_t y)
{
    if (y < RD8(dune_near + x)) {
        for (uint8_t k = 0; k < WORM_SEGS; k++) {
            if (!wvis[k]) continue;
            int16_t dx = (int16_t)(x - wx[k]), dy = (int16_t)(y - wy[k]);
            int16_t r = wr[k];
            if (dx > r || dx < -r || dy > r || dy < -r) continue;
            int16_t d2 = (int16_t)(dx * dx + dy * dy);
            if (d2 > r * r) continue;
            if (k == 0) {
                if (d2 <= (r - 4) * (r - 4)) return RGB(36, 8, 14);
                if (d2 <= (r - 2) * (r - 2)) return ((x + y) & 1) ? RGB(250, 246, 230) : RGB(120, 30, 60);
            }
            if (d2 > (r - 1) * (r - 1)) return RGB(80, 42, 24);
            return (dy < -1) ? ((k & 1) ? RGB(222, 168, 104) : RGB(200, 146, 86))
                             : ((k & 1) ? RGB(176, 116, 66) : RGB(156, 98, 56));
        }
    }
    return bg_title(x, y);
}

static void worm_breach(uint8_t dir)
{
    int16_t x0 = 127, x1 = 0, y0 = 159, y1 = 0;
    for (uint8_t phase = 0; phase <= 32 + WORM_SEGS * 3; phase++) {
        int16_t nx0 = 127, nx1 = 0, ny0 = 159, ny1 = 0;
        for (uint8_t k = 0; k < WORM_SEGS; k++) {
            int8_t s = (int8_t)(phase - k * 3);
            wvis[k] = (s >= 0 && s <= 32);
            if (!wvis[k]) continue;
            int16_t px = (int16_t)(12 + (s * 104) / 32);
            wx[k] = dir ? (int16_t)(127 - px) : px;
            wy[k] = (int16_t)(130 - (sin_half((uint8_t)s) * 34) / 255);
            wr[k] = (uint8_t)(k == 0 ? 7 : 7 - (k >> 1));
            if (wx[k] - wr[k] < nx0) nx0 = (int16_t)(wx[k] - wr[k]);
            if (wx[k] + wr[k] > nx1) nx1 = (int16_t)(wx[k] + wr[k]);
            if (wy[k] - wr[k] < ny0) ny0 = (int16_t)(wy[k] - wr[k]);
            if (wy[k] + wr[k] > ny1) ny1 = (int16_t)(wy[k] + wr[k]);
        }
        int16_t ax0 = nx0 < x0 ? nx0 : x0, ax1 = nx1 > x1 ? nx1 : x1;
        int16_t ay0 = ny0 < y0 ? ny0 : y0, ay1 = ny1 > y1 ? ny1 : y1;
        if (ax0 < 0) ax0 = 0;
        if (ax1 > 127) ax1 = 127;
        if (ay1 > 159) ay1 = 159;
        if (ax0 <= ax1 && ay0 <= ay1)
            gfx_shade((uint8_t)ax0, (uint8_t)ay0, (uint8_t)(ax1 - ax0 + 1), (uint8_t)(ay1 - ay0 + 1), sh_worm);
        x0 = nx0; x1 = nx1; y0 = ny0; y1 = ny1;
        hal_delay(20);
    }
}

static const char s_title1[] PROGMEM = "SPICE";
static const char s_title2[] PROGMEM = "THRONE";
static const char s_sub[] PROGMEM = "chronicles of Kessar";
static const char s_l_r[] PROGMEM = "L/R: chronicle";
static const char s_fates[] PROGMEM = "  fates ";

static void title_draw(void)
{
    char b[24], n[8];
    gfx_bg = bg_title;
    gfx_clear(0, 0, LCD_W, LCD_H);
    {
        char t[8];
        uint8_t i = 0;
        while ((t[i] = (char)RD8(s_title1 + i)) != 0) i++;
        gfx_text_c(40, t, C_ORANGE, RGB(40, 8, 20), 3);
        i = 0;
        while ((t[i] = (char)RD8(s_title2 + i)) != 0) i++;
        gfx_text_c(66, t, RGB(255, 214, 120), RGB(40, 8, 20), 2);
    }
    text_P(86, s_sub, RGB(250, 200, 170), 1);
    fmt_num(b, s_best, save.best, s_empty);
    fmt_num(n, s_empty, popcount16(save.fates), s_empty);
    {
        uint8_t k = gfx_strlen(b);
        const char *p = s_fates;
        char c;
        while ((c = (char)RD8(p++)) != 0) b[k++] = c;
        for (uint8_t i = 0; n[i]; i++) b[k++] = n[i];
        b[k++] = '/';
        b[k++] = '1';
        b[k++] = '6';
        b[k] = 0;
    }
    gfx_text_c(141, b, RGB(150, 110, 100), NO_SHADOW, 1);
    text_P(151, s_l_r, RGB(110, 80, 80), 1);
}

static void title(void)
{
    title_draw();
    led_set(LED_YELLOW, LM_BREATHE, 120);
    for (uint8_t i = 0; i < 3; i++) led_set(i, LM_BREATHE, 160);
    HOOK(HOOK_SCREEN, SCR_TITLE);
    hal_btn_take();
    uint16_t t_blink = hal_ms(), t_worm = hal_ms();
    uint8_t on = 0, dir = 0;
    for (;;) {
        uint8_t e = hal_btn_take();
        if (e & BTN_POWER) {
            rng ^= (uint32_t)hal_ms() << 7;
            rnd16();
            return;
        }
        if (e & (BTN_LEFT | BTN_RIGHT)) {
            chronicle();
            title_draw();
            HOOK(HOOK_SCREEN, SCR_TITLE);
        }
        uint16_t now = hal_ms();
        if ((uint16_t)(now - t_blink) >= 600) {
            t_blink = now;
            on ^= 1;
            if (on) text_P(108, s_press, C_SAND, 1);
            else gfx_clear(0, 108, LCD_W, 8);
        }
        if ((uint16_t)(now - t_worm) >= 7000) {
            worm_breach(dir);
            dir ^= 1;
            t_worm = hal_ms();
        }
        hal_idle();
    }
}

/* ------------------------------------------------------------ dynasty */
static void dynasty(void)
{
    memset(&st, 0, sizeof st);
    memset(st.recent, CARD_NONE, sizeof st.recent);
    st.tutorial = (save.games == 0);
    st.house = rnd(NHOUSES);
    st.lives = 3;
    st.presc = 1;
    uint8_t victory = 0;
    for (;;) {
        new_ruler();
        succession_screen();
        game_screen();
        leds_show();
        uint8_t fate = reign();
        hal_delay(400);
        fate_screen(fate);
        st.rulers++;
        if (fate == FATE_GREEN_WORLD) { victory = 1; break; }
        st.lives--;
        if (!st.lives) break;
    }
    dynasty_end(victory);
}

void game_main(void)
{
    save_load();
    uint16_t seed = (uint16_t)(ee16(EE_SEED) + 1);
    ee16w(EE_SEED, seed);
    rng = hal_entropy() ^ ((uint32_t)seed << 16) ^ seed;
    if (!rng) rng = 0x1234567;
    for (;;) {
        title();
        dynasty();
    }
}
