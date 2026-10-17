/* Minimal streaming interface to the 128x160 ST7735 panel. */
#ifndef LCD_DRV_H
#define LCD_DRV_H

#include "platform.h"

#define LCD_W 128
#define LCD_H 160

void lcd_init(void);
/* Open a drawing window (inclusive corners); pixels then stream
 * left-to-right, top-to-bottom through lcd_px / lcd_fill. */
void lcd_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_px(uint16_t c);
void lcd_fill(uint16_t c, uint16_t n);

#endif
