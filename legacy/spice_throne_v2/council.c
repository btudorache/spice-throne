/* The Hall: Reigns-style petitions. A petitioner's card is leaned left or
 * right (previewing which influences move) and swiped to decide. */
#include "game.h"

#define C_FACE   RGB(232, 208, 160)
#define C_FACE2  RGB(196, 164, 112)
#define C_EDGE   RGB(58, 40, 28)
#define C_BACK   RGB(64, 24, 56)
#define C_BACK2  RGB(170, 90, 40)
#define C_RIBBON RGB(30, 18, 26)
#define TRANSP   NO_SHADOW

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

static uint8_t shown[4];
static char qbuf[128];
static char lab[2][22];

/* ------------------------------------------------------------ shader */
static uint8_t star(uint8_t x, uint8_t y)
{
    uint16_t h = (uint16_t)(x * 0x9E37u ^ y * 0x7F4Bu);
    h ^= h >> 7;
    h *= 0x2C1Bu;
    h ^= h >> 9;
    return (uint8_t)h;
}

/* night desert behind the petitions, the title and the end screen */
uint16_t bg_night(uint8_t x, uint8_t y)
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

/* -------------------------------------------------------------- cards */
typedef struct {
    int16_t cx;
    int8_t shear;
    uint8_t w, who, back;
} Pose;

typedef struct {
    const uint8_t *p;
    int8_t row;
    uint8_t buf[16];
} Dec;

static Pose top;
static int16_t prev_x0 = 200, prev_x1 = -1;
static Dec dec_top;
static const Pose back_pose = { CX, 0, CW, 0, 1 };

static void dec_start(Dec *d, uint8_t who)
{
    d->p = portrait_rle + RD16(portrait_off + who);
    d->row = -1;
}

/* portraits are 16x16, 4 bpp, run-length encoded per row, shown at 3x */
static void dec_row(Dec *d, int8_t r)
{
    while (d->row < r) {
        uint8_t n = 0;
        while (n < 16) {
            uint8_t b = RD8(d->p++);
            uint8_t run = (uint8_t)((b >> 4) + 1), c = (uint8_t)(b & 15);
            while (run-- && n < 16) d->buf[n++] = c;
        }
        d->row++;
    }
}

#define DIV3(v) ((uint8_t)(((uint16_t)(v) * 86) >> 8))   /* exact for 0..47 */

static uint16_t card_px(const Pose *p, const Dec *d, uint8_t u, uint8_t r)
{
    uint8_t eu = (u == 0 || u == CW - 1), er = (r == 0 || r == CH - 1);
    if (eu && er) return TRANSP;
    if (eu || er) return C_EDGE;
    uint16_t base = p->back ? C_BACK : C_FACE;
    if (u >= 4 && u < 52 && r >= 4 && r < 52) {
        uint8_t idx = d->buf[DIV3(u - 4)];
        return idx ? RD16(palette + idx) : base;
    }
    if (u == 2 || u == CW - 3 || r == 2 || r == CH - 3) return p->back ? C_BACK2 : C_FACE2;
    return base;
}

static int16_t pose_left(const Pose *p, uint8_t r)
{
    return (int16_t)(p->cx - (p->w >> 1) + ((int16_t)p->shear * (int16_t)(CH - 1 - r)) / (CH - 1));
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
    int16_t x0 = nx0 < prev_x0 ? nx0 : prev_x0;
    int16_t x1 = nx1 > prev_x1 ? nx1 : prev_x1;
    prev_x0 = nx0;
    prev_x1 = nx1;
    if (x0 < 0) x0 = 0;
    if (x1 > LCD_W - 1) x1 = LCD_W - 1;
    if (x0 > x1) return;

    uint16_t step = top.w ? (uint16_t)((CW << 8) / top.w) : 0;
    dec_start(&dec_top, top.back ? NCHARS : top.who);
    lcd_window((uint8_t)x0, CARD_Y, (uint8_t)x1, (uint8_t)(CARD_Y + CH - 1));
    for (uint8_t r = 0; r < CH; r++) {
        uint8_t y = (uint8_t)(CARD_Y + r);
        int16_t l = pose_left(&top, r);
        if (r >= 4 && r < 52) {
            dec_row(&dec_top, (int8_t)DIV3(r - 4));
        }
        for (int16_t x = x0; x <= x1; x++) {
            uint16_t c = TRANSP;
            int16_t u = (int16_t)(x - l);
            if (u >= 0 && u < top.w) c = card_px(&top, &dec_top, (uint8_t)(((uint16_t)u * step) >> 8), r);
            if (c == TRANSP) c = gfx_bg((uint8_t)x, y);
            lcd_px(c);
        }
    }
}

static void card_flip_in(uint8_t who)
{
    top = back_pose;
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

/* ------------------------------------------------------------- meters */
void meter_icon(uint8_t i, uint8_t x0, uint8_t y0, uint8_t val, uint16_t fill)
{
    uint8_t level = (uint8_t)(((uint16_t)val * 16 + 50) / 100);
    const uint8_t *ic = icon_bits + i * 32;
    lcd_window(x0, y0, (uint8_t)(x0 + 15), (uint8_t)(y0 + 15));
    for (uint8_t r = 0; r < 16; r++) {
        uint16_t bits = (uint16_t)((RD8(ic + r * 2) << 8) | RD8(ic + r * 2 + 1));
        uint16_t on = (r >= 16 - level) ? fill : C_EMPTY;
        for (uint8_t c = 0; c < 16; c++) {
            lcd_px((bits & 0x8000) ? on : C_INK);
            bits <<= 1;
        }
    }
}

uint16_t meter_color(uint8_t v)
{
    return (v <= 15 || v >= 85) ? C_RED : C_SAND;
}

static void meter_draw(uint8_t i, uint8_t v, uint16_t c) { meter_icon(i, (uint8_t)(8 + i * 32), MET_Y, v, c); }

static int8_t eff(const uint8_t *side, uint8_t m)
{
    uint8_t b = RD8(side + (m >> 1));
    int8_t n = (int8_t)((m & 1) ? (b & 0xF0) : (uint8_t)(b << 4)) >> 4;
    return (int8_t)(n * 5);
}

static int8_t prev_eff;

static uint16_t sh_prev(uint8_t x, uint8_t y)
{
    uint8_t dx = (uint8_t)((x & 31) - 8);
    uint8_t dy = (uint8_t)(y - PREV_Y);
    int8_t e = prev_eff;
    if (!e || x < 8) return C_INK;
    uint8_t r = (e >= 15 || e <= -15) ? 2 : 1;
    if (dx >= 8 - r && dx < 8 + r && dy >= 2 - (r >> 1) && dy < 2 + r) return C_SAND;
    return C_INK;
}

static void preview_draw(const uint8_t *side)
{
    for (uint8_t m = 0; m < 4; m++) {
        prev_eff = side ? eff(side, m) : 0;
        gfx_shade((uint8_t)(8 + m * 32), PREV_Y, 16, 4, sh_prev);
    }
}

/* ------------------------------------------------------------- ribbon */
static uint16_t sh_ribbon(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_RIBBON;
}

static void ribbon(const char *s, uint8_t right, uint16_t fg)
{
    if (!s) {
        gfx_clear(0, RIB_Y, LCD_W, RIB_H);
        return;
    }
    gfx_fill(0, RIB_Y, LCD_W, RIB_H, C_RIBBON);
    uint8_t w = (uint8_t)(gfx_strlen(s) * FONT_W);
    shader_t keep = gfx_bg;
    gfx_bg = sh_ribbon;
    gfx_text(right ? (uint8_t)(LCD_W - 2 - w) : 2, RIB_Y + 1, s, fg, NO_SHADOW, 1);
    gfx_bg = keep;
}

/* footer: exact resource and troop changes of the leaned side */
static void footer(const uint8_t *side)
{
    char b[28], *p = b;
    gfx_fill(0, FOOT_Y - 1, LCD_W, 9, C_INK);
    if (!side) return;
    for (uint8_t r = 0; r < NRES + 1; r++) {
        int8_t v = (int8_t)RD8(side + 2 + r);
        if (r == NRES) v = (int8_t)(uint8_t)(v << 3) >> 3;    /* troop count */
        if (!v) continue;
        if (r == NRES) { fmt(p, PSTR("%U"), (uint8_t)RD8(side + 5) >> 5); p[3] = 0; p += 3; }
        else *p++ = "SWP"[r];
        p += gfx_strlen(fmt(p, v > 0 ? PSTR("+%u ") : PSTR("-%u "), (r == NRES ? 1 : 10) * (v > 0 ? v : -v)));
    }
    *p = 0;
    shader_t keep = gfx_bg;
    gfx_bg = bg_ink;
    gfx_text_c(FOOT_Y, b, C_DIM, NO_SHADOW, 1);
    gfx_bg = keep;
}

static void card_lean(int8_t lean, const uint8_t *card)
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
        if (side) { strcpy(b, lab[1]); strcat(b, " >"); }
        else      { b[0] = '<'; b[1] = ' '; strcpy(b + 2, lab[0]); }
        ribbon(b, side, C_TEXT);
        preview_draw(card + 7 + side * 9);
        footer(card + 7 + side * 9);
    } else {
        ribbon(0, 0, 0);
        preview_draw(0);
        footer(0);
    }
}

static void card_swipe(int8_t dir)
{
    int8_t v = 6;
    while (top.cx > -40 && top.cx < LCD_W + 40) {
        top.cx = (int16_t)(top.cx + dir * v);
        if (top.shear > -30 && top.shear < 30) top.shear = (int8_t)(top.shear + dir * 2);
        v = (int8_t)(v + 3);
        card_draw();
        hal_delay(8);
    }
    top = back_pose;
}

static uint8_t play_card(uint8_t ci)
{
    const uint8_t *card = CARD(ci);
    uint16_t off = RD16(card);
    uint8_t who = RD8(card + 2);
    HOOK(HOOK_CARD, ci);

    text_get(&off, qbuf, sizeof qbuf - 1);
    text_get(&off, lab[0], sizeof lab[0] - 3);
    text_get(&off, lab[1], sizeof lab[1] - 3);

    gfx_clear(0, TEXT_Y, LCD_W, TEXT_N * LINE_H);
    ribbon(0, 0, 0);
    preview_draw(0);
    footer(0);
    card_flip_in(who);
    {
        char b[22];
        uint16_t o = RD16(char_name + who);
        text_get(&o, b, 21);
        shader_t keep = gfx_bg;
        gfx_bg = bg_ink;
        gfx_fill(0, NAME_Y, LCD_W, 8, C_INK);
        gfx_text_c(NAME_Y, b, C_ORANGE, NO_SHADOW, 1);
        gfx_bg = keep;
    }
    gfx_wrap(1, TEXT_Y, 21, TEXT_N, LINE_H, qbuf, C_TEXT, 14, 0);

    HOOK(HOOK_SCREEN, 6);
    int8_t lean = 0;
    for (;;) {
        uint8_t ev = input_poll();
        if (!ev) {
            hal_idle();
            continue;
        }
        int8_t want = lean;
        if (ev == EV_L) {
            if (lean < 0) break;
            want = lean > 0 ? 0 : -1;
        } else if (ev == EV_R) {
            if (lean > 0) break;
            want = lean < 0 ? 0 : 1;
        } else if (ev == EV_P) {
            if (lean) break;
            continue;
        } else {
            continue;
        }
        lean = want;
        card_lean(lean, card);
    }
    ribbon(0, 0, 0);
    preview_draw(0);
    footer(0);
    card_swipe(lean);
    return lean > 0;
}

static void meters_animate(void)
{
    uint8_t from[4];
    memcpy(from, shown, 4);
    for (uint8_t s = 1; s <= 8; s++) {
        for (uint8_t m = 0; m < 4; m++) {
            if (from[m] == G.meter[m]) continue;
            shown[m] = (uint8_t)(from[m] + ((int16_t)G.meter[m] - from[m]) * s / 8);
            meter_draw(m, shown[m], G.meter[m] > from[m] ? C_GREEN : C_RED);
        }
        hal_delay(30);
    }
    hal_delay(250);
    for (uint8_t m = 0; m < 4; m++)
        if (from[m] != G.meter[m]) meter_draw(m, shown[m], meter_color(shown[m]));
}

/* ------------------------------------------------------------- effects */
static uint8_t flag_get(uint8_t f) { return (uint8_t)((G.flags[f >> 3] >> (f & 7)) & 1); }
static void flag_put(uint8_t f, uint8_t v)
{
    if (v) G.flags[f >> 3] |= (uint8_t)(1 << (f & 7));
    else   G.flags[f >> 3] &= (uint8_t)~(1 << (f & 7));
}

void update_builtins(void)
{
    uint16_t day = G.secs / DAY_SECS, eta, tot;
    uint8_t who, where, fields = 0;
    for (uint8_t n = 1; n < NNODES; n++)
        if (G.node[n].owner == OWN_PLAYER && RD8(&node_kind[n]) == K_FIELD) fields = 1;
    flag_put(FLAG_DAY5, day >= 5);
    flag_put(FLAG_DAY15, day >= 15);
    flag_put(FLAG_DAY30, day >= 30);
    flag_put(FLAG_ALLIED, G.meter[M_TRIBES] >= 70);
    flag_put(FLAG_FIELDS, fields);
    flag_put(FLAG_WAR, threat(&eta, &tot, &who, &where));
    flag_put(FLAG_SEER, G.charges > 0);
    flag_put(FLAG_FIRST, G.first);
    flag_put(FLAG_RICH, G.res[R_SPICE] >= 600);
    flag_put(FLAG_POOR, G.res[R_SPICE] < 100);
}

static uint8_t cond_ok(uint8_t c)
{
    if (!c) return 1;
    if (c & COND_METER) {
        uint8_t v = G.meter[(c >> 5) & 3];
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
    if ((pr & CP_ONCE) && ((G.used[i >> 3] >> (i & 7)) & 1)) return 0;
    if (!cond_ok(RD8(c + 3)) || !cond_ok(RD8(c + 4)) || !cond_ok(RD8(c + 5))) return 0;
    if (strict)
        for (uint8_t k = 0; k < sizeof G.recent; k++)
            if (G.recent[k] == i) return 0;
    static const uint8_t wmap[4] = { 1, 2, 4, 8 };
    return wmap[pr & 3];
}

static uint8_t pick_card(void)
{
    if (G.next_card != NONE) return G.next_card;
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

static void apply(uint8_t ci, uint8_t side)
{
    const uint8_t *s = CARD(ci) + 7 + side * 9;
    HOOK(HOOK_CHOICE, side);
    for (uint8_t m = 0; m < 4; m++) {
        int16_t v = (int16_t)(G.meter[m] + eff(s, m));
        G.meter[m] = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
    }
    uint16_t cap = world_cap();
    for (uint8_t r = 0; r < NRES; r++) {
        int16_t d = (int16_t)((int8_t)RD8(s + 2 + r) * 10);
        int32_t v = (int32_t)G.res[r] + d;
        G.res[r] = (uint16_t)(v < 0 ? 0 : v > cap ? cap : v);
    }
    uint8_t tr = RD8(s + 5);
    int8_t n = (int8_t)(uint8_t)(tr << 3) >> 3;
    if (n) {
        int16_t v = (int16_t)(G.home[tr >> 5] + n);
        G.home[tr >> 5] = (uint8_t)(v < 0 ? 0 : v > 250 ? 250 : v);
    }
    uint8_t fo = RD8(s + 6);
    if (fo) flag_put((uint8_t)(fo & 0x3F), !(fo & 0x40));
    uint8_t op = RD8(s + 7), arg = (uint8_t)(op & 15), nx = RD8(s + 8);
    switch (op >> 4) {
    case DO_CHARGE: G.charges = (uint8_t)(G.charges + arg > 3 ? 3 : G.charges + arg); break;
    case DO_ATTACK: ai_attack(G.node[N_VULKAR].owner == OWN_VULKAR ? 0 : 1, 1); break;
    case DO_LOSE_NODE: node_lose_random(); break;
    case DO_GAIN_NODE: node_gain_field(); break;
    case DO_WIN: G.result = 1; break;
    case DO_LOSE: G.result = 2; break;
    case DO_CHANCE: if (rnd16() & 0x100) nx = NONE; break;
    }
    G.next_card = nx;
    meters_animate();
}

void council_play(void)
{
    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    for (uint8_t i = 0; i < 4; i++) {
        shown[i] = G.meter[i];
        meter_draw(i, shown[i], meter_color(shown[i]));
    }
    prev_x0 = 200;
    prev_x1 = -1;
    do {
        update_builtins();
        uint8_t ci = pick_card();
        G.next_card = NONE;
        G.first = 0;
        G.used[ci >> 3] |= (uint8_t)(1 << (ci & 7));
        G.recent[G.recent_i] = ci;
        G.recent_i = (uint8_t)((G.recent_i + 1) & 3);
        apply(ci, play_card(ci));
    } while (G.next_card != NONE && !G.result);
    if (G.pending) G.pending--;
}
