/* Layout of the generated content (assets_gen.c, from tools/gen_assets.py). */
#ifndef DATA_H
#define DATA_H

#include "platform.h"
#include "assets_gen.h"

/* Petition card record, CARD_SIZE bytes:
 *  0-1  text offset (little endian) -> petition, left label, right label
 *  2    bearer (portrait index)
 *  3-4  conditions: 0 none, 0x10|u = unit u unlocked, 0x20|u = not unlocked
 *  5    first wave it may appear in
 *  6    flags: CF_ONCE
 *  7-8  left side effects, 9-10 right side effects (FX_* << 4 | arg) */
#define CARD_SIZE 11
#define CF_ONCE   0x01
#define COND_HAS  0x10
#define COND_LACK 0x20

enum { FX_NONE, FX_UNLOCK, FX_HP, FX_ATK, FX_COST, FX_INCOME, FX_SPICE, FX_WALL,
       FX_HEAL, FX_HURT, FX_HARDER, FX_WORM, FX_WORMRATE };

extern const uint8_t  text_blob[] PROGMEM;
extern const uint8_t  text_lits[] PROGMEM;
extern const uint8_t  text_pairs[] PROGMEM;
extern const uint8_t  card_data[] PROGMEM;
extern const uint8_t  portrait_rle[] PROGMEM;
extern const uint16_t portrait_off[] PROGMEM;   /* NCHARS + 1: last is the card back */
extern const uint16_t char_name[] PROGMEM;      /* text offsets */
extern const uint16_t palette[] PROGMEM;        /* 16 RGB565, [0] unused */
extern const uint8_t  unit_spr[] PROGMEM;       /* (NUNITS + champion) x 2 frames x 8 rows x 2 bytes (2bpp) */
extern const uint8_t  dune_near[] PROGMEM;      /* 128 heights */

#define CARD(i) (card_data + (uint16_t)(i) * CARD_SIZE)

/* text.c: decode one compressed string at *off into out, advance *off.
 * \x01 expands to the ruler's name, \x02 to the house name. */
uint8_t text_get(uint16_t *off, char *out, uint8_t max);
uint8_t name_ruler(char *dst);
uint8_t name_house(char *dst);

#endif
