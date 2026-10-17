/* ST7735 driver over hardware SPI at 8 MHz.
 *   CS  D10 (PB2)   DC  D9 (PB1)   RST D8 (PB0)
 *   SDA D11 (PB3 / MOSI)   SCK D13 (PB5)
 * The init sequence is the one the Arduino TFT library sends to this
 * exact 1.8" module (initG), so the panel behaves as it did in Meteor
 * Dodge. CS stays asserted: the display is the only SPI device.
 *
 * SPI pipelining: the bus is always left with SPIF set when idle, so
 * every byte write first waits for the previous one and then returns
 * immediately, letting the CPU compute the next pixel during transfer. */
#ifdef __AVR__
#include <util/delay.h>
#include "lcd.h"

#define DC_CMD()  (PORTB &= (uint8_t)~_BV(PB1))
#define DC_DATA() (PORTB |= _BV(PB1))

#ifndef LCD_MADCTL
#define LCD_MADCTL 0x08   /* portrait, same as TFT setRotation(2) */
#endif

static inline void spi_put(uint8_t b)
{
    while (!(SPSR & _BV(SPIF))) {}
    SPDR = b;
}

static inline void spi_wait(void)
{
    while (!(SPSR & _BV(SPIF))) {}
}

static void cmd(uint8_t c)
{
    spi_wait();
    DC_CMD();
    SPDR = c;
    spi_wait();
    DC_DATA();
}

#define DELAY 0x80
static const uint8_t init_seq[] PROGMEM = {
    0x01, DELAY, 50,               /* SWRESET */
    0x11, DELAY, 120,              /* SLPOUT */
    0x26, 1, 0x04,                 /* gamma curve */
    0xB1, 2, 0x0B, 0x14,           /* frame rate */
    0xC0, 2, 0x08, 0x00,           /* power 1 */
    0xC1, 1, 0x05,                 /* power 2 */
    0xC5, 2, 0x41, 0x30,           /* VCOM */
    0xC7, 1, 0xC1,                 /* VCOM offset */
    0xEC, 1, 0x1B,                 /* pumping clock */
    0x3A, 1 | DELAY, 0x55, 10,     /* 16-bit colour */
    0x36, 1, LCD_MADCTL,           /* orientation */
    0xB7, 1, 0x00,                 /* source output direction */
    0xF2, 1, 0x00,                 /* gamma adjust enable */
    0xE0, 15 | DELAY, 0x28, 0x24, 0x22, 0x31, 0x2B, 0x0E, 0x53, 0xA5,
                      0x42, 0x16, 0x18, 0x12, 0x1A, 0x14, 0x03, 20,
    0xE1, 15 | DELAY, 0x17, 0x1B, 0x1D, 0x0E, 0x14, 0x11, 0x2C, 0xA5,
                      0x3D, 0x09, 0x27, 0x2D, 0x25, 0x2B, 0x3C, 20,
    0x13, DELAY, 10,               /* NORON */
    0x29, DELAY, 20,               /* DISPON */
    0x00
};

void lcd_init(void)
{
    DDRB  |= _BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB3) | _BV(PB5);
    PORTB |= _BV(PB0) | _BV(PB1) | _BV(PB2);

    SPCR = _BV(SPE) | _BV(MSTR);       /* mode 0, MSB first */
    SPSR = _BV(SPI2X);                 /* fosc/2 = 8 MHz    */
    SPDR = 0;                          /* prime SPIF        */

    _delay_ms(10);
    PORTB &= (uint8_t)~_BV(PB0);       /* hardware reset */
    _delay_ms(20);
    PORTB |= _BV(PB0);
    _delay_ms(150);
    PORTB &= (uint8_t)~_BV(PB2);       /* CS low for good */

    const uint8_t *p = init_seq;
    uint8_t c;
    while ((c = RD8(p++)) != 0) {
        uint8_t n = RD8(p++);
        cmd(c);
        for (uint8_t i = 0; i < (n & 0x7F); i++) spi_put(RD8(p++));
        if (n & DELAY) {
            uint8_t ms = RD8(p++);
            while (ms--) _delay_ms(1);
        }
    }
}

void lcd_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    cmd(0x2A);
    spi_put(0); spi_put(x0); spi_put(0); spi_put(x1);
    cmd(0x2B);
    spi_put(0); spi_put(y0); spi_put(0); spi_put(y1);
    cmd(0x2C);
}

void lcd_px(uint16_t c)
{
    spi_put((uint8_t)(c >> 8));
    spi_put((uint8_t)c);
}

void lcd_fill(uint16_t c, uint16_t n)
{
    uint8_t hi = (uint8_t)(c >> 8), lo = (uint8_t)c;
    while (n--) {
        spi_put(hi);
        spi_put(lo);
    }
}
#endif
