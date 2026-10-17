/* ATmega328P implementation of hal.h (bare metal, no Arduino core). */
#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "hal.h"

static volatile uint16_t ticks;
static volatile uint8_t btn_level, btn_events;
static uint8_t btn_cnt[3];
static uint8_t led_div;
static volatile uint8_t yellow_level;

/* Timer0 overflow: 16 MHz / 64 / 256 = 976.5 Hz (~1.024 ms). */
ISR(TIMER0_OVF_vect)
{
    ticks++;

    uint8_t raw = 0;
    if (!(PIND & _BV(PD2))) raw |= BTN_LEFT;
    if (!(PINB & _BV(PB4))) raw |= BTN_POWER;
    if (!(PINC & _BV(PC0))) raw |= BTN_RIGHT;

    /* a level must be stable for 6 consecutive ticks to be accepted */
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t bit = (uint8_t)(1 << i);
        if ((raw ^ btn_level) & bit) {
            if (++btn_cnt[i] >= 6) {
                btn_cnt[i] = 0;
                btn_level ^= bit;
                if (btn_level & bit) btn_events |= bit;
            }
        } else {
            btn_cnt[i] = 0;
        }
    }

    if (++led_div >= 16) {
        led_div = 0;
        leds_tick();
    }
}

/* Yellow LED (PD4) has no PWM channel: Timer2 overflow turns it on,
 * compare-match A (pin output disconnected, PB3 is MOSI) turns it off. */
ISR(TIMER2_OVF_vect)
{
    if (yellow_level) PORTD |= _BV(PD4);
}

ISR(TIMER2_COMPA_vect)
{
    if (yellow_level != 255) PORTD &= (uint8_t)~_BV(PD4);
}

void hal_init(void)
{
    cli();
    /* buttons: inputs with pull-ups */
    DDRD &= (uint8_t)~_BV(PD2);  PORTD |= _BV(PD2);
    DDRB &= (uint8_t)~_BV(PB4);  PORTB |= _BV(PB4);
    DDRC &= (uint8_t)~_BV(PC0);  PORTC |= _BV(PC0);

    /* LEDs: outputs, off */
    DDRD |= _BV(PD3) | _BV(PD4) | _BV(PD5) | _BV(PD6);
    PORTD &= (uint8_t)~(_BV(PD3) | _BV(PD4) | _BV(PD5) | _BV(PD6));

    /* Timer0: fast PWM 8-bit, clk/64, overflow interrupt = system tick */
    TCCR0A = _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01) | _BV(CS00);
    TIMSK0 = _BV(TOIE0);

    /* Timer2: fast PWM 8-bit, clk/64 */
    TCCR2A = _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS22);
    TIMSK2 = _BV(TOIE2) | _BV(OCIE2A);

    /* Timer1 free running at clk/1: only used as an entropy source */
    TCCR1A = 0;
    TCCR1B = _BV(CS10);

    sei();
}

uint16_t hal_ms(void)
{
    CRIT_BEGIN();
    uint16_t t = ticks;
    CRIT_END();
    return t;
}

void hal_idle(void)
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
}

void hal_delay(uint16_t ms)
{
    uint16_t start = hal_ms();
    while ((uint16_t)(hal_ms() - start) < ms) hal_idle();
}

uint8_t hal_btn_held(void)
{
    return btn_level;
}

uint8_t hal_btn_take(void)
{
    CRIT_BEGIN();
    uint8_t e = btn_events;
    btn_events = 0;
    CRIT_END();
    return e;
}

void hal_pwm(uint8_t led, uint8_t v)
{
    switch (led) {
    case 0: /* D3 = OC2B */
        if (v) { OCR2B = v; TCCR2A |= _BV(COM2B1); }
        else   { TCCR2A &= (uint8_t)~_BV(COM2B1); PORTD &= (uint8_t)~_BV(PD3); }
        break;
    case 1: /* D5 = OC0B */
        if (v) { OCR0B = v; TCCR0A |= _BV(COM0B1); }
        else   { TCCR0A &= (uint8_t)~_BV(COM0B1); PORTD &= (uint8_t)~_BV(PD5); }
        break;
    case 2: /* D6 = OC0A */
        if (v) { OCR0A = v; TCCR0A |= _BV(COM0A1); }
        else   { TCCR0A &= (uint8_t)~_BV(COM0A1); PORTD &= (uint8_t)~_BV(PD6); }
        break;
    default: /* D4, soft PWM */
        yellow_level = v;
        OCR2A = v;
        if (!v) PORTD &= (uint8_t)~_BV(PD4);
        break;
    }
}

uint8_t hal_ee_read(uint16_t addr)
{
    return eeprom_read_byte((const uint8_t *)addr);
}

void hal_ee_write(uint16_t addr, uint8_t v)
{
    eeprom_update_byte((uint8_t *)addr, v);
}

/* Floating analog inputs A1..A5 plus timer jitter, folded into 32 bits. */
uint32_t hal_entropy(void)
{
    uint32_t h = 2166136261u;
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    for (uint8_t i = 0; i < 64; i++) {
        ADMUX = (uint8_t)(_BV(REFS0) | (1 + (i % 5)));
        ADCSRA |= _BV(ADSC);
        while (ADCSRA & _BV(ADSC)) {}
        uint16_t a = ADC; /* avr-libc reads ADCL before ADCH */
        h = (h ^ a ^ TCNT1L) * 16777619u;
    }
    ADCSRA = 0;
    return h ^ ((uint32_t)TCNT1 << 16) ^ ticks;
}
#endif
