/* Title screen (night dunes and a worm that surfaces now and then), the
 * NEW/CONTINUE menu, and the end-of-game screen. */
#include "game.h"

#define EE_MAGIC 0
#define EE_BEST  2
#define EE_WINS  4
#define EE_SEED  6
#define META_MAGIC 0x5B
#define EE_SAVE_MAGIC 32   /* first byte of the saved Game (G.magic) */

static uint16_t best_day;
static uint8_t meta_wins;

static void meta_load(void)
{
    if (hal_ee_read(EE_MAGIC) != META_MAGIC) {
        for (uint8_t a = 0; a < 8; a++) hal_ee_write(a, 0);
        hal_ee_write(EE_MAGIC, META_MAGIC);
        hal_ee_write(EE_SAVE_MAGIC, 0);
    }
    best_day = (uint16_t)(hal_ee_read(EE_BEST) | (hal_ee_read(EE_BEST + 1) << 8));
    meta_wins = hal_ee_read(EE_WINS);
}

static void menu_draw(uint8_t choice, uint8_t can_cont)
{
    char b[20];
    gfx_clear(0, 104, LCD_W, 14);
    fmt(b, can_cont ? PSTR("< %S >") : PSTR("%S"), choice ? PSTR("CONTINUE") : PSTR("NEW GAME"));
    gfx_text_c(108, b, C_SAND, RGB(30, 10, 10), 1);
}

uint8_t title_menu(void)
{
    char b[12];
    meta_load();
    uint8_t can_cont = hal_ee_read(EE_SAVE_MAGIC) == 0xB2;
    uint8_t choice = can_cont;

    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    gfx_text_c(40, fmt(b, PSTR("SPICE")), C_ORANGE, RGB(40, 8, 20), 3);
    gfx_text_c(66, fmt(b, PSTR("THRONE")), RGB(255, 214, 120), RGB(40, 8, 20), 2);
    say(SAY_C, 86, RGB(250, 200, 170), PSTR("war for Kessar"));
    say(SAY_C, 7, C_DIM, PSTR("best: day %u  wins %u"), best_day, meta_wins);
    menu_draw(choice, can_cont);

    led_set(LED_YELLOW, LM_BREATHE, 120);
    for (uint8_t i = 0; i < 3; i++) led_set(i, LM_BREATHE, 160);
    HOOK(HOOK_SCREEN, 1);
    uint16_t t_worm = hal_ms();
    uint8_t dir = 0;
    for (;;) {
        uint8_t e = input_poll();
        if (e == EV_P) {
            /* the boot counter plus the human's reaction time seed the game */
            uint16_t boots = (uint16_t)(hal_ee_read(EE_SEED) | (hal_ee_read(EE_SEED + 1) << 8));
            hal_ee_write(EE_SEED, (uint8_t)++boots);
            hal_ee_write(EE_SEED + 1, (uint8_t)(boots >> 8));
            world_seed(((uint32_t)boots << 16) ^ hal_ms() ^ 0x5EED1234u);
            if (!choice) hal_ee_write(EE_SAVE_MAGIC, 0);
            return choice;
        }
        if ((e == EV_L || e == EV_R) && can_cont) {
            choice ^= 1;
            menu_draw(choice, can_cont);
        }
        if ((uint16_t)(hal_ms() - t_worm) >= 7000) {
            dir = (uint8_t)(dir == 26 ? 100 : 26);
            worm_mouth(dir, (uint8_t)(RD8(dune_near + dir) + 4), bg_night);
            t_worm = hal_ms();
        }
        hal_idle();
    }
}

void end_screen(void)
{
    char b[12];
    uint8_t win = G.result == 1;
    uint16_t day = (uint16_t)(G.secs / DAY_SECS + 1);
    uint8_t record = day > best_day;
    if (record) {
        hal_ee_write(EE_BEST, (uint8_t)day);
        hal_ee_write(EE_BEST + 1, (uint8_t)(day >> 8));
    }
    if (win && meta_wins < 255) hal_ee_write(EE_WINS, (uint8_t)(meta_wins + 1));
    HOOK(HOOK_DYNASTY_END, day);

    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    uint16_t c = win ? C_GREEN : C_RED;
    gfx_text_c(30, fmt(b, win ? PSTR("KESSAR") : PSTR("CITADEL")), c, RGB(30, 10, 10), 2);
    gfx_text_c(50, fmt(b, win ? PSTR("IS YOURS") : PSTR("HAS FALLEN")), c, RGB(30, 10, 10), 2);
    say(SAY_C, 76, C_SAND, PSTR("House %H"), G.house);
    say(SAY_C, 88, C_TEXT, PSTR("survived %u days"), day);
    if (record) say(SAY_C, 100, C_ORANGE, PSTR("NEW RECORD"));
    for (uint8_t i = 0; i < 3; i++) led_set(i, win || record ? LM_BLINK : LM_OFF, 255);
    led_set(LED_YELLOW, win ? LM_BREATHE : LM_OFF, 255);
    say(SAY_C, 7, C_DIM, PSTR("- press POWER -"));
    hal_delay(800);
    HOOK(HOOK_SCREEN, 9);
    while (input_poll() != EV_P) hal_idle();
}
