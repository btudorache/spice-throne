/* Layout of the generated content (assets_gen.c, from tools/gen_assets.py). */
#ifndef DATA_H
#define DATA_H

#include "platform.h"
#include "assets_gen.h"

/* Card record, CARD_SIZE bytes:
 *  0-1  text offset (little endian) -> question, left label, right label
 *  2    bearer (character index)
 *  3-5  conditions (COND_*)
 *  6    props: bits0-1 weight class (1,2,4,8), CP_ONCE, CP_REIGN_ONCE, CP_CHAIN, CP_PRIORITY
 *  7-11 left side:  eff_ot, eff_ls, flag op, do op, next card
 *  12-16 right side */
#define CARD_SIZE 17
#define CARD_NONE 0xFF

#define CP_ONCE       0x04
#define CP_REIGN_ONCE 0x08
#define CP_CHAIN      0x10
#define CP_PRIORITY   0x20

/* condition byte: 0 none; 0x01-0x3F flag set; 0x41-0x7F flag clear;
 * 0x80 | meter<<5 | ge<<4 | t  ->  meter >= / < 20 + 5t */
#define COND_FLAG_CLEAR 0x40
#define COND_METER      0x80

/* flag op: 0 none; 1-63 set; 0x40|n clear */
/* do op: op<<4 | arg */
enum { DO_NONE, DO_PRESC, DO_HEIR_UP, DO_HEIR_DOWN, DO_FATE, DO_AGE, DO_CHANCE };

extern const uint8_t  text_blob[] PROGMEM;
extern const uint8_t  text_pairs[] PROGMEM;
extern const uint8_t  card_data[] PROGMEM;
extern const uint8_t  portrait_rle[] PROGMEM;
extern const uint16_t portrait_off[] PROGMEM;   /* NCHARS + 1: last is the card back */
extern const uint16_t char_name[] PROGMEM;      /* text offsets */
extern const uint16_t fate_text[] PROGMEM;      /* text offsets: title, then body */
extern const uint8_t  text_lits[] PROGMEM;
extern const uint8_t  fate_who[] PROGMEM;
extern const uint16_t palette[] PROGMEM;        /* 16 RGB565, [0] unused */
extern const uint8_t  icon_bits[] PROGMEM;      /* 4 x 16 rows x 2 bytes */
extern const uint8_t  dune_near[] PROGMEM;      /* 128 heights */
extern const uint8_t  dune_far[] PROGMEM;

#define CARD(i) (card_data + (uint16_t)(i) * CARD_SIZE)

/* text.c: decode one compressed string at *off into out, advance *off.
 * \x01 expands to the ruler's name, \x02 to the house name. */
uint8_t text_get(uint16_t *off, char *out, uint8_t max);
uint8_t name_ruler(char *dst);
uint8_t name_house(char *dst);

#endif
