/* Autobattler: armies are split into squads that march, shoot and brawl
 * on their own. The player can only hold/charge and call the worm. */
#include "game.h"

#define MAXSQ 24
#define FIELD_Y0 20
#define FIELD_Y1 139

typedef struct {
    uint8_t x, y, ox, oy;
    uint16_t hp, maxhp;
    uint8_t type, side, n, cd, mv, dirty;
    uint8_t uhp, atk, range, speed, cdmax;   /* copied from unit_info */
} Sq;

static Sq sq[MAXSQ];
static uint8_t nsq;

/* damage multipliers in tenths: [attacker][defender] */
static const uint8_t dmg_mult[NUNITS][NUNITS] PROGMEM = {
    { 10, 7, 12, 9, 4 },
    { 12, 10, 11, 9, 4 },
    { 10, 8, 10, 13, 15 },
    { 12, 8, 15, 10, 4 },
    { 12, 11, 12, 12, 10 },
};

static uint16_t bg_sand(uint8_t x, uint8_t y)
{
    if (y < FIELD_Y0 || y > FIELD_Y1) return C_INK;
    uint8_t t = (uint8_t)((y + (RD8(dune_near + ((x + (y >> 2)) & 127)) >> 1)) & 15);
    return gfx_mix(RGB(206, 150, 92), RGB(170, 110, 64), (uint8_t)(t * 14), x, y);
}

/* ---------------------------------------------------------------- worm */
static uint8_t wcx, wcy, wr;
static shader_t worm_bg;

static uint16_t sh_worm(uint8_t x, uint8_t y)
{
    int16_t dx = (int16_t)x - wcx, dy = (int16_t)y - wcy;
    int16_t d2 = (int16_t)(dx * dx + dy * dy), r = wr;
    if (d2 > r * r) return worm_bg(x, y);
    if (d2 <= (r - 4) * (r - 4)) return RGB(36, 8, 14);
    if (d2 <= (r - 2) * (r - 2)) return ((x + y) & 1) ? RGB(250, 246, 230) : RGB(120, 30, 60);
    return (d2 > (r - 1) * (r - 1)) ? RGB(80, 42, 24) : RGB(200, 146, 86);
}

/* A maw rises out of the sand at (cx, cy) and sinks back. */
void worm_mouth(uint8_t cx, uint8_t cy, shader_t bg)
{
    wcx = cx;
    wcy = cy;
    worm_bg = bg;
    uint8_t x0 = cx > 20 ? (uint8_t)(cx - 20) : 0, y0 = cy > 20 ? (uint8_t)(cy - 20) : 0;
    uint8_t w = (uint8_t)(x0 + 41 > LCD_W ? LCD_W - x0 : 41), h = (uint8_t)(y0 + 41 > LCD_H ? LCD_H - y0 : 41);
    for (uint8_t k = 0; k < 17; k++) {
        wr = (uint8_t)(k < 8 ? 4 + k * 2 : 4 + (16 - k) * 2);
        gfx_shade(x0, y0, w, h, sh_worm);
        hal_delay(40);
    }
    gfx_shade(x0, y0, w, h, bg);
}

/* ------------------------------------------------------------- squads */
static void make_squads(const uint8_t *u, uint8_t side)
{
    static const uint8_t ys[2][3] = { { 106, 118, 128 }, { 44, 32, 22 } };
    uint16_t total = army_total(u);
    if (!total) return;
    uint8_t per = (uint8_t)((total + 6) / 7);
    uint8_t first = nsq;
    for (uint8_t t = 0; t < NUNITS; t++) {
        uint8_t c = u[t];
        while (c && nsq < MAXSQ) {
            uint8_t k = c < per ? c : per;
            Sq *s = &sq[nsq++];
            memset(s, 0, sizeof *s);
            s->type = t;
            s->side = side;
            s->n = k;
            memcpy_P(&s->uhp, &unit_info[t], 5);
            s->hp = s->maxhp = (uint16_t)k * s->uhp;
            s->cd = rnd(8);
            c = (uint8_t)(c - k);
        }
    }
    /* rows: melee front, rifles behind, thopters at the back */
    for (uint8_t row = 0; row < 3; row++) {
        uint8_t cnt = 0, i = 0;
        for (uint8_t pass = 0; pass < 2; pass++)
            for (uint8_t j = first; j < nsq; j++) {
                Sq *s = &sq[j];
                uint8_t r = s->type == U_RIFLE ? 1 : s->type == U_THOPTER ? 2 : 0;
                if (r != row) continue;
                if (!pass) { cnt++; continue; }
                s->x = s->ox = (uint8_t)(4 + (uint16_t)112 * (2 * i + 1) / (2 * cnt));
                s->y = s->oy = ys[side][row];
                s->dirty = 1;
                i++;
            }
    }
}

static uint8_t alive(uint8_t side)
{
    uint8_t c = 0;
    for (uint8_t i = 0; i < nsq; i++)
        if (sq[i].side == side) c = (uint8_t)(c + sq[i].n);
    return c;
}

static uint8_t adist(uint8_t a, uint8_t b) { return a > b ? (uint8_t)(a - b) : (uint8_t)(b - a); }

static void hurt(Sq *t, uint16_t dmg)
{
    uint8_t hp = t->uhp;
    t->hp = t->hp > dmg ? (uint16_t)(t->hp - dmg) : 0;
    t->n = (uint8_t)((t->hp + hp - 1) / hp);
    if (!t->n) t->dirty = 1;
}

/* one simulation frame */
static void step(const uint8_t morale[2], uint8_t wall_side, uint8_t wall, uint8_t tactic)
{
    for (uint8_t i = 0; i < nsq; i++) {
        Sq *s = &sq[i];
        if (!s->n) continue;
        if (s->cd) s->cd--;
        uint8_t best = NONE, bd = 255;
        for (uint8_t j = 0; j < nsq; j++) {
            if (!sq[j].n || sq[j].side == s->side) continue;
            uint16_t d = (uint16_t)adist(s->x, sq[j].x) + adist(s->y, sq[j].y);
            if (d < bd) { bd = d > 254 ? 254 : (uint8_t)d; best = j; }
        }
        if (best == NONE) return;
        Sq *t = &sq[best];
        if (bd <= s->range + 2) {
            if (!s->cd) {
                uint16_t dmg = (uint16_t)s->n * s->atk * RD8(&dmg_mult[s->type][t->type]) / 10;
                dmg = dmg < 500 ? (uint16_t)(dmg * morale[s->side] / 100) : (uint16_t)((dmg >> 2) * morale[s->side] / 25);
                dmg = (uint16_t)(dmg + (dmg >> 3) - (uint16_t)((dmg >> 2) * rnd(2)));
                if (t->side == wall_side) dmg = (uint16_t)(dmg - (dmg >> 3) * (wall > 6 ? 6 : wall));   /* -12.5% per wall level */
                hurt(t, dmg ? dmg : 1);
                s->cd = s->cdmax;
            }
            continue;
        }
        if (s->side == 0 && tactic == 1 && bd > 28) continue;   /* hold the line */
        s->mv = (uint8_t)(s->mv + s->speed + (s->side == 0 && tactic == 2 ? 2 : 0));
        while (s->mv >= 4) {
            s->mv -= 4;
            if (adist(s->y, t->y) > adist(s->x, t->x) / 2 + 2) s->y = (uint8_t)(s->y + (t->y > s->y ? 1 : -1));
            else if (s->x != t->x) s->x = (uint8_t)(s->x + (t->x > s->x ? 1 : -1));
        }
        if (s->y < FIELD_Y0 + 1) s->y = FIELD_Y0 + 1;
        if (s->y > FIELD_Y1 - 9) s->y = FIELD_Y1 - 9;
    }
}

static void render(uint8_t frame, uint16_t team1)
{
    for (uint8_t i = 0; i < nsq; i++) {
        Sq *s = &sq[i];
        if (s->dirty || s->x != s->ox || s->y != s->oy) {
            gfx_clear(s->ox, s->oy, 8, 9);
            s->dirty = 0;
        }
    }
    for (uint8_t i = 0; i < nsq; i++) {
        Sq *s = &sq[i];
        if (!s->n) continue;
        uint8_t busy = s->x != s->ox || s->y != s->oy || s->cd + 3 > s->cdmax;
        sprite(s->type, (uint8_t)(busy && (frame & 4)), s->x, s->y, s->side ? team1 : C_GOLD, s->side, 1);
        uint8_t w = (uint8_t)(s->hp * 8 / s->maxhp);
        if (!w) w = 1;
        gfx_fill(s->x, (uint8_t)(s->y + 8), w, 1, s->side ? C_RED : C_GREEN);
        if (w < 8) gfx_fill((uint8_t)(s->x + w), (uint8_t)(s->y + 8), (uint8_t)(8 - w), 1, RGB(60, 30, 20));
        s->ox = s->x;
        s->oy = s->y;
    }
}

static void worm_strike(void)
{
    uint8_t best = NONE, bc = 0;
    for (uint8_t i = 0; i < nsq; i++) {
        if (!sq[i].n || sq[i].side != 1) continue;
        uint8_t c = 0;
        for (uint8_t j = 0; j < nsq; j++)
            if (sq[j].n && adist(sq[i].x, sq[j].x) < 16 && adist(sq[i].y, sq[j].y) < 16) c++;
        if (c > bc) { bc = c; best = i; }
    }
    if (best == NONE) return;
    G.charges--;
    led_set(LED_YELLOW, LM_FLICKER, 255);
    uint8_t cx = (uint8_t)(sq[best].x + 4), cy = (uint8_t)(sq[best].y + 4);
    worm_mouth(cx, cy < FIELD_Y0 + 20 ? FIELD_Y0 + 20 : cy, bg_sand);
    for (uint8_t j = 0; j < nsq; j++) {
        Sq *s = &sq[j];
        s->dirty = 1;
        if (s->n && s->type != U_THOPTER && adist((uint8_t)(s->x + 4), cx) < 18 && adist((uint8_t)(s->y + 4), cy) < 18)
            hurt(s, (uint16_t)(s->maxhp * 7 / 10 + 1));
    }
}

static void counts(void)
{
    gfx_bg = bg_ink;
    gfx_fill(0, 10, LCD_W, 9, C_INK);
    say(SAY_C, 11, C_TEXT, PSTR("%u vs %u"), alive(0), alive(1));
    gfx_bg = bg_sand;
}

static void footer_P(const char *s_P, uint16_t c)
{
    gfx_bg = bg_ink;
    gfx_fill(0, 141, LCD_W, 19, C_INK);
    say(SAY_C, 146, c, s_P);
    gfx_bg = bg_sand;
}

uint8_t battle(uint8_t *att, uint8_t att_owner, uint8_t *def, uint8_t def_owner,
               uint8_t node, uint8_t wall, uint8_t animate)
{
#ifndef __AVR__
    extern uint8_t sim_fast;
    if (sim_fast) animate = 0;
#endif
    if (!army_total(def)) return 1;
    if (!army_total(att)) return 0;

    /* side 0 is drawn at the bottom: the player's army when animating */
    uint8_t att_side = (animate && def_owner == OWN_PLAYER) ? 1 : 0;
    uint8_t *side_u[2];
    side_u[att_side] = att;
    side_u[!att_side] = def;
    uint8_t owners[2];
    owners[att_side] = att_owner;
    owners[!att_side] = def_owner;
    uint16_t before[2] = { army_total(side_u[0]), army_total(side_u[1]) };
    uint8_t morale[2];
    for (uint8_t s = 0; s < 2; s++)
        morale[s] = (uint8_t)(owners[s] == OWN_PLAYER ? 70 + G.meter[M_LEGION] * 6 / 10 : 100);

    nsq = 0;
    make_squads(side_u[0], 0);
    make_squads(side_u[1], 1);
    uint16_t team1 = owner_color(owners[1]);
    uint8_t tactic = 0;

    if (animate) {
        HOOK(HOOK_SCREEN, 7);
        for (uint8_t i = 0; i < 3; i++) led_set(i, LM_BLINK, 255);
        gfx_bg = bg_ink;
        gfx_fill(0, 0, LCD_W, FIELD_Y0, C_INK);
        say(SAY_C, 1, C_ORANGE, PSTR("BATTLE: %N"), node);
        footer_P(PSTR("L hold R charge P worm"), C_DIM);
        gfx_clear(0, FIELD_Y0, LCD_W, FIELD_Y1 - FIELD_Y0 + 1);
    }

    for (uint16_t frame = 0; frame < 1500; frame++) {
        if (animate) {
            uint8_t ev = input_poll();
            if (ev == EV_L || ev == EV_R) {
                tactic = ev == EV_L ? 1 : 2;
                footer_P(tactic == 1 ? PSTR("- holding the line -") : PSTR("- CHARGE! -"), C_SAND);
            } else if (ev == EV_P && G.charges && owners[0] == OWN_PLAYER) {
                worm_strike();
                leds_world();
            }
        }
        step(morale, (uint8_t)!att_side, wall, tactic);
        if (animate) {
            render((uint8_t)frame, team1);
            if ((frame & 15) == 0) counts();
            hal_delay(35);
        }
        if (!alive(0) || !alive(1)) break;
    }

    memset(side_u[0], 0, NUNITS);
    memset(side_u[1], 0, NUNITS);
    for (uint8_t i = 0; i < nsq; i++) side_u[sq[i].side][sq[i].type] = (uint8_t)(side_u[sq[i].side][sq[i].type] + sq[i].n);
    /* a battle that times out is a win for the defender */
    uint8_t att_won = alive((uint8_t)!att_side) == 0 && alive(att_side) > 0;

    if (animate) {
        char b[10];
        uint8_t won = att_side == 0 ? att_won : !att_won;
        hal_delay(500);
        gfx_bg = bg_ink;
        gfx_fill(0, 141, LCD_W, 19, C_INK);
        gfx_fill(14, 56, 100, 48, C_PANEL);
        gfx_fill(16, 58, 96, 44, C_INK);
        gfx_text_c(62, fmt(b, won ? PSTR("VICTORY") : PSTR("DEFEAT")), won ? C_GREEN : C_RED, NO_SHADOW, 2);
        say(SAY_C, 82, C_GOLD, PSTR("ours %u -> %u"), before[0], alive(0));
        say(SAY_C, 92, team1, PSTR("theirs %u -> %u"), before[1], alive(1));
        leds_world();
        HOOK(HOOK_SCREEN, 8);
        while (input_poll() != EV_P) hal_idle();
    }
    return att_won;
}
