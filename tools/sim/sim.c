/* Desktop simulator for SPICE THRONE: Sand Siege.
 *
 * Links the real game sources against a virtual LCD framebuffer, clock,
 * EEPROM and buttons.
 *
 *   sim shots <dir> <keys>   scripted input, a PPM screenshot before each key.
 *                            L P R tap a lane, 1 2 3 hold a lane,
 *                            s wait 1 s, w wait 5 s, W wait 60 s
 *   sim soak <minutes> [seed] random mashing; fails on invalid draws
 *   sim stats <runs> [seed]   a simple bot plays whole runs; prints waves held
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../spice_throne/game.h"

void game_main(void);
uint8_t sim_spice_ok(void);
uint8_t sim_worm_ready(void);
uint8_t sim_threat(uint8_t lane);

/* ------------------------------------------------------------- LCD */
static uint16_t fb[LCD_H][LCD_W];
static uint8_t wx0, wy0, wx1, wy1, cx, cy;

void lcd_init(void) {}

void lcd_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if (x1 >= LCD_W || y1 >= LCD_H || x0 > x1 || y0 > y1) {
        fprintf(stderr, "bad window %d,%d-%d,%d\n", x0, y0, x1, y1);
        exit(3);
    }
    wx0 = x0; wy0 = y0; wx1 = x1; wy1 = y1; cx = x0; cy = y0;
}

void lcd_px(uint16_t c)
{
    if (cy > wy1) {
        fprintf(stderr, "pixel overflow in window %d,%d-%d,%d\n", wx0, wy0, wx1, wy1);
        exit(3);
    }
    fb[cy][cx] = c;
    if (cx++ == wx1) { cx = wx0; cy++; }
}

void lcd_fill(uint16_t c, uint16_t n)
{
    while (n--) lcd_px(c);
}

/* ------------------------------------------------------------- HAL */
static const uint8_t lane_btn[3] = { BTN_LEFT, BTN_POWER, BTN_RIGHT };
static unsigned long vt;           /* virtual ms */
static uint8_t pending, held;
static unsigned long held_until;
static uint8_t eeprom[1024];
static uint8_t pwm[4];

static enum { MODE_SHOTS, MODE_SOAK, MODE_STATS } mode;
static const char *keys, *shot_dir;
static int shot_n;
static unsigned long next_key, end_at;
static int screen;
static long runs, runs_target, waves_sum, waves_max, cards_seen, worms;
static int hist[32];

void hal_init(void) {}
uint16_t hal_ms(void) { return (uint16_t)vt; }
uint8_t hal_btn_held(void) { return held; }
void hal_pwm(uint8_t led, uint8_t level) { pwm[led] = level; }
uint8_t hal_ee_read(uint16_t a) { return eeprom[a & 1023]; }
void hal_ee_write(uint16_t a, uint8_t v) { eeprom[a & 1023] = v; }

static void save_shot(void)
{
    char path[512];
    snprintf(path, sizeof path, "%s/shot_%03d.ppm", shot_dir, shot_n++);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    int h = LCD_H + 12;
    fprintf(f, "P6\n%d %d\n255\n", LCD_W, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < LCD_W; x++) {
            unsigned char p[3] = { 16, 16, 16 };
            if (y < LCD_H) {
                uint16_t c = fb[y][x];
                p[0] = (unsigned char)(((c >> 11) & 31) * 255 / 31);
                p[1] = (unsigned char)(((c >> 5) & 63) * 255 / 63);
                p[2] = (unsigned char)((c & 31) * 255 / 31);
            } else {
                /* LED strip: yellow on the left, the three green LEDs on the right */
                static const int lx[4] = { 92, 104, 116, 20 };
                for (int i = 0; i < 4; i++) {
                    int dx = x - lx[i], dy = y - (LCD_H + 6);
                    if (dx * dx + dy * dy <= 16) {
                        int v = 40 + pwm[i] * 215 / 255;
                        p[0] = (unsigned char)(i == 3 ? v : v / 4);
                        p[1] = (unsigned char)v;
                        p[2] = (unsigned char)(v / 6);
                    }
                }
            }
            fwrite(p, 1, 3, f);
        }
    fclose(f);
}

static void press(uint8_t lane, uint8_t hold)
{
    pending |= lane_btn[lane];
    held = lane_btn[lane];
    held_until = vt + (hold ? 600 : 60);
}

static void check_invariants(void)
{
    if (R.hp > R.hp_max) { fprintf(stderr, "wall hp over max\n"); exit(5); }
    if (R.wave && !R.deck) { fprintf(stderr, "empty deck\n"); exit(5); }
}

static void bot(void)
{
    if (screen == 20) {             /* in a wave */
        uint8_t best = 0, bt = 0;
        for (uint8_t l = 0; l < 3; l++) {
            uint8_t t = sim_threat(l);
            if (t > bt) { bt = t; best = l; }
        }
        if (sim_worm_ready() && bt > 95) { press(best, 1); worms++; next_key = vt + 900; return; }
        if (sim_spice_ok()) { press(bt ? best : (uint8_t)(rand() % 3), 0); next_key = vt + 250; return; }
        next_key = vt + 100;
    } else {                        /* title, petitions, end screen */
        press(screen == 6 ? (uint8_t)(rand() & 1 ? 2 : 0) : 1, 0);
        next_key = vt + 400;
    }
}

static void feed_input(void)
{
    if (held && vt >= held_until) held = 0;
    if (vt < next_key || held) return;
    if (mode == MODE_SHOTS) {
        save_shot();
        if (!*keys) exit(0);
        char k = *keys++;
        next_key = vt + 700;
        switch (k) {
        case 'L': press(0, 0); break;
        case 'P': press(1, 0); break;
        case 'R': press(2, 0); break;
        case '1': case '2': case '3': press((uint8_t)(k - '1'), 1); break;
        case 's': next_key = vt + 1000; break;
        case 'w': next_key = vt + 5000; break;
        case 'W': next_key = vt + 60000; break;
        }
    } else if (mode == MODE_SOAK) {
        check_invariants();
        if (vt > end_at) {
            printf("soak ok: %lu virtual minutes, %ld runs, %ld petitions\n", vt / 60000, runs, cards_seen);
            exit(0);
        }
        press((uint8_t)(rand() % 3), rand() % 8 == 0);
        next_key = vt + 80 + rand() % 600;
    } else {
        check_invariants();
        bot();
    }
}

static void advance(void)
{
    vt++;
    if ((vt & 15) == 0) leds_tick();
}

void hal_idle(void)
{
    advance();
    feed_input();
}

/* the firmware's hal_delay sleeps through hal_idle too, so input keeps flowing */
void hal_delay(uint16_t ms)
{
    while (ms--) hal_idle();
}

uint8_t hal_btn_take(void)
{
    uint8_t e = pending;
    pending = 0;
    return e;
}

void sim_hook(uint8_t ev, uint16_t a)
{
    if (ev == HOOK_SCREEN) screen = a;
    if (ev == HOOK_CARD) cards_seen++;
    if (ev == HOOK_WAVE_END && (a & 0x100)) {     /* a run ended */
        int w = a & 0xFF;
        runs++;
        waves_sum += w;
        if (w > waves_max) waves_max = w;
        hist[w < 31 ? w : 31]++;
        if (mode == MODE_STATS && runs >= runs_target) {
            printf("runs %ld  avg waves held %.1f  best %ld  worms %ld  petitions %ld\nhistogram:",
                   runs, (double)waves_sum / runs, waves_max, worms, cards_seen);
            for (int i = 0; i < 32; i++) if (hist[i]) printf(" %d:%d", i, hist[i]);
            printf("\n");
            exit(0);
        }
    }
}

int main(int argc, char **argv)
{
    memset(eeprom, 0xFF, sizeof eeprom);
    if (argc >= 4 && !strcmp(argv[1], "shots")) {
        mode = MODE_SHOTS;
        shot_dir = argv[2];
        keys = argv[3];
    } else if (argc >= 3 && !strcmp(argv[1], "soak")) {
        mode = MODE_SOAK;
        end_at = (unsigned long)atol(argv[2]) * 60000UL;
        srand(argc >= 4 ? (unsigned)atoi(argv[3]) : 1u);
    } else if (argc >= 3 && !strcmp(argv[1], "stats")) {
        mode = MODE_STATS;
        runs_target = atol(argv[2]);
        srand(argc >= 4 ? (unsigned)atoi(argv[3]) : 1u);
    } else {
        fprintf(stderr, "usage: sim shots <dir> <keys> | sim soak <minutes> [seed] | sim stats <runs> [seed]\n");
        return 1;
    }
    eeprom[2] = (uint8_t)rand();     /* vary the run seed */
    eeprom[3] = (uint8_t)rand();
    game_main();
    return 0;
}
