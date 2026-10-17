/* LED animation engine shared by firmware and simulator.
 * leds_tick() runs every ~16 ms, stepped from hal_idle() in the main context. */
#include "hal.h"

typedef struct {
    uint8_t mode;
    uint8_t level;
    uint8_t t;
} Led;

static Led leds[4];
static uint8_t lfsr = 0xA5;

void led_set(uint8_t i, uint8_t mode, uint8_t level)
{
    leds[i].mode = mode;
    leds[i].level = level;
    leds[i].t = 0;
}

/* triangle wave 0..255 with a squared (gamma-ish) response */
static uint8_t wave(uint8_t phase)
{
    uint8_t tri = (phase & 0x80) ? (uint8_t)~(phase << 1) : (uint8_t)(phase << 1);
    return (uint8_t)(((uint16_t)tri * tri) >> 8);
}

static uint8_t scale8(uint8_t a, uint8_t b)
{
    return (uint8_t)(((uint16_t)a * b + 255) >> 8);
}

void leds_tick(void)
{
    lfsr = (uint8_t)((lfsr >> 1) ^ (-(lfsr & 1) & 0xB8));
    for (uint8_t i = 0; i < 4; i++) {
        Led *l = &leds[i];
        uint8_t t = ++l->t;
        uint8_t out;
        switch (l->mode) {
        case LM_BREATHE: out = scale8(l->level, wave((uint8_t)(t << 1))); if (out < 6) out = 6; break;
        case LM_PULSE:   out = scale8(l->level, wave((uint8_t)(t << 3))); if (out < 4) out = 4; break;
        case LM_ON:      out = l->level; break;
        case LM_BLINK:   out = (t & 8) ? l->level : 0; break;
        case LM_FLICKER: out = scale8(l->level, (uint8_t)(lfsr | 0x30)); break;
        default:         out = 0; break;
        }
        hal_pwm(i, out);
    }
}
