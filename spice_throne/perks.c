/* Between waves a petitioner offers a bargain: a Reigns-style card that is
 * leaned left or right (previewing the effects) and swiped to decide. */
#include "game.h"

#define C_FACE   RGB(232, 208, 160)
#define C_FACE2  RGB(196, 164, 112)
#define C_EDGE   RGB(58, 40, 28)
#define C_BACK   RGB(64, 24, 56)
#define C_BACK2  RGB(170, 90, 40)
#define C_RIBBON RGB(30, 18, 26)
#define TRANSP   NO_SHADOW

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

static char qbuf[128];
static char lab[2][20];

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
static int16_t prev_x0, prev_x1;
static Dec dec;
static const Pose back_pose = { CX, 0, CW, 0, 1 };

/* portraits are 16x16, 4 bpp, run-length encoded per row, shown at 3x */
static void dec_start(uint8_t who)
{
    dec.p = portrait_rle + RD16(portrait_off + who);
    dec.row = -1;
}

static void dec_row(int8_t r)
{
    while (dec.row < r) {
        uint8_t n = 0;
        while (n < 16) {
            uint8_t b = RD8(dec.p++);
            uint8_t run = (uint8_t)((b >> 4) + 1), c = (uint8_t)(b & 15);
            while (run-- && n < 16) dec.buf[n++] = c;
        }
        dec.row++;
    }
}

#define DIV3(v) ((uint8_t)(((uint16_t)(v) * 86) >> 8))   /* exact for 0..47 */

static uint16_t card_px(uint8_t u, uint8_t r)
{
    uint8_t eu = (u == 0 || u == CW - 1), er = (r == 0 || r == CH - 1);
    if (eu && er) return TRANSP;
    if (eu || er) return C_EDGE;
    uint16_t base = top.back ? C_BACK : C_FACE;
    if (u >= 4 && u < 52 && r >= 4 && r < 52) {
        uint8_t idx = dec.buf[DIV3(u - 4)];
        return idx ? RD16(palette + idx) : base;
    }
    if (u == 2 || u == CW - 3 || r == 2 || r == CH - 3) return top.back ? C_BACK2 : C_FACE2;
    return base;
}

static int16_t pose_left(uint8_t r)
{
    return (int16_t)(top.cx - (top.w >> 1) + ((int16_t)top.shear * (int16_t)(CH - 1 - r)) / (CH - 1));
}

/* Redraw the union of the card's previous and current footprint. */
static void card_draw(void)
{
    int16_t nx0 = 200, nx1 = -1;
    if (top.w) {
        int16_t a = pose_left(0), b = pose_left(CH - 1);
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
    dec_start(top.back ? NCHARS : top.who);
    lcd_window((uint8_t)x0, CARD_Y, (uint8_t)x1, (uint8_t)(CARD_Y + CH - 1));
    for (uint8_t r = 0; r < CH; r++) {
        uint8_t y = (uint8_t)(CARD_Y + r);
        int16_t l = pose_left(r);
        if (r >= 4 && r < 52) dec_row((int8_t)DIV3(r - 4));
        for (int16_t x = x0; x <= x1; x++) {
            uint16_t c = TRANSP;
            int16_t u = (int16_t)(x - l);
            if (u >= 0 && u < top.w) c = card_px((uint8_t)(((uint16_t)u * step) >> 8), r);
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
}

/* ----------------------------------------------------------- effects */
static void fx_text(char *b, uint8_t fx)
{
    uint8_t a = (uint8_t)(fx & 15);
    switch (fx >> 4) {
    case FX_UNLOCK:   fmt(b, PSTR("+ %U joins"), a); break;
    case FX_HP:       fmt(b, PSTR("%U HP +25%%"), a); break;
    case FX_ATK:      fmt(b, PSTR("%U ATK +25%%"), a); break;
    case FX_COST:     fmt(b, PSTR("%U costs -1"), a); break;
    case FX_INCOME:   fmt(b, PSTR("spice flows faster")); break;
    case FX_SPICE:    fmt(b, PSTR("+%u spice each wave"), a); break;
    case FX_WALL:     fmt(b, PSTR("wall +%u"), a * 10); break;
    case FX_HEAL:     fmt(b, PSTR("repair %u"), a * 10); break;
    case FX_HURT:     fmt(b, PSTR("wall -%u"), a * 10); break;
    case FX_HARDER:   fmt(b, PSTR("foes +%u%% HP"), a * 10); break;
    case FX_WORM:     fmt(b, PSTR("worm awake next wave")); break;
    case FX_WORMRATE: fmt(b, PSTR("worm wakes faster")); break;
    default:          b[0] = 0; break;
    }
}

static uint8_t fx_bad(uint8_t fx) { return (fx >> 4) == FX_HURT || (fx >> 4) == FX_HARDER; }

static void fx_apply(uint8_t fx)
{
    uint8_t a = (uint8_t)(fx & 15);
    switch (fx >> 4) {
    case FX_UNLOCK:   R.deck |= (uint8_t)(1 << a); break;
    case FX_HP:       if (R.hp_up[a] < 8) R.hp_up[a]++; break;
    case FX_ATK:      if (R.atk_up[a] < 8) R.atk_up[a]++; break;
    case FX_COST:     if (R.cost[a] > 1) R.cost[a]--; break;
    case FX_INCOME:   R.income++; break;
    case FX_SPICE:    R.spice_bonus = (uint8_t)(R.spice_bonus + a); break;
    case FX_WALL:     R.hp_max = (uint8_t)(R.hp_max + a * 10 > 250 ? 250 : R.hp_max + a * 10);
                      R.hp = (uint8_t)(R.hp + a * 10 > R.hp_max ? R.hp_max : R.hp + a * 10); break;
    case FX_HEAL:     R.hp = (uint8_t)(R.hp + a * 10 > R.hp_max ? R.hp_max : R.hp + a * 10); break;
    case FX_HURT:     R.hp = R.hp > a * 10 ? (uint8_t)(R.hp - a * 10) : 1; break;
    case FX_HARDER:   R.harder = (uint8_t)(R.harder + a); break;
    case FX_WORM:     R.worm_ready = 1; break;
    case FX_WORMRATE: R.worm_rate++; break;
    }
}

/* the two effect lines under the card while leaning, the speaker otherwise */
static void effects_show(const uint8_t *side, uint8_t who)
{
    char b[24];
    gfx_bg = bg_ink;
    gfx_fill(0, NAME_Y, LCD_W, 18, C_INK);
    if (!side) {
        uint16_t o = RD16(char_name + who);
        text_get(&o, b, 21);
        gfx_text_c(NAME_Y, b, C_ORANGE, NO_SHADOW, 1);
        return;
    }
    for (uint8_t k = 0; k < 2; k++) {
        uint8_t fx = RD8(side + k);
        fx_text(b, fx);
        gfx_text_c((uint8_t)(NAME_Y + k * 9), b, fx_bad(fx) ? C_RED : C_GREEN, NO_SHADOW, 1);
    }
}

static uint16_t sh_ribbon(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_RIBBON;
}

static void ribbon(int8_t lean)
{
    gfx_bg = bg_night;
    if (!lean) {
        gfx_clear(0, RIB_Y, LCD_W, RIB_H);
        return;
    }
    char b[24];
    gfx_fill(0, RIB_Y, LCD_W, RIB_H, C_RIBBON);
    fmt(b, lean > 0 ? PSTR("%s >") : PSTR("< %s"), lab[lean > 0]);
    gfx_bg = sh_ribbon;
    uint8_t w = (uint8_t)(gfx_strlen(b) * FONT_W);
    gfx_text(lean > 0 ? (uint8_t)(LCD_W - 2 - w) : 2, RIB_Y + 1, b, C_TEXT, NO_SHADOW, 1);
}

static void card_lean(int8_t lean, const uint8_t *card)
{
    int16_t tx = (int16_t)(CX + lean * 14);
    int8_t ts = (int8_t)(lean * 9);
    gfx_bg = bg_night;
    for (uint8_t i = 0; i < 4; i++) {
        top.cx = (int16_t)(top.cx + (tx - top.cx) / 2);
        top.shear = (int8_t)(top.shear + (ts - top.shear) / 2);
        if (i == 3) { top.cx = tx; top.shear = ts; }
        card_draw();
        hal_delay(10);
    }
    ribbon(lean);
    effects_show(lean ? card + 7 + (lean > 0) * 2 : 0, RD8(card + 2));
    gfx_bg = bg_night;
}

/* ------------------------------------------------------------ choice */
static uint8_t cond_ok(uint8_t c)
{
    if (!c) return 1;
    uint8_t has = (uint8_t)((R.deck >> (c & 7)) & 1);
    return (c & COND_HAS) ? has : !has;
}

static uint8_t eligible(uint8_t i)
{
    const uint8_t *c = CARD(i);
    if ((RD8(c + 6) & CF_ONCE) && ((R.used[i >> 3] >> (i & 7)) & 1)) return 0;
    return RD8(c + 5) <= R.wave && cond_ok(RD8(c + 3)) && cond_ok(RD8(c + 4));
}

static uint8_t pick(void)
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < NCARDS; i++) n = (uint8_t)(n + eligible(i));
    if (!n) return 0xFF;
    uint8_t k = rnd(n);
    for (uint8_t i = 0; i < NCARDS; i++)
        if (eligible(i) && !k--) return i;
    return 0xFF;
}

static void deck_bar(void)
{
    gfx_bg = bg_ink;
    for (uint8_t u = 0; u < NUNITS; u++)
        sprite(u, 0, (uint8_t)(6 + u * 25), 3, (R.deck >> u) & 1 ? C_GOLD : C_EMPTY, 0, 2);
}

void perk_pick(void)
{
    uint8_t ci = pick();
    if (ci == 0xFF) return;
    R.used[ci >> 3] |= (uint8_t)(1 << (ci & 7));
    const uint8_t *card = CARD(ci);
    uint16_t off = RD16(card);
    uint8_t who = RD8(card + 2);
    HOOK(HOOK_CARD, ci);

    text_get(&off, qbuf, sizeof qbuf - 1);
    text_get(&off, lab[0], sizeof lab[0] - 3);
    text_get(&off, lab[1], sizeof lab[1] - 3);

    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    deck_bar();
    leds_run();
    prev_x0 = 200;
    prev_x1 = -1;
    gfx_bg = bg_night;
    card_flip_in(who);
    effects_show(0, who);
    gfx_bg = bg_night;
    gfx_wrap(1, TEXT_Y, 21, TEXT_N, LINE_H, qbuf, C_TEXT, 14, 0);

    HOOK(HOOK_SCREEN, 6);
    input_flush();
    int8_t lean = 0;
    for (;;) {
        uint8_t ev = input_poll();
        if (!ev) {
            hal_idle();
            continue;
        }
        uint8_t l = EV_LANE(ev);
        int8_t want = l == 0 ? -1 : l == 2 ? 1 : lean;
        if (lean && (want == lean)) break;          /* same side again, or POWER */
        lean = (int8_t)(lean && want != lean ? 0 : want);
        card_lean(lean, card);
    }
    ribbon(0);
    card_swipe(lean);
    const uint8_t *s = card + 7 + (lean > 0) * 2;
    fx_apply(RD8(s));
    fx_apply(RD8(s + 1));
    HOOK(HOOK_CHOICE, lean > 0);
}
