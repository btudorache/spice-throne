/* Platform glue: lets the same game sources build for the ATmega328P
 * (avr-gcc / Arduino IDE) and for the desktop simulator in tools/sim. */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define RD8(p)  pgm_read_byte(p)
#define RD16(p) pgm_read_word(p)
#define CRIT_BEGIN() uint8_t sreg_ = SREG; cli()
#define CRIT_END()   SREG = sreg_
#ifdef SPICE_TRACE
/* Debug build: game events are printed on the serial port (trace.c). */
void trace_hook(uint8_t ev, uint16_t a);
#define HOOK(ev, a)  trace_hook((ev), (a))
#else
#define HOOK(ev, a)  ((void)(a))
#endif
#else
#define PROGMEM
#define PSTR(s) (s)
#define memcpy_P memcpy
#define RD8(p)  (*(const uint8_t *)(p))
#define RD16(p) (*(const uint16_t *)(p))
#define CRIT_BEGIN() ((void)0)
#define CRIT_END()   ((void)0)
/* Simulator instrumentation (balance statistics, screenshots). */
void sim_hook(uint8_t ev, uint16_t a);
#define HOOK(ev, a) sim_hook((ev), (a))
#endif

enum {
    HOOK_CARD = 1,      /* a petition is shown: a = card index           */
    HOOK_CHOICE,        /* a petition was decided: a = 0 left, 1 right   */
    HOOK_WAVE_END,      /* a = wave held, or 0x100 | waves held at death  */
    HOOK_SCREEN,        /* a = screen id: 1 title, 6 petition, 9 end, 20 wave */
    HOOK_FRAME,         /* a = milliseconds the last battle frame took   */
};

#endif
