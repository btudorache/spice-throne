/* Optional serial trace for hardware debugging (build with -DSPICE_TRACE).
 * Prints "<ms> <event> <arg>" lines at 115200 baud on D1 (TX). */
#if defined(__AVR__) && defined(SPICE_TRACE)
#include "hal.h"

static void tx(uint8_t c)
{
    while (!(UCSR0A & _BV(UDRE0))) {}
    UDR0 = c;
}

static void tx_num(uint16_t v)
{
    char b[6];
    uint8_t n = 0;
    do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) tx((uint8_t)b[--n]);
}

void trace_hook(uint8_t ev, uint16_t a)
{
    if (!(UCSR0B & _BV(TXEN0))) {
        UCSR0A = _BV(U2X0);
        UBRR0 = 16;                 /* 115200 baud at 16 MHz, 2.1% error */
        UCSR0B = _BV(TXEN0);
    }
    tx_num(hal_ms());
    tx(' ');
    tx_num(ev);
    tx(' ');
    tx_num(a);
    tx('\n');
}
#endif
