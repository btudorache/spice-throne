/* Byte-pair-encoded text: codes >= TEXT_FIRST_PAIR expand to two codes from
 * text_pairs, codes below are indexes into the literal table text_lits.
 * The generator bounds nesting depth to TEXT_MAX_DEPTH. */
#include "data.h"

uint8_t text_get(uint16_t *off, char *out, uint8_t max)
{
    uint8_t stack[TEXT_MAX_DEPTH];
    uint8_t sp = 0, n = 0;
    const uint8_t *p = text_blob + *off;

    for (;;) {
        uint8_t c;
        if (sp) {
            c = stack[--sp];
        } else {
            c = RD8(p++);
            if (!c) break;
        }
        while (c >= TEXT_FIRST_PAIR) {
            const uint8_t *pp = text_pairs + (uint16_t)(c - TEXT_FIRST_PAIR) * 2;
            stack[sp++] = RD8(pp + 1);
            c = RD8(pp);
        }
        c = RD8(text_lits + c - 1);
        if (c < 3) {
            char tmp[16];
            uint8_t k = (c == 1) ? name_ruler(tmp) : name_house(tmp);
            for (uint8_t i = 0; i < k && n < max; i++) out[n++] = tmp[i];
        } else if (n < max) {
            out[n++] = (char)c;
        }
    }
    out[n] = 0;
    *off = (uint16_t)(p - text_blob);
    return n;
}
