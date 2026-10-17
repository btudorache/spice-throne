#ifndef GFX_H
#define GFX_H

#include "lcd.h"

#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

typedef uint16_t (*shader_t)(uint8_t x, uint8_t y);

/* Current scene background; "erasing" means re-shading with it. */
extern shader_t gfx_bg;

void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c);
void gfx_shade(uint8_t x, uint8_t y, uint8_t w, uint8_t h, shader_t s);
#define gfx_clear(x, y, w, h) gfx_shade((x), (y), (w), (h), gfx_bg)

/* Dithered blend of two RGB565 colours, t = 0..255. */
uint16_t gfx_mix(uint16_t c0, uint16_t c1, uint8_t t, uint8_t x, uint8_t y);

#define FONT_W 6
#define FONT_H 8
#define NO_SHADOW 0x0020  /* not a colour any scene uses */

/* Text is drawn opaque over gfx_bg, optionally with a drop shadow. */
void    gfx_char(uint8_t x, uint8_t y, char ch, uint16_t fg, uint16_t shadow, uint8_t scale);
uint8_t gfx_text(uint8_t x, uint8_t y, const char *s, uint16_t fg, uint16_t shadow, uint8_t scale);
void    gfx_text_c(uint8_t y, const char *s, uint16_t fg, uint16_t shadow, uint8_t scale);
uint8_t gfx_strlen(const char *s);

/* Word-wrap into lines of `cols` characters; returns line count.
 * With ms_per_char > 0 the text types itself out; any key press skips. */
uint8_t gfx_wrap(uint8_t x, uint8_t y, uint8_t cols, uint8_t max_lines, uint8_t line_h,
                 const char *s, uint16_t fg, uint8_t ms_per_char, uint8_t centered);

#endif
