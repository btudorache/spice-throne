/* Main game screen: resource HUD, four tabs (FIEF, ARMY, MAP, HALL),
 * status line, three-button navigation and the real-time loop.
 *
 * Controls: LEFT/RIGHT move; POWER enters/acts; holding POWER goes back. */
#include <stdarg.h>
#include "game.h"

#define TAB_Y    10
#define VIEW_Y   22
#define PANEL_Y  128
#define STATUS_Y 151

enum { T_FIEF, T_ARMY, T_MAP, NTABS };
enum { MD_BROWSE, MD_QTY, MD_ACTION, MD_PCT };

static uint8_t tab, focus, mode;
static uint8_t sel[3];
static uint8_t qty, act, pct_i;
static const char *msg_P;
static uint16_t msg_c, msg_t;
static uint16_t hud_last[NRES];
static uint8_t dot_x[NMARCH], dot_y[NMARCH];

/* ------------------------------------------------------------- input */
static uint16_t p_down, rep_t;
static uint8_t p_state, rep_btn;

uint8_t input_poll(void)
{
    uint8_t e = hal_btn_take(), h = hal_btn_held();
    uint16_t now = hal_ms();
    if (e & (BTN_LEFT | BTN_RIGHT)) {
        rep_btn = (uint8_t)(e & BTN_LEFT ? BTN_LEFT : BTN_RIGHT);
        rep_t = (uint16_t)(now + 400);
        return rep_btn == BTN_LEFT ? EV_L : EV_R;
    }
    if (rep_btn) {
        if (!(h & rep_btn)) rep_btn = 0;
        else if ((int16_t)(now - rep_t) >= 0) {
            rep_t = (uint16_t)(now + 110);
            return rep_btn == BTN_LEFT ? EV_L : EV_R;
        }
    }
    if (e & BTN_POWER) { p_state = 1; p_down = now; }
    if (p_state == 1) {
        if (!(h & BTN_POWER)) { p_state = 0; return EV_P; }
        if ((uint16_t)(now - p_down) >= 450) { p_state = 2; return EV_HOLD; }
    } else if (p_state == 2 && !(h & BTN_POWER)) {
        p_state = 0;
    }
    return EV_NONE;
}

/* ------------------------------------------------------------ strings */
static const char names_tbl[] PROGMEM =
    "Aren\0Ilya\0Corin\0Dara\0Ezran\0Mira\0Talen\0Sora\0Vashti\0Orin\0Nyla\0Rhun\0Isel\0Joren\0Kira\0Lazar";
static const char houses_tbl[] PROGMEM =
    "Vantor\0Merrow\0Castell\0Korvath\0Sarkand\0Ilmaren\0Tessar\0Halloran";
static const char nodes_tbl[] PROGMEM =
    "Citadel\0Ember Flats\0Glass Basin\0Sietch Rhu\0Deep Erg\0Sietch Talun\0"
    "Vulkar Watch\0Ordane Spire\0Vulkar Keep\0Ordane Hold";
static const char units_tbl[] PROGMEM = "Levy\0Trooper\0Rifleman\0Sandrider\0Thopter";
static const char blds_tbl[] PROGMEM =
    "Keep\0Refinery\0Windtrap\0Foundry\0Warehouse\0Barracks\0Shield Wall\0Thopter Pad";
static const char owners_tbl[] PROGMEM = "free\0yours\0Vulkar\0Ordane\0tribes";
static const char tabs_tbl[] PROGMEM = "FIEF\0ARMY\0MAP";

static uint8_t copy_nth(char *dst, const char *tbl, uint8_t n)
{
    while (n) if (RD8(tbl++) == 0) n--;
    uint8_t k = 0;
    char c;
    while ((c = (char)RD8(tbl++)) != 0) dst[k++] = c;
    dst[k] = 0;
    return k;
}

uint8_t name_ruler(char *dst) { return copy_nth(dst, names_tbl, G.name); }
uint8_t name_house(char *dst) { return copy_nth(dst, houses_tbl, G.house); }

/* printf-lite over PROGMEM formats:
 *   %u unsigned  %t m:ss  %s RAM string  %S flash string
 *   %N node  %U unit  %B building  %O owner  %T tab  %H house */
static char *vfmt(char *b, const char *f, va_list ap)
{
    char c;
    while ((c = (char)RD8(f++)) != 0) {
        if (c != '%') { *b++ = c; continue; }
        c = (char)RD8(f++);
        if (c == '%') {
            *b++ = '%';
        } else if (c == 'u' || c == 't') {
            unsigned v = va_arg(ap, unsigned);
            unsigned lead = c == 't' ? v / 60 : v;
            char t[6], *p = t + 5;
            *p = 0;
            do { *--p = (char)('0' + lead % 10); lead /= 10; } while (lead);
            while (*p) *b++ = *p++;
            if (c == 't') {
                *b++ = ':';
                *b++ = (char)('0' + (v % 60) / 10);
                *b++ = (char)('0' + v % 10);
            }
        } else if (c == 's' || c == 'S') {
            const char *s = va_arg(ap, const char *);
            char d;
            while ((d = c == 's' ? *s : (char)RD8(s)) != 0) { *b++ = d; s++; }
        } else {
            static const char *const tbls[] = { nodes_tbl, units_tbl, blds_tbl, owners_tbl, tabs_tbl, houses_tbl };
            static const char keys[] = "NUBOTH";
            uint8_t n = (uint8_t)va_arg(ap, int);
            b += copy_nth(b, tbls[strchr(keys, c) - keys], n);
        }
    }
    *b = 0;
    return b;
}

char *fmt(char *b, const char *f_P, ...)
{
    va_list ap;
    va_start(ap, f_P);
    vfmt(b, f_P, ap);
    va_end(ap);
    return b;
}

/* draw formatted text at x (SAY_C centred, SAY_R right-aligned) */
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

void node_name(uint8_t n, char *b) { copy_nth(b, nodes_tbl, n); }
void unit_name(uint8_t u, char *b) { copy_nth(b, units_tbl, u); }

uint16_t bg_ink(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_INK;
}

static uint16_t bg_panel(uint8_t x, uint8_t y)
{
    (void)x; (void)y;
    return C_PANEL;
}

uint16_t owner_color(uint8_t o)
{
    static const uint16_t c[5] = { RGB(150, 140, 130), C_GOLD, C_RED, C_PURPLE, C_BLUE };
    return c[o];
}

/* ------------------------------------------------------------ sprites */
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

/* 1-bit icon of w (<=16) columns, MSB first, one or two bytes per row */
static void icon(const uint8_t *p, uint8_t w, uint8_t h, uint8_t x, uint8_t y, uint16_t fg)
{
    uint8_t bpr = w > 8 ? 2 : 1;
    lcd_window(x, y, (uint8_t)(x + w - 1), (uint8_t)(y + h - 1));
    for (uint8_t j = 0; j < h; j++) {
        uint16_t bits = (uint16_t)(RD8(p) << 8);
        if (bpr == 2) bits |= RD8(p + 1);
        p += bpr;
        for (uint8_t i = 0; i < w; i++) {
            lcd_px((bits & 0x8000) ? fg : gfx_bg((uint8_t)(x + i), (uint8_t)(y + j)));
            bits <<= 1;
        }
    }
}

/* ---------------------------------------------------------------- LEDs */
static uint8_t led_mode[4], led_lvl[4];

static void led(uint8_t i, uint8_t m, uint8_t lvl)
{
    if (led_mode[i] == m && led_lvl[i] == lvl) return;
    led_mode[i] = m;
    led_lvl[i] = lvl;
    led_set(i, m, lvl);
}

/* green: attack countdown (or territories held); yellow: petition / worm charges */
static void leds_update(void)
{
    uint16_t eta, tot;
    uint8_t who, where;
    if (threat(&eta, &tot, &who, &where)) {
        uint8_t lit = (uint8_t)((eta * 3 + tot - 1) / tot);
        for (uint8_t i = 0; i < 3; i++) led(i, i < lit ? (eta < 10 ? LM_BLINK : LM_PULSE) : LM_OFF, 255);
    } else {
        uint8_t held = 0;
        for (uint8_t n = 1; n < NNODES; n++) if (G.node[n].owner == OWN_PLAYER) held++;
        for (uint8_t i = 0; i < 3; i++) led(i, i < held ? LM_ON : LM_OFF, 30);
    }
    static const uint8_t ylvl[4] = { 0, 18, 70, 255 };
    if (G.pending) led(LED_YELLOW, LM_BLINK, 200);
    else if (G.charges >= 3) led(LED_YELLOW, LM_BREATHE, 255);
    else led(LED_YELLOW, G.charges ? LM_ON : LM_OFF, ylvl[G.charges]);
}

/* after other screens drove the LEDs directly */
void leds_world(void)
{
    memset(led_mode, 0xFF, sizeof led_mode);
    leds_update();
}

/* ------------------------------------------------------- frame pieces */
void status(const char *s_P, uint16_t fg)
{
    msg_P = s_P;
    msg_c = fg;
    msg_t = G.secs;
}

static void hud(uint8_t force)
{
    static const uint16_t col[NRES] = { C_ORANGE, C_BLUE, RGB(200, 200, 210) };
    gfx_bg = bg_ink;
    for (uint8_t r = 0; r < NRES; r++) {
        if (!force && hud_last[r] == G.res[r]) continue;
        hud_last[r] = G.res[r];
        uint8_t x = (uint8_t)(r * 43);
        icon(res_icons + r * 7, 7, 7, (uint8_t)(x + 1), 1, col[r]);
        gfx_fill((uint8_t)(x + 9), 0, 32, 9, C_INK);
        say((uint8_t)(x + 9), 1, G.res[r] >= world_cap() ? C_RED : C_TEXT, PSTR("%u"), G.res[r]);
    }
}

static void tabs(void)
{
    for (uint8_t t = 0; t < NTABS; t++) {
        uint8_t x = (uint8_t)(t * 43), on = t == tab;
        gfx_fill(x, TAB_Y, 42, 11, on ? C_PANEL : C_INK);
        if (on) gfx_fill((uint8_t)(x + 2), TAB_Y + 10, 38, 1, focus ? C_LINE : C_ORANGE);
        gfx_bg = on ? bg_panel : bg_ink;
        say((uint8_t)(x + (t == T_MAP ? 12 : 9)), TAB_Y + 1, on ? C_ORANGE : C_DIM, PSTR("%T"), t);
    }
    gfx_bg = bg_ink;
}

static void status_line(void)
{
    uint16_t eta, tot;
    uint8_t who, where;
    gfx_bg = bg_ink;
    gfx_fill(0, STATUS_Y - 1, LCD_W, 9, C_INK);
    if (threat(&eta, &tot, &who, &where))
        say(SAY_C, STATUS_Y, (eta & 1) ? C_RED : C_ORANGE, PSTR("! %N %t"), where, eta);
    else if (msg_P && (uint16_t)(G.secs - msg_t) < 4)
        say(SAY_C, STATUS_Y, msg_c, PSTR("%S"), msg_P);
    else if (G.build_id != NONE)
        say(SAY_C, STATUS_Y, C_SAND, PSTR("%B L%u %t"), G.build_id, G.bld[G.build_id] + 1, G.build_left);
    else
        say(SAY_C, STATUS_Y, C_DIM, PSTR("Day %u  worms %u"), G.secs / DAY_SECS + 1, G.charges);
}

static void panel_clear(void)
{
    gfx_bg = bg_ink;
    gfx_fill(0, PANEL_Y, LCD_W, STATUS_Y - 2 - PANEL_Y, C_INK);
}

#define ROW0 (PANEL_Y + 2)
#define ROW1 (PANEL_Y + 12)

/* ---------------------------------------------------------------- FIEF */
static const char s_busy[] PROGMEM = "Builders are busy";
static const char s_nores[] PROGMEM = "Not enough resources";
static const char s_maxed[] PROGMEM = "Upgrade the Keep first";
static const char s_started[] PROGMEM = "Construction started";

static void fief_tile(uint8_t b)
{
    uint8_t x = (uint8_t)(2 + (b & 1) * 63), y = (uint8_t)(VIEW_Y + 2 + (b >> 1) * 26);
    gfx_fill(x, y, 61, 24, focus && sel[T_FIEF] == b ? C_ORANGE : C_LINE);
    gfx_fill((uint8_t)(x + 1), (uint8_t)(y + 1), 59, 22, C_PANEL);
    gfx_bg = bg_panel;
    icon(bld_icons + b * 32, 16, 16, (uint8_t)(x + 4), (uint8_t)(y + 4), G.bld[b] ? C_SAND : C_EMPTY);
    say((uint8_t)(x + 24), (uint8_t)(y + 4), C_TEXT, PSTR("L%u"), G.bld[b]);
    if (G.build_id == b) {
        say((uint8_t)(x + 24), (uint8_t)(y + 13), C_ORANGE, PSTR("%t"), G.build_left);
    } else if (G.bld[b] >= build_max(b)) {
        say((uint8_t)(x + 24), (uint8_t)(y + 13), C_DIM, PSTR("MAX"));
    }
    gfx_bg = bg_ink;
}

static void fief_panel(void)
{
    uint8_t s = sel[T_FIEF];
    panel_clear();
    if (G.bld[s] >= build_max(s)) {
        say(SAY_C, ROW0, C_ORANGE, PSTR("%B"), s);
        say(SAY_C, ROW1, C_DIM, G.bld[s] >= (s == B_KEEP ? 7 : 9) ? PSTR("MAX") : s_maxed);
        return;
    }
    uint16_t cs, cp, ct;
    build_cost(s, &cs, &cp, &ct);
    say(SAY_C, ROW0, C_ORANGE, PSTR("%B L%u"), s, G.bld[s] + 1);
    say(SAY_C, ROW1, (G.res[R_SPICE] >= cs && G.res[R_STEEL] >= cp) ? C_TEXT : C_RED,
        PSTR("S%u P%u %t"), cs, cp, ct);
}

static void fief_draw(void)
{
    for (uint8_t b = 0; b < NBUILD; b++) fief_tile(b);
    if (focus) fief_panel();
}

/* ---------------------------------------------------------------- ARMY */
static const char s_need[] PROGMEM = "Needs a bigger Barracks";
static const char s_other[] PROGMEM = "Barracks busy";
static const char s_training[] PROGMEM = "Training started";

static void army_row(uint8_t u)
{
    uint8_t y = (uint8_t)(VIEW_Y + 2 + u * 20);
    uint8_t hot = focus && sel[T_ARMY] == u, ok = unit_unlocked(u);
    gfx_fill(0, y, LCD_W, 19, hot ? C_PANEL : C_INK);
    gfx_bg = hot ? bg_panel : bg_ink;
    sprite(u, 0, 4, (uint8_t)(y + 1), ok ? C_GOLD : C_EMPTY, 0, 2);
    say(26, (uint8_t)(y + 6), ok ? C_TEXT : C_DIM, PSTR("%U"), u);
    if (ok) say(SAY_R, (uint8_t)(y + 6), C_SAND, PSTR("x%u"), G.home[u]);
    else say(SAY_R, (uint8_t)(y + 6), C_EMPTY, PSTR("locked"));
    if (G.train_n && G.train_type == u)
        gfx_fill(26, (uint8_t)(y + 16), G.train_n > 50 ? 50 : G.train_n, 2, C_ORANGE);
    gfx_bg = bg_ink;
}

static void army_panel(void)
{
    uint8_t u = sel[T_ARMY];
    uint8_t n = mode == MD_QTY ? qty : 1;
    uint16_t s = (uint16_t)UI8(u, cost_s) * n, p = (uint16_t)UI8(u, cost_p) * n, w = (uint16_t)UI8(u, cost_w) * n;
    panel_clear();
    say(SAY_C, ROW0, G.res[R_SPICE] >= s && G.res[R_STEEL] >= p && G.res[R_WATER] >= w ? C_TEXT : C_RED,
        PSTR("S%u P%u W%u"), s, p, w);
    if (mode == MD_QTY)
        say(SAY_C, ROW1, C_ORANGE, PSTR("Train < %u >"), qty);
    else if (!unit_unlocked(u))
        say(SAY_C, ROW1, C_DIM, u == U_RIDER ? PSTR("Barracks 2 + tribe allies") : u == U_THOPTER ? PSTR("Needs a Thopter Pad") : s_need);
}

static void army_draw(void)
{
    for (uint8_t u = 0; u < NUNITS; u++) army_row(u);
    if (focus) army_panel();
}

/* ----------------------------------------------------------------- MAP */
static const char s_sent[] PROGMEM = "The army marches!";
static const char s_noarmy[] PROGMEM = "No troops at home";
static const char s_nomarch[] PROGMEM = "Too many armies afield";

static uint16_t bg_map(uint8_t x, uint8_t y)
{
    if (y < VIEW_Y || y >= PANEL_Y) return C_INK;
    uint8_t t = (uint8_t)(((y << 1) + RD8(dune_far + ((x * 3 + y) & 127))) & 31);
    return gfx_mix(RGB(190, 134, 80), RGB(150, 96, 56), (uint8_t)(t * 8), x, y);
}

static void map_node(uint8_t n)
{
    uint8_t x = RD8(&node_xy[n][0]), y = RD8(&node_xy[n][1]);
    gfx_bg = bg_map;
    gfx_clear((uint8_t)(x - 6), (uint8_t)(y - 6), 13, 13);
    gfx_fill((uint8_t)(x - 5), (uint8_t)(y - 5), 11, 11, C_INK);
    gfx_bg = bg_ink;
    icon(terrain_icons + RD8(&node_kind[n]) * 8, 8, 8, (uint8_t)(x - 4), (uint8_t)(y - 4), owner_color(G.node[n].owner));
    if (focus && sel[T_MAP] == n) {
        gfx_fill((uint8_t)(x - 6), (uint8_t)(y + 6), 13, 1, C_TEXT);   /* selection underline */
        gfx_fill((uint8_t)(x - 6), (uint8_t)(y - 6), 13, 1, C_TEXT);
    }
    gfx_bg = bg_ink;
}

static void map_marches(void)
{
    gfx_bg = bg_map;
    for (uint8_t i = 0; i < NMARCH; i++) {
        March *m = &G.march[i];
        if (dot_x[i] != NONE) gfx_clear(dot_x[i], dot_y[i], 3, 3);
        dot_x[i] = NONE;
        if (!m->mission) continue;
        int16_t fx = RD8(&node_xy[m->from][0]), fy = RD8(&node_xy[m->from][1]);
        int16_t tx = RD8(&node_xy[m->to][0]), ty = RD8(&node_xy[m->to][1]);
        int16_t done = (int16_t)(m->total - m->eta);
        dot_x[i] = (uint8_t)(fx + (tx - fx) * done / (int16_t)m->total - 1);
        dot_y[i] = (uint8_t)(fy + (ty - fy) * done / (int16_t)m->total - 1);
        gfx_fill(dot_x[i], dot_y[i], 3, 3, owner_color(m->owner));
        gfx_fill((uint8_t)(dot_x[i] + 1), (uint8_t)(dot_y[i] + 1), 1, 1, RGB(30, 16, 10));
    }
    gfx_bg = bg_ink;
}

static void map_panel(void)
{
    uint8_t n = sel[T_MAP], o = G.node[n].owner;
    panel_clear();
    if (mode == MD_ACTION)
        say(SAY_C, ROW0, C_ORANGE, o == OWN_PLAYER ? PSTR("< RECALL >") : act ? PSTR("< CONQUER >") : PSTR("< RAID >"));
    else if (mode == MD_PCT)
        say(SAY_C, ROW0, C_ORANGE, PSTR("< %u%% of army >"), 25 * (pct_i + 1));
    else
        say(SAY_C, ROW0, owner_color(o), PSTR("%N (%O)"), n, o);
    say(SAY_C, ROW1, C_DIM, n == N_HOME ? PSTR("home: %u troops") : PSTR("%u troops"),
        army_total(n == N_HOME ? G.home : G.node[n].g));
}

static void map_draw(void)
{
    gfx_bg = bg_map;
    gfx_clear(0, VIEW_Y, LCD_W, PANEL_Y - VIEW_Y);
    memset(dot_x, NONE, sizeof dot_x);
    for (uint8_t n = 0; n < NNODES; n++) map_node(n);
    map_marches();
    if (focus) map_panel();
}

/* ------------------------------------------------------------- frame */
static void view_draw(void)
{
    gfx_fill(0, VIEW_Y - 1, LCD_W, STATUS_Y - VIEW_Y - 1, C_INK);
    if (tab == T_FIEF) fief_draw();
    else if (tab == T_ARMY) army_draw();
    else map_draw();
}

static void full_redraw(void)
{
    HOOK(HOOK_SCREEN, 10 + tab);
    gfx_fill(0, 0, LCD_W, LCD_H, C_INK);
    hud(1);
    tabs();
    view_draw();
    status_line();
    leds_world();
}

/* light refresh once per game second */
static void tick_redraw(void)
{
    hud(0);
    status_line();
    if (tab == T_MAP) map_marches();
    else if (tab == T_FIEF && G.build_id != NONE) fief_tile(G.build_id);
    else if (tab == T_ARMY && G.train_n) army_row(G.train_type);
    if ((G.secs & 7) == 0) tabs();
    leds_update();
}

static void move_sel(int8_t d)
{
    uint8_t n = tab == T_FIEF ? NBUILD : tab == T_ARMY ? NUNITS : NNODES;
    uint8_t old = sel[tab];
    sel[tab] = (uint8_t)((sel[tab] + n + d) % n);
    if (tab == T_FIEF) { fief_tile(old); fief_tile(sel[tab]); fief_panel(); }
    else if (tab == T_ARMY) { army_row(old); army_row(sel[tab]); army_panel(); }
    else { map_node(old); map_node(sel[tab]); map_panel(); }
}

static void handle(uint8_t ev)
{
    if (!focus) {
        if (ev == EV_L || ev == EV_R) {
            tab = (uint8_t)((tab + (ev == EV_L ? NTABS - 1 : 1)) % NTABS);
            tabs();
            view_draw();
            HOOK(HOOK_SCREEN, 10 + tab);
        } else if (ev == EV_P) {
            focus = 1;
            mode = MD_BROWSE;
            tabs();
            view_draw();
        }
        return;
    }
    if (ev == EV_HOLD) {
        if (mode != MD_BROWSE) mode = MD_BROWSE;
        else focus = 0;
        tabs();
        view_draw();
        return;
    }
    if (mode == MD_BROWSE) {
        if (ev == EV_L || ev == EV_R) { move_sel(ev == EV_L ? -1 : 1); return; }
        if (ev != EV_P) return;
        if (tab == T_FIEF) {
            static const char *const reasons[3] = { s_busy, s_maxed, s_nores };
            uint8_t r = build_start(sel[T_FIEF]);
            status(r ? reasons[r - 1] : s_started, r ? C_RED : C_GREEN);
            fief_tile(sel[T_FIEF]);
            fief_panel();
            hud(0);
        } else if (tab == T_ARMY) {
            if (!unit_unlocked(sel[T_ARMY])) return;
            mode = MD_QTY;
            qty = 1;
            army_panel();
        } else {
            uint8_t n = sel[T_MAP];
            if (n == N_HOME || (G.node[n].owner == OWN_PLAYER && !army_total(G.node[n].g))) return;
            mode = MD_ACTION;
            act = 0;
            map_panel();
        }
        status_line();
        return;
    }
    if (mode == MD_QTY) {
        if (ev == EV_L && qty > 1) qty--;
        else if (ev == EV_R && qty < 50) qty++;
        else if (ev == EV_P) {
            static const char *const r[3] = { s_need, s_nores, s_other };
            uint8_t e = train_start(sel[T_ARMY], qty);
            status(e ? r[e - 1] : s_training, e ? C_RED : C_GREEN);
            mode = MD_BROWSE;
            army_row(sel[T_ARMY]);
            hud(0);
            status_line();
        }
        army_panel();
        return;
    }
    if (mode == MD_ACTION) {
        uint8_t n = sel[T_MAP];
        if (ev == EV_L || ev == EV_R) act ^= 1;
        else if (ev == EV_P) {
            if (G.node[n].owner == OWN_PLAYER) {
                recall(n);
                mode = MD_BROWSE;
                map_draw();
                return;
            }
            mode = MD_PCT;
            pct_i = 3;
        }
        map_panel();
        return;
    }
    /* MD_PCT: how much of the home army marches */
    if (ev == EV_L || ev == EV_R) pct_i = (uint8_t)((pct_i + (ev == EV_L ? 3 : 1)) & 3);
    else if (ev == EV_P) {
        uint8_t e = march_send(sel[T_MAP], act ? MS_CONQUER : MS_RAID, (uint8_t)(25 * (pct_i + 1)));
        status(e == 1 ? s_noarmy : e == 2 ? s_nomarch : s_sent, e ? C_RED : C_GREEN);
        mode = MD_BROWSE;
        map_draw();
        status_line();
        return;
    }
    map_panel();
}

/* A waiting petitioner steps in once the player is idle at the tab bar. */
static void play(void)
{
    tab = T_FIEF;
    focus = 0;
    mode = MD_BROWSE;
    msg_P = 0;
    full_redraw();
    uint16_t last = hal_ms(), idle = last;
    while (!G.result) {
        uint8_t ev = input_poll();
        if (ev) {
            handle(ev);
            idle = hal_ms();
        }
        if (G.pending && !focus && (uint16_t)(hal_ms() - idle) >= 2500) {
            council_play();
            full_redraw();
            idle = last = hal_ms();
        }
        if ((uint16_t)(hal_ms() - last) >= 1000) {
            if (world_tick()) full_redraw();
            else tick_redraw();
            last = hal_ms();
        }
        hal_idle();
    }
}

void game_main(void)
{
    for (;;) {
        uint8_t cont = title_menu();
        if (!cont || !load_game()) world_new();
        play();
        save_game();
        end_screen();
    }
}
