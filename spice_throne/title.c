/* Title screen (night dunes and a worm that surfaces now and then) and
 * the end-of-run screen. The best wave reached is kept in EEPROM. */
#include "game.h"

#define EE_MAGIC 0
#define EE_BEST  1
#define EE_SEED  2
#define META_MAGIC 0x5C

static uint8_t best;

void title_screen(void)
{
    char b[8];
    if (hal_ee_read(EE_MAGIC) != META_MAGIC) {
        hal_ee_write(EE_MAGIC, META_MAGIC);
        hal_ee_write(EE_BEST, 0);
    }
    best = hal_ee_read(EE_BEST);

    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    gfx_text_c(36, fmt(b, PSTR("SPICE")), C_ORANGE, RGB(40, 8, 20), 3);
    gfx_text_c(62, fmt(b, PSTR("THRONE")), RGB(255, 214, 120), RGB(40, 8, 20), 2);
    say(SAY_C, 82, RGB(250, 200, 170), PSTR("sand siege"));
    say(SAY_C, 7, C_DIM, PSTR("best: wave %u"), best);
    say(SAY_C, 146, C_DIM, PSTR("L / POWER / R = lanes"));

    for (uint8_t i = 0; i < 4; i++) led_set(i, LM_BREATHE, 160);
    HOOK(HOOK_SCREEN, 1);
    input_flush();
    uint16_t t_blink = hal_ms(), t_worm = hal_ms();
    uint8_t on = 0, x = 26;
    for (;;) {
        if (input_poll() & EV_TAP) break;
#ifdef SPICE_TRACE
        if (hal_ms() > 8000) break;   /* trace builds start unattended for telemetry */
#endif
        uint16_t now = hal_ms();
        if ((uint16_t)(now - t_blink) >= 600) {
            t_blink = now;
            on ^= 1;
            gfx_clear(0, 104, LCD_W, 10);
            if (on) say(SAY_C, 106, C_SAND, PSTR("- press any button -"));
        }
        if ((uint16_t)(now - t_worm) >= 6000) {
            x = (uint8_t)(x == 26 ? 100 : 26);
            worm_mouth(x, (uint8_t)(RD8(dune_near + x) + 4), bg_night);
            t_worm = hal_ms();
        }
        hal_idle();
    }
    /* the boot counter plus the human's reaction time seed the run */
    uint16_t boots = (uint16_t)(hal_ee_read(EE_SEED) | (hal_ee_read(EE_SEED + 1) << 8));
    boots++;
    hal_ee_write(EE_SEED, (uint8_t)boots);
    hal_ee_write(EE_SEED + 1, (uint8_t)(boots >> 8));
    rng_seed(((uint32_t)boots << 16) ^ hal_ms() ^ 0x5EED1234u);
}

void end_screen(void)
{
    char b[12];
    uint8_t held = (uint8_t)(R.wave - 1), record = held > best;
    if (record) hal_ee_write(EE_BEST, held);
    HOOK(HOOK_WAVE_END, 0x100 | held);

    gfx_bg = bg_night;
    gfx_clear(0, 0, LCD_W, LCD_H);
    gfx_text_c(34, fmt(b, PSTR("CITADEL")), C_RED, RGB(30, 10, 10), 2);
    gfx_text_c(54, fmt(b, PSTR("FALLEN")), C_RED, RGB(30, 10, 10), 2);
    say(SAY_C, 80, C_SAND, PSTR("House %s held"), (name_house(b), b));
    say(SAY_C, 92, C_TEXT, PSTR("%u waves"), held);
    if (record) say(SAY_C, 106, C_ORANGE, PSTR("NEW RECORD"));
    for (uint8_t i = 0; i < 3; i++) led_set(i, record ? LM_BLINK : LM_OFF, 255);
    led_set(LED_YELLOW, LM_OFF, 0);
    hal_delay(1500);
    say(SAY_C, 7, C_DIM, PSTR("- press any button -"));
    HOOK(HOOK_SCREEN, 9);
    input_flush();
    while (!(input_poll() & EV_TAP)) hal_idle();
}
