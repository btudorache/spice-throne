/* The siege: three lanes between Vulkar's gate (top) and your wall
 * (bottom). Units fight on their own; you choose what goes where and when
 * to wake the worm. */
#include "game.h"

#define HUD_H     11
#define GATE_Y0   12
#define GATE_Y1   17
#define TOP_Y     18      /* enemies enter here; your units raid past it   */
#define BOT_Y     103     /* your units enter here; enemies breach past it */
#define SPR       16      /* units are 8x8 sprites drawn at 2x            */
#define WALL_Y0   120
#define WALL_Y1   127
#define PANEL_Y   128
#define MAXU      28
#define WORM_FULL 16
#define FRAME_MS  40

/* ranges are measured between sprite tops: 16 = touching           */
/*                                       hp atk rng spd cd  $ wall air */
const UnitInfo unit_info[NUNITS + 1] PROGMEM = {
    {  14, 3, 18, 5,  8, 2,  6, 0 },   /* levy      */
    {  42, 4, 18, 3, 10, 4, 10, 0 },   /* trooper   */
    {  12, 4, 44, 4, 14, 3,  6, 0 },   /* rifleman  */
    {  26, 6, 18, 9,  8, 5, 12, 0 },   /* sandrider */
    {  22, 4, 36, 6, 12, 6, 10, 1 },   /* thopter   */
    { 150, 9, 20, 2, 12, 0, 40, 0 },   /* Vulkar's champion */
};
#define MELEE(range) ((range) <= 20)

static const uint8_t lane_x[3] = { 21, 64, 107 };

typedef struct {
    uint8_t type, side, lane, x, y, oy, mv, cd, alive, dirty, flash, dying;
    uint16_t hp, maxhp;
} Unit;

static Unit un[MAXU];
static uint8_t spice, spice_t, spice_flash, worm, hand[2];
static uint8_t budget, total, spawn_t, boss_due, gate_flash, wall_flash;
static uint8_t shown[6];   /* last drawn: spice, hand0, hand1, worm, hp, left */

/* ------------------------------------------------------------ shading */
/* sand ripples: 16 precomputed shades, so a background pixel costs a lookup */
static uint16_t sand_lut[16];

static void sand_init(void)
{
    for (uint8_t t = 0; t < 16; t++) sand_lut[t] = gfx_mix(RGB(212, 156, 96), RGB(176, 116, 68), (uint8_t)(t * 14), 1, 1);
}

static uint16_t sand(uint8_t x, uint8_t y)
{
    return sand_lut[(y + (RD8(dune_near + ((x * 2 + y) & 127)) >> 1)) & 15];
}

static uint16_t bg_field(uint8_t x, uint8_t y)
{
    if (y < GATE_Y0 || y >= PANEL_Y) return y == HUD_H ? C_LINE : C_INK;
    if (y <= GATE_Y1) {
        if (y == GATE_Y1 && ((x >> 1) & 1)) return RGB(30, 10, 12);   /* portcullis teeth */
        return ((x & 15) == 0) ? RGB(40, 14, 18) : RGB(84, 28, 32);
    }
    if (y >= WALL_Y0) {
        if (y < WALL_Y0 + 2 && ((x >> 2) & 1)) return sand(x, y);      /* crenellations */
        uint8_t row = (uint8_t)(y - WALL_Y0);
        if (row == 4 || ((x + (row > 4 ? 4 : 0)) & 7) == 0) return RGB(120, 80, 48);
        return RGB(200, 150, 92);
    }
    if (x == 42 || x == 85) return RGB(160, 104, 62);                  /* lane ridges */
    return sand(x, y);
}

/* ----------------------------------------------------------------- worm */
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

static void mouth_at(uint8_t cx, uint8_t cy, uint8_t r)
{
    wcx = cx;
    wcy = cy;
    wr = r;
    uint8_t x0 = cx > r ? (uint8_t)(cx - r) : 0, y0 = cy > r ? (uint8_t)(cy - r) : 0;
    uint8_t w = (uint8_t)(x0 + 2 * r + 1 > LCD_W ? LCD_W - x0 : 2 * r + 1);
    uint8_t h = (uint8_t)(y0 + 2 * r + 1 > LCD_H ? LCD_H - y0 : 2 * r + 1);
    gfx_shade(x0, y0, w, h, sh_worm);
}

/* A maw rises out of the sand at (cx, cy) and sinks back (title screen). */
void worm_mouth(uint8_t cx, uint8_t cy, shader_t bg)
{
    worm_bg = bg;
    for (uint8_t k = 0; k < 17; k++) {
        mouth_at(cx, cy, (uint8_t)(k < 8 ? 4 + k * 2 : 4 + (16 - k) * 2));
        hal_delay(40);
    }
    wr = 0;
    mouth_at(cx, cy, 20);
}

/* ----------------------------------------------------------------- LEDs */
static uint8_t led_m[4], led_l[4];

static void led(uint8_t i, uint8_t m, uint8_t l)
{
    if (led_m[i] == m && led_l[i] == l) return;
    led_m[i] = m;
    led_l[i] = l;
    led_set(i, m, l);
}

/* green: the wall in thirds (last one pulses); yellow: worm charge */
void leds_run(void)
{
    uint8_t lit = (uint8_t)((R.hp * 3 + R.hp_max - 1) / R.hp_max);
    for (uint8_t i = 0; i < 3; i++) led(i, i >= lit ? LM_OFF : lit == 1 ? LM_PULSE : LM_ON, 160);
    if (worm >= WORM_FULL) led(LED_YELLOW, LM_BLINK, 255);
    else led(LED_YELLOW, worm ? LM_ON : LM_OFF, (uint8_t)(worm * 10));
}

/* ---------------------------------------------------------------- units */
static uint8_t draw_card(void)
{
    uint8_t u;
    do u = rnd(NUNITS); while (!(R.deck & (1 << u)));
    return u;
}

static Unit *spawn(uint8_t type, uint8_t side, uint8_t lane)
{
    for (uint8_t i = 0; i < MAXU; i++) {
        Unit *u = &un[i];
        if (u->alive || u->dirty || u->dying) continue;
        memset(u, 0, sizeof *u);
        u->type = type;
        u->side = side;
        u->lane = lane;
        u->x = (uint8_t)(lane_x[lane] - 8 + rnd(21) - 10);   /* stays inside the lane */
        u->y = u->oy = side ? TOP_Y : BOT_Y;
        u->alive = 1;
        u->cd = rnd(6);
        uint16_t hp = UI8(type, hp);
        if (type == U_BOSS) hp = (uint16_t)(hp + R.wave * 12);
        if (side) hp = (uint16_t)((uint32_t)hp * (100 + 5 * R.wave + 10 * R.harder) / 100);
        else hp = (uint16_t)(hp * (4 + R.hp_up[type]) / 4);
        u->hp = u->maxhp = hp;
        return u;
    }
    return 0;
}

static void kill(Unit *t)
{
    t->alive = 0;
    t->dying = 3;
    if (t->side && worm < WORM_FULL) {
        worm = (uint8_t)(worm + 1 + R.worm_rate);
        if (worm > WORM_FULL) worm = WORM_FULL;
    }
}

static void hit(Unit *s, Unit *t)
{
    uint16_t d = UI8(s->type, atk);
    if (!s->side && s->type < NUNITS) d = (uint16_t)(d * (4 + R.atk_up[s->type]) / 4);
    if (s->type == U_RIFLE && UI8(t->type, air)) d *= 2;
    if (s->type == U_RIDER && t->type == U_RIFLE) d = (uint16_t)(d + d / 2);
    if (t->type == U_TROOPER && MELEE(UI8(s->type, range))) d = (uint16_t)(d - d * 3 / 10);
    if (!d) d = 1;
    t->flash = 2;
    if (t->hp > d) t->hp = (uint16_t)(t->hp - d);
    else kill(t);
}

static uint8_t enemies_alive(void)
{
    uint8_t c = 0;
    for (uint8_t i = 0; i < MAXU; i++) c = (uint8_t)(c + (un[i].alive && un[i].side));
    return c;
}

static void step(void)
{
    for (uint8_t i = 0; i < MAXU; i++) {
        Unit *s = &un[i];
        if (!s->alive) continue;
        if (s->cd) s->cd--;
        uint8_t range = UI8(s->type, range), best = 0xFF, bd = 255;
        for (uint8_t j = 0; j < MAXU; j++) {
            Unit *t = &un[j];
            if (!t->alive || t->side == s->side || t->lane != s->lane) continue;
            if (MELEE(range) && UI8(t->type, air) && !UI8(s->type, air)) continue;  /* melee can't reach air */
            int16_t d = s->side ? (int16_t)(t->y - s->y) : (int16_t)(s->y - t->y);
            if (d < -4) continue;                                                   /* behind */
            uint8_t ad = (uint8_t)(d < 0 ? -d : d);
            if (ad < bd) { bd = ad; best = j; }
        }
        if (best != 0xFF && bd <= range) {
            if (!s->cd) {
                hit(s, &un[best]);
                s->cd = UI8(s->type, cd);
            }
            continue;
        }
        s->mv = (uint8_t)(s->mv + UI8(s->type, speed));
        while (s->mv >= 8) {
            s->mv -= 8;
            s->y = (uint8_t)(s->y + (s->side ? 1 : -1));
        }
        if (!s->side && s->y <= TOP_Y) {                 /* raid the gate */
            s->alive = 0;
            s->dirty = 1;
            if (spice < 10) spice++;
            budget = budget > 2 ? (uint8_t)(budget - 2) : 0;
            gate_flash = 3;
        } else if (s->side && s->y >= BOT_Y) {          /* breach the wall */
            uint8_t w = UI8(s->type, wall);
            R.hp = R.hp > w ? (uint8_t)(R.hp - w) : 0;
            s->alive = 0;
            s->dirty = 1;
            wall_flash = 3;
        }
    }
}

/* Only the rows a unit vacated are repainted; the sprite itself is drawn
 * opaque over the sand, so the rest of its old footprint is overwritten. */
static void render(uint8_t frame)
{
    gfx_bg = bg_field;
    for (uint8_t i = 0; i < MAXU; i++) {
        Unit *u = &un[i];
        if (u->dirty) {
            gfx_clear(u->x, u->oy, SPR, SPR + 1);
            u->dirty = 0;
        } else if (u->alive && u->y < u->oy) {
            gfx_clear(u->x, (uint8_t)(u->y + SPR + 1), SPR, (uint8_t)(u->oy - u->y));
        } else if (u->alive && u->y > u->oy) {
            gfx_clear(u->x, u->oy, SPR, (uint8_t)(u->y - u->oy));
        }
    }
    for (uint8_t i = 0; i < MAXU; i++) {
        Unit *u = &un[i];
        uint8_t boss = u->type == U_BOSS, look = u->type;
        if (u->dying) {
            /* a white flash, a puff of sand, then gone */
            if (--u->dying) sprite(look, 1, u->x, u->oy, u->dying == 2 ? RGB(255, 255, 255) : RGB(255, 222, 170), u->side, 2);
            else gfx_clear(u->x, u->oy, SPR, SPR + 1);
            continue;
        }
        if (!u->alive) continue;
        uint8_t busy = u->y != u->oy || u->cd + 3 > UI8(u->type, cd);
        uint16_t team = u->flash ? RGB(255, 255, 255) : u->side ? (boss ? RGB(190, 60, 230) : C_RED) : C_GOLD;
        sprite(look, (uint8_t)(busy && (frame & 4)), u->x, u->y, team, u->side, 2);
        if (u->flash) u->flash--;
        uint8_t w = (uint8_t)((uint32_t)u->hp * SPR / u->maxhp);
        if (!w) w = 1;
        gfx_fill(u->x, (uint8_t)(u->y + SPR), w, 1, u->side ? C_RED : C_GREEN);
        if (w < SPR) gfx_fill((uint8_t)(u->x + w), (uint8_t)(u->y + SPR), (uint8_t)(SPR - w), 1, RGB(60, 30, 20));
        u->oy = u->y;
    }
}

/* the worm swallows everything on the ground in one lane, both sides */
static void worm_sweep(uint8_t lane)
{
    uint8_t cx = lane_x[lane];
    led_set(LED_YELLOW, LM_FLICKER, 255);
    worm_bg = bg_field;
    for (int16_t y = BOT_Y + 12; y >= TOP_Y + 8; y -= 5) {
        mouth_at(cx, (uint8_t)y, 13);
        hal_delay(20);
    }
    for (uint8_t i = 0; i < MAXU; i++) {
        Unit *u = &un[i];
        if (u->alive && u->lane == lane && !UI8(u->type, air)) {
            if (u->type == U_BOSS) {
                u->hp = u->hp > 80 ? (uint16_t)(u->hp - 80) : 1;   /* the champion survives, barely */
            } else {
                u->alive = 0;
                u->dirty = 1;
            }
        }
    }
    worm = 0;
    gfx_bg = bg_field;
    gfx_clear((uint8_t)(cx - 21), TOP_Y - 1, 42, WALL_Y0 - TOP_Y + 1);
    for (uint8_t i = 0; i < MAXU; i++) un[i].dirty = 0, un[i].oy = un[i].y;
    led_m[LED_YELLOW] = 0xFF;
}

/* ------------------------------------------------------------------ HUD */
static void hud(void)
{
    uint8_t left = (uint8_t)(budget + enemies_alive());
    gfx_bg = bg_ink;
    if (shown[5] != left) {
        shown[5] = left;
        uint8_t w = (uint8_t)((uint16_t)left * 60 / (total + 1));
        gfx_fill(64, 3, w, 4, C_RED);
        gfx_fill((uint8_t)(64 + w), 3, (uint8_t)(60 - w), 4, C_EMPTY);
    }
    if (shown[0] != spice || spice_flash) {
        shown[0] = spice;
        for (uint8_t i = 0; i < 10; i++)
            gfx_fill((uint8_t)(4 + i * 12), 131, 10, 5, i < spice ? C_GOLD : spice_flash ? C_RED : C_EMPTY);
        if (spice_flash) spice_flash--;
    }
    if (shown[1] != hand[0] || shown[2] != hand[1]) {
        shown[1] = hand[0];
        shown[2] = hand[1];
        gfx_fill(0, 139, 54, 20, C_INK);
        sprite(hand[0], 0, 2, 141, C_GOLD, 0, 2);
        say(20, 146, C_SAND, PSTR("%u"), R.cost[hand[0]]);
        sprite(hand[1], 0, 34, 141, RGB(110, 90, 70), 0, 2);
    }
    if (shown[3] != worm) {
        shown[3] = worm;
        uint8_t w = (uint8_t)(worm * 4);
        gfx_fill(58, 141, w, 4, worm >= WORM_FULL ? C_ORANGE : C_BLUE);
        gfx_fill((uint8_t)(58 + w), 141, (uint8_t)(64 - w), 4, C_EMPTY);
        leds_run();
    }
    if (shown[4] != R.hp) {
        shown[4] = R.hp;
        gfx_fill(58, 149, 68, 8, C_INK);
        say(58, 149, R.hp * 3 < R.hp_max ? C_RED : C_SAND, PSTR("WALL %u"), R.hp);
        leds_run();
    }
}

static void banner(const char *a_P, const char *b_P, uint16_t fg)
{
    gfx_bg = bg_field;
    gfx_clear(0, 52, LCD_W, 30);
    if (!a_P) return;
    char b[16];
    gfx_text_c(54, fmt(b, a_P, R.wave), fg, RGB(40, 14, 10), 2);
    if (b_P) say(SAY_C, 74, C_INK, b_P);
}

/* --------------------------------------------------------------- a wave */
/* which button feeds which lane, carved into the wall */
static void wall_labels(void)
{
    static const char keys[3] = { 'L', 'P', 'R' };
    gfx_bg = bg_field;
    for (uint8_t l = 0; l < 3; l++) gfx_char((uint8_t)(lane_x[l] - 2), WALL_Y0 + 1, keys[l], C_INK, NO_SHADOW, 1);
}

static void deploy(uint8_t lane)
{
    uint8_t u = hand[0];
    if (spice < R.cost[u]) {
        spice_flash = 4;
        return;
    }
    if (!spawn(u, 0, lane)) return;
    spice = (uint8_t)(spice - R.cost[u]);
    hand[0] = hand[1];
    hand[1] = draw_card();
}

static void enemy_spawn(void)
{
    if (!budget) return;
    uint8_t w = R.wave, type;
    if (boss_due && budget <= total / 2) {
        type = U_BOSS;
        boss_due = 0;
        budget = budget > 8 ? (uint8_t)(budget - 8) : 0;
    } else {
        uint8_t allowed = (uint8_t)(1 | (w >= 2) << 1 | (w >= 3) << 2 | (w >= 5) << 3 | (w >= 7) << 4);
        do type = rnd(NUNITS); while (!(allowed & (1 << type)) || UI8(type, cost) > budget + 1);
        budget = budget > UI8(type, cost) ? (uint8_t)(budget - UI8(type, cost)) : 0;
    }
    /* a third of the time, hit the lane you defend least */
    uint8_t lane = rnd(3);
    if (rnd(3) == 0) {
        uint8_t cnt[3] = { 0, 0, 0 };
        for (uint8_t i = 0; i < MAXU; i++) if (un[i].alive && !un[i].side) cnt[un[i].lane]++;
        lane = cnt[0] <= cnt[1] ? (cnt[0] <= cnt[2] ? 0 : 2) : (cnt[1] <= cnt[2] ? 1 : 2);
    }
    spawn(type, 1, lane);
    spawn_t = (uint8_t)((w > 18 ? 18 : 56 - w * 2) + rnd(24));
}

uint8_t field_wave(void)
{
    memset(un, 0, sizeof un);
    sand_init();
    memset(shown, 0xFF, sizeof shown);
    memset(led_m, 0xFF, sizeof led_m);
    if (R.wave == 1) worm = 0;
    if (R.worm_ready) { worm = WORM_FULL; R.worm_ready = 0; }
    total = budget = (uint8_t)(6 + R.wave * 3 > 250 ? 250 : 6 + R.wave * 3);
    boss_due = R.wave % 5 == 0;
    spawn_t = 50;
    spice = (uint8_t)(5 + R.spice_bonus > 10 ? 10 : 5 + R.spice_bonus);
    spice_t = 0;
    hand[0] = draw_card();
    hand[1] = draw_card();

    gfx_bg = bg_field;
    gfx_clear(0, 0, LCD_W, LCD_H);
    wall_labels();
    gfx_bg = bg_ink;
    say(2, 2, C_ORANGE, PSTR("WAVE %u"), R.wave);
    hud();
    banner(PSTR("WAVE %u"), R.wave == 1 ? PSTR("L  POWER  R : deploy") : boss_due ? PSTR("Vulkar's champion!") : 0,
           C_ORANGE);
    if (R.wave == 1) say(SAY_C, 86, C_INK, PSTR("hold: unleash worm"));
    HOOK(HOOK_SCREEN, 20);
    hal_delay(R.wave == 1 ? 2500 : 1300);
    banner(0, 0, 0);
    if (R.wave == 1) { gfx_bg = bg_field; gfx_clear(0, 82, LCD_W, 12); }
    input_flush();

    for (uint16_t frame = 0;; frame++) {
        uint16_t t0 = hal_ms();
        uint8_t ev = input_poll();
        if (ev & EV_HOLD && worm >= WORM_FULL) worm_sweep(EV_LANE(ev));
        else if (ev) deploy(EV_LANE(ev));

        uint8_t regen = (uint8_t)(36 - 4 * (R.income > 5 ? 5 : R.income));
        if (++spice_t >= regen) {
            spice_t = 0;
            if (spice < 10) spice++;
        }
        if (!spawn_t || !--spawn_t) enemy_spawn();

        step();
        render((uint8_t)frame);

        gfx_bg = bg_field;
        if (gate_flash) {
            if (--gate_flash) gfx_fill(0, GATE_Y0, LCD_W, GATE_Y1 - GATE_Y0 + 1, C_GOLD);
            else gfx_clear(0, GATE_Y0, LCD_W, GATE_Y1 - GATE_Y0 + 1);
        }
        if (wall_flash) {
            if (--wall_flash) gfx_fill(0, WALL_Y0, LCD_W, WALL_Y1 - WALL_Y0 + 1, C_RED);
            else { gfx_clear(0, WALL_Y0, LCD_W, WALL_Y1 - WALL_Y0 + 1); wall_labels(); }
        }
        hud();

        if (!R.hp) {
            for (uint8_t k = 0; k < 6; k++) {
                gfx_fill(0, WALL_Y0, LCD_W, 8, (k & 1) ? C_RED : C_INK);
                hal_delay(150);
            }
            banner(PSTR("BREACHED"), PSTR("the wall has fallen"), C_RED);
            hal_delay(1500);
            return 0;
        }
        if (!budget && !enemies_alive()) {
            R.hp = (uint8_t)(R.hp + 5 > R.hp_max ? R.hp_max : R.hp + 5);
            banner(PSTR("WAVE %u"), PSTR("the wall holds!"), C_GREEN);
            HOOK(HOOK_WAVE_END, R.wave);
            hal_delay(1500);
            return 1;
        }
        if ((frame & 31) == 0) HOOK(HOOK_FRAME, (uint16_t)(hal_ms() - t0));
        while ((uint16_t)(hal_ms() - t0) < FRAME_MS) hal_idle();
    }
}

#ifndef __AVR__
/* simulator bot helpers */
uint8_t sim_spice_ok(void) { return spice >= R.cost[hand[0]]; }
uint8_t sim_worm_ready(void) { return worm >= WORM_FULL; }
/* how close the nearest enemy is to the wall in a lane (0 = none) */
uint8_t sim_threat(uint8_t lane)
{
    uint8_t m = 0, mine = 0;
    for (uint8_t i = 0; i < MAXU; i++) {
        if (!un[i].alive || un[i].lane != lane) continue;
        if (un[i].side && un[i].y > m) m = un[i].y;
        if (!un[i].side) mine++;
    }
    return m ? (uint8_t)(m + 20 - (mine > 4 ? 20 : mine * 5)) : 0;
}
#endif
