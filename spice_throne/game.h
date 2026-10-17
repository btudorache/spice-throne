/* SPICE THRONE: Sand Siege - shared state and module interfaces.
 *
 * Three buttons, three lanes. Tap a lane's button to send the unit in your
 * hand down that lane; hold it when the worm is ready to unleash the worm
 * there. Between waves a petitioner offers a Reigns-style bargain. */
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

/* -------------------------------------------------------------- units */
enum { U_LEVY, U_TROOPER, U_RIFLE, U_RIDER, U_THOPTER, NUNITS, U_BOSS = NUNITS };

typedef struct {
    uint8_t hp, atk, range, speed, cd, cost, wall, air;
} UnitInfo;

extern const UnitInfo unit_info[NUNITS + 1] PROGMEM;
#define UI8(u, f) RD8(&unit_info[u].f)

/* --------------------------------------------------------------- run */
#define NBYTES ((NCARDS + 7) / 8)

typedef struct {
    uint8_t wave;
    uint8_t hp, hp_max;         /* citadel wall */
    uint8_t income;             /* spice regenerates faster per level */
    uint8_t spice_bonus;        /* spice at the start of each wave */
    uint8_t worm_rate;          /* extra worm charge per kill */
    uint8_t worm_ready;         /* next wave starts with a full worm */
    uint8_t harder;             /* enemies +10% HP per level */
    uint8_t deck;               /* bitmask of unlocked units */
    uint8_t hp_up[NUNITS], atk_up[NUNITS], cost[NUNITS];
    uint8_t used[NBYTES];       /* once-per-run petitions already seen */
    uint8_t name, house;
} Run;

extern Run R;

/* field.c */
uint8_t  field_wave(void);              /* play R.wave; 1 = the wall held */
void     worm_mouth(uint8_t cx, uint8_t cy, shader_t bg);
void     leds_run(void);

/* perks.c */
void     perk_pick(void);
uint16_t bg_night(uint8_t x, uint8_t y);

/* run.c */
uint8_t  rnd(uint8_t n);
uint16_t rnd16(void);
void     rng_seed(uint32_t s);
/* input: taps and holds per lane (0 left, 1 POWER/middle, 2 right) */
#define EV_NONE   0
#define EV_TAP    0x10
#define EV_HOLD   0x20
#define EV_LANE(e) ((e) & 3)
uint8_t  input_poll(void);
void     input_flush(void);
/* printf-lite with PROGMEM formats: %u %s %S(flash) %U unit name */
#define SAY_C 0xFF
#define SAY_R 0xFE
char    *fmt(char *b, const char *f_P, ...);
void     say(uint8_t x, uint8_t y, uint16_t fg, const char *f_P, ...);
uint16_t bg_ink(uint8_t x, uint8_t y);
void     sprite(uint8_t u, uint8_t frame, uint8_t x, uint8_t y, uint16_t team, uint8_t flip, uint8_t scale);
uint8_t  name_ruler(char *dst);
uint8_t  name_house(char *dst);

/* title.c */
void     title_screen(void);
void     end_screen(void);

#endif
