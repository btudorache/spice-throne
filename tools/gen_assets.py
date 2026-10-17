#!/usr/bin/env python3
"""Convert tools/content/{perks,art}.py into spice_throne/assets_gen.{c,h}.

Text is byte-pair encoded, portraits are 4-bit run-length encoded per row,
unit sprites are 2 bpp. Run after editing any content file:

    python tools/gen_assets.py
"""
import math
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "content"))
import art    # noqa: E402
import perks  # noqa: E402

OUT_DIR = os.path.join(ROOT, "spice_throne")
MAX_DEPTH = 14          # decoder stack is TEXT_MAX_DEPTH = MAX_DEPTH + 2
UNIT_ORDER = ["levy", "trooper", "rifleman", "sandrider", "thopter"]
# effect name -> (FX_* code from data.h, argument kind)
FX = {
    "unlock": (1, "unit"), "hp": (2, "unit"), "atk": (3, "unit"), "cost": (4, "unit"),
    "income": (5, None), "spice": (6, "n"), "wall": (7, "n"), "heal": (8, "n"),
    "hurt": (9, "n"), "harder": (10, "n"), "worm": (11, None), "wormrate": (12, None),
    "nothing": (0, None),
}
# worst-case expansions used for layout checks
RULER_MAX, HOUSE_MAX = "Vashti", "Halloran"


class GenError(Exception):
    pass


def wrap_lines(s, cols):
    """Mirror of gfx_wrap() in gfx.c."""
    lines, i = [], 0
    while i < len(s):
        while i < len(s) and s[i] == " ":
            i += 1
        if i >= len(s):
            break
        n, cut = 0, 0
        while i + n < len(s) and n < cols:
            if s[i + n] == " ":
                cut = n
            n += 1
        if i + n < len(s) and s[i + n] != " " and cut:
            n = cut
        lines.append(s[i:i + n])
        i += n
    return lines


def expand(s):
    return s.replace("$R", RULER_MAX).replace("$D", HOUSE_MAX)


def encode_str(s):
    s = s.replace("$R", "\x01").replace("$D", "\x02")
    out = []
    for ch in s:
        c = ord(ch)
        if not (c in (1, 2) or 32 <= c <= 126):
            raise GenError(f"non-ASCII character {ch!r} in {s!r}")
        out.append(c)
    return out


def bpe(strings):
    """Greedy byte-pair encoding. Literals are remapped to codes 1..n so every
    remaining byte value can name a pair. Returns (seqs, lits, first_pair, pairs, depth)."""
    lits = sorted({c for s in strings for c in s})
    lit_code = {c: i + 1 for i, c in enumerate(lits)}
    first_pair = len(lits) + 1
    seqs = [[lit_code[c] for c in s] for s in strings]
    pairs, depth = [], {}
    while first_pair + len(pairs) < 256:
        counts = {}
        for seq in seqs:
            for a, b in zip(seq, seq[1:]):
                counts[(a, b)] = counts.get((a, b), 0) + 1
        best, best_n = None, 2
        for p, n in counts.items():
            d = 1 + max(depth.get(p[0], 0), depth.get(p[1], 0))
            if n > best_n and d <= MAX_DEPTH:
                best, best_n = p, n
        if best is None:
            break
        code = first_pair + len(pairs)
        depth[code] = 1 + max(depth.get(best[0], 0), depth.get(best[1], 0))
        pairs.append(best)
        for k, seq in enumerate(seqs):
            out, i = [], 0
            while i < len(seq):
                if i + 1 < len(seq) and seq[i] == best[0] and seq[i + 1] == best[1]:
                    out.append(code)
                    i += 2
                else:
                    out.append(seq[i])
                    i += 1
            seqs[k] = out
    return seqs, lits, first_pair, pairs, max(depth.values(), default=0)


class TextTable:
    def __init__(self):
        self.strings = []

    def add(self, s):
        self.strings.append(encode_str(s))
        return len(self.strings) - 1

    def build(self):
        enc, lits, first_pair, pairs, depth = bpe(self.strings)
        blob, offs = [], []
        for seq in enc:
            offs.append(len(blob))
            blob.extend(seq)
            blob.append(0)
        if len(blob) > 0xFFFF:
            raise GenError("text blob exceeds 64 KiB")
        return blob, offs, lits, first_pair, pairs, depth


def rgb565(c):
    r, g, b = c
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rle_rows(rows, what, size=16):
    """4 bpp, runs of 1..16 pixels that never cross a row."""
    if len(rows) != size or any(len(r) != size for r in rows):
        raise GenError(f"{what}: portraits must be {size}x{size}")
    out = []
    for row in rows:
        idx = [0 if ch == "." else int(ch, 16) for ch in row]
        i = 0
        while i < size:
            j = i
            while j < size and j - i < 16 and idx[j] == idx[i]:
                j += 1
            out.append(((j - i - 1) << 4) | idx[i])
            i = j
    return out


def sprite_rows(rows, what):
    """8x8 2bpp: '.'=0 transparent, '#'=1 team, '+'=2 dark, 'o'=3 light."""
    code = {".": 0, "#": 1, "+": 2, "o": 3}
    if len(rows) != 8 or any(len(r) != 8 for r in rows):
        raise GenError(f"{what}: sprites are 8x8")
    out = []
    for row in rows:
        v = 0
        for ch in row:
            v = (v << 2) | code[ch]
        out += [v >> 8, v & 0xFF]
    return out


def parse_fx(e, where):
    """'unlock:rifleman', 'wall+2', 'income' -> FX_* << 4 | arg"""
    for sep in (":", "+"):
        if sep in e:
            name, arg = e.split(sep, 1)
            break
    else:
        name, arg = e, None
    if name not in FX:
        raise GenError(f"{where}: unknown effect {e!r}")
    code, kind = FX[name]
    if kind == "unit":
        if arg not in UNIT_ORDER:
            raise GenError(f"{where}: bad unit in {e!r}")
        return (code << 4) | UNIT_ORDER.index(arg)
    if kind == "n":
        if arg is None or not 1 <= int(arg) <= 15:
            raise GenError(f"{where}: bad amount in {e!r}")
        return (code << 4) | int(arg)
    if arg is not None:
        raise GenError(f"{where}: {name} takes no argument")
    return code << 4


def parse_cond(c, where):
    neg = c.startswith("!")
    name, _, unit = c.lstrip("!").partition(":")
    if name != "has" or unit not in UNIT_ORDER:
        raise GenError(f"{where}: bad condition {c!r}")
    return (0x20 if neg else 0x10) | UNIT_ORDER.index(unit)


def c_array(ctype, name, data, per_line=16, fmt="0x{:02X}"):
    body = []
    for i in range(0, len(data), per_line):
        body.append("    " + ", ".join(fmt.format(v) for v in data[i:i + per_line]) + ",")
    return f"const {ctype} {name}[] PROGMEM = {{\n" + "\n".join(body) + "\n};\n"


def main():
    cards = perks.CARDS
    # only portraits that some card uses are stored
    all_chars = dict(art.CHARACTERS)
    used = []
    for c in cards:
        if c["who"] not in all_chars:
            raise GenError(f"{c['id']}: unknown character {c['who']}")
        if c["who"] not in used:
            used.append(c["who"])
    char_ids = {cid: i for i, cid in enumerate(used)}
    if len(cards) > 254 or len({c["id"] for c in cards}) != len(cards):
        raise GenError("duplicate card ids or too many cards")

    # ---- text
    tt = TextTable()
    card_text = []
    for c in cards:
        lines = wrap_lines(expand(c["text"]), 21)
        if len(lines) > 5:
            raise GenError(f"{c['id']}: text needs {len(lines)} lines: {lines}")
        for side in ("L", "R"):
            if len(expand(c[side]["label"])) > 16:
                raise GenError(f"{c['id']}: label too long")
        card_text.append(tt.add(c["text"]))
        tt.add(c["L"]["label"])
        tt.add(c["R"]["label"])
    name_text = [tt.add(all_chars[cid]) for cid in used]
    blob, offs, lits, first_pair, pairs, depth = tt.build()

    # ---- cards
    data = []
    for i, c in enumerate(cards):
        where = c["id"]
        conds = [parse_cond(w, where) for w in c.get("when", [])]
        if len(conds) > 2:
            raise GenError(f"{where}: more than 2 conditions")
        conds += [0] * (2 - len(conds))
        off = offs[card_text[i]]
        rec = [off & 0xFF, off >> 8, char_ids[c["who"]]] + conds + [c.get("wave", 1), 1 if c.get("once") else 0]
        for sname in ("L", "R"):
            fx = [parse_fx(e, f"{where}.{sname}") for e in c[sname]["fx"]]
            if not 1 <= len(fx) <= 2:
                raise GenError(f"{where}.{sname}: needs 1-2 effects")
            rec += fx + [0] * (2 - len(fx))
        assert len(rec) == 11
        data += rec

    # ---- graphics
    rle, poff = [], []
    for cid in used:
        if cid not in art.PORTRAITS16:
            raise GenError(f"no 16x16 portrait for {cid}")
        poff.append(len(rle))
        rle += rle_rows(art.PORTRAITS16[cid], cid)
    poff.append(len(rle))
    rle += rle_rows(art.CARD_BACK16, "card back")
    pal = [0] + [rgb565(c) for c in art.PALETTE[1:]]
    # unit sprites in engine order; the champion (U_BOSS) follows the five units
    units = [b for name in UNIT_ORDER + ["champion"] for fr in art.UNITS[name] for b in sprite_rows(fr, name)]
    near = [round(122 + 5 * math.sin(2 * math.pi * x / 97 + 0.7) + 3 * math.sin(2 * math.pi * x / 41 + 2))
            for x in range(128)]

    # ---- write
    h = [
        "/* Generated by tools/gen_assets.py - do not edit. */",
        "#ifndef ASSETS_GEN_H", "#define ASSETS_GEN_H",
        f"#define NCARDS {len(cards)}",
        f"#define NCHARS {len(used)}",
        f"#define TEXT_MAX_DEPTH {MAX_DEPTH + 2}",
        f"#define TEXT_FIRST_PAIR {first_pair}",
        "#endif", "",
    ]
    c = ["/* Generated by tools/gen_assets.py - do not edit. */", '#include "data.h"', ""]
    c.append(c_array("uint8_t", "text_blob", blob))
    c.append(c_array("uint8_t", "text_lits", lits))
    c.append(c_array("uint8_t", "text_pairs", [v for p in pairs for v in p] or [0, 0]))
    c.append(c_array("uint8_t", "card_data", data, per_line=11))
    c.append(c_array("uint8_t", "portrait_rle", rle))
    c.append(c_array("uint16_t", "portrait_off", poff, 8, "{}"))
    c.append(c_array("uint16_t", "char_name", [offs[t] for t in name_text], 8, "{}"))
    c.append(c_array("uint16_t", "palette", pal, 8, "0x{:04X}"))
    c.append(c_array("uint8_t", "unit_spr", units))
    c.append(c_array("uint8_t", "dune_near", near, 16, "{}"))
    with open(os.path.join(OUT_DIR, "assets_gen.h"), "w", newline="\n") as f:
        f.write("\n".join(h))
    with open(os.path.join(OUT_DIR, "assets_gen.c"), "w", newline="\n") as f:
        f.write("\n".join(c))

    raw = sum(len(s) + 1 for s in tt.strings)
    print(f"cards {len(cards)}, portraits {len(used)}, "
          f"text {raw} -> {len(blob)} + {2 * len(pairs) + len(lits)} table bytes (depth {depth}), "
          f"card data {len(data)} B, portrait data {len(rle)} B")


if __name__ == "__main__":
    try:
        main()
    except GenError as e:
        sys.exit(f"gen_assets: {e}")
