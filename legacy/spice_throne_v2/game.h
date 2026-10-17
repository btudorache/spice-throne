/* SPICE THRONE II - shared game state and module interfaces. */
#ifndef GAME_H
#define GAME_H

#include <string.h>
#include "hal.h"
#include "gfx.h"
#include "data.h"

/* ------------------------------------------------------------ colours */
#define C_INK    RGB(12, 14, 28)
#define C_PANEL  RGB(30, 22, 30)
#define C_LINE   RGB(92, 62, 40)
#define C_SAND   RGB(242, 210, 150)
#define C_TEXT   RGB(246, 234, 210)
#define C_DIM    RGB(120, 106, 140)
#define C_ORANGE RGB(255, 163, 0)
#define C_RED    RGB(255, 70, 60)
#define C_GREEN  RGB(110, 235, 120)
#define C_BLUE   RGB(80, 180, 255)
#define C_EMPTY  RGB(66, 58, 92)
#define C_GOLD   RGB(240, 190, 80)
#define C_PURPLE RGB(170, 120, 255)

/* --------------------------------------------------------------- world */
enum { R_SPICE, R_WATER, R_STEEL, NRES };
enum { B_KEEP, B_REFINERY, B_WINDTRAP, B_FOUNDRY, B_WAREHOUSE, B_BARRACKS, B_WALL, B_PAD, NBUILD };
enum { U_LEVY, U_TROOPER, U_RIFLE, U_RIDER, U_THOPTER, NUNITS };
enum { OWN_NONE, OWN_PLAYER, OWN_VULKAR, OWN_ORDANE, OWN_TRIBES };
enum { K_CITADEL, K_SIETCH, K_FIELD, K_OUTPOST };
enum { M_ORDER, M_TRIBES, M_LEGION, M_GUILD };
enum { MS_NONE, MS_RAID, MS_CONQUER, MS_RETURN };

#define NNODES   10
#define NMARCH   6
#define N_HOME   0      /* the player's citadel */
#define N_VULKAR 8
#define N_ORDANE 9
#define NONE     0xFF
#define DAY_SECS 30
#define NBYTES   ((NCARDS + 7) / 8)

typedef struct {
    uint8_t owner;
    uint8_t g[NUNITS];      /* garrison */
} Node;

typedef struct {
    uint8_t mission, owner, from, to;
    uint8_t u[NUNITS];
    uint16_t eta, total;    /* seconds left / journey length */
    uint16_t loot;          /* spice carried home */
} March;

typedef struct {
    uint8_t magic;
    uint16_t secs;
    uint16_t res[NRES];
    uint8_t acc[NRES];
    uint8_t bld[NBUILD];
    uint8_t build_id;
    uint16_t build_left;
    uint8_t home[NUNITS];
    uint8_t train_type, train_n, train_left;
    Node node[NNODES];
    March march[NMARCH];
    uint8_t meter[4];
    uint8_t flags[8];
    uint8_t used[NBYTES];
    uint8_t recent[4], recent_i;
    uint8_t pending, charges;
    uint16_t council_t;
    uint16_t ai_t[2];
    uint8_t ai_grow;
    uint8_t house, name;
    uint8_t next_card, first;
    uint8_t result;         /* 0 playing, 1 won, 2 lost */
    uint8_t sum;
} Game;

extern Game G;

typedef struct {
    uint8_t hp, atk, range, speed, cd, cost_s, cost_p, cost_w, time, carry;
} UnitInfo;

extern const UnitInfo unit_info[NUNITS] PROGMEM;
#define UI8(u, f) RD8(&unit_info[u].f)

/* world.c */
void     world_new(void);
uint8_t  world_tick(void);              /* one game second; 1 = screen must be redrawn */
int16_t  world_rate(uint8_t r);         /* per minute */
uint16_t world_cap(void);
uint8_t  build_max(uint8_t b);
void     build_cost(uint8_t b, uint16_t *s, uint16_t *p, uint16_t *t);
uint8_t  build_start(uint8_t b);        /* 0 ok, else reason code */
uint8_t  unit_unlocked(uint8_t u);
uint8_t  train_start(uint8_t u, uint8_t n);
uint8_t  march_send(uint8_t to, uint8_t mission, uint8_t pct);
uint8_t  recall(uint8_t n);
uint16_t army_total(const uint8_t *u);
uint16_t player_troops(void);
uint8_t  threat(uint16_t *eta, uint16_t *total, uint8_t *who, uint8_t *where);
void     ai_attack(uint8_t rival, uint8_t forced);
void     node_lose_random(void);
void     node_gain_field(void);
uint8_t  rnd(uint8_t n);
uint16_t rnd16(void);
void     save_game(void);
uint8_t  load_game(void);
extern const uint8_t node_xy[NNODES][2] PROGMEM;
extern const uint8_t node_kind[NNODES] PROGMEM;

/* battle.c: returns 1 if the attacker wins; armies are updated in place */
uint8_t  battle(uint8_t *att, uint8_t att_owner, uint8_t *def, uint8_t def_owner,
                uint8_t node, uint8_t wall, uint8_t animate);

/* council.c */
void     council_play(void);
void     meter_icon(uint8_t i, uint8_t x, uint8_t y, uint8_t val, uint16_t fill);
uint16_t meter_color(uint8_t v);
void     update_builtins(void);
uint16_t bg_night(uint8_t x, uint8_t y);

/* ui.c helpers shared by all screens */
enum { EV_NONE, EV_L, EV_R, EV_P, EV_HOLD };
uint8_t  input_poll(void);
/* printf-lite with PROGMEM formats: %u %t(m:ss) %s %S(flash) %N node %U unit
 * %B building %O owner %M meter %T tab %H house; say() draws over gfx_bg */
#define SAY_C 0xFF
#define SAY_R 0xFE
char    *fmt(char *b, const char *f_P, ...);
void     say(uint8_t x, uint8_t y, uint16_t fg, const char *f_P, ...);
void     status(const char *s_P, uint16_t fg);
uint16_t owner_color(uint8_t o);
void     node_name(uint8_t n, char *b);
void     unit_name(uint8_t u, char *b);
void     sprite(uint8_t u, uint8_t frame, uint8_t x, uint8_t y, uint16_t team, uint8_t flip, uint8_t scale);
uint16_t bg_ink(uint8_t x, uint8_t y);
void     leds_world(void);
uint8_t  name_ruler(char *dst);
uint8_t  name_house(char *dst);
void     world_seed(uint32_t s);

/* battle.c */
void     worm_mouth(uint8_t cx, uint8_t cy, shader_t bg);

/* title.c */
uint8_t  title_menu(void);              /* 0 new game, 1 continue */
void     end_screen(void);


#endif
