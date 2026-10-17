"""SPICE THRONE pixel art (pure data). Consumed by tools/gen_assets.py.

Portrait chars: '.' = palette index 0 (transparent), '1'-'9','a'-'f' = palette 1..15.
PORTRAITS16 / CARD_BACK16 are 16x16 (drawn at 3x on the parchment card).
UNITS are 8x8 two-frame battle sprites with their own 4-char set (see below).
"""

PALETTE = [None, (0,0,0), (29,43,83), (126,37,83), (0,135,81), (171,82,54), (95,87,79), (194,195,199), (255,241,232), (255,0,77), (255,163,0), (255,236,39), (41,173,255), (131,118,156), (255,204,170), (122,74,42)]
# index: 0 transparent(card face shows through), 1 black, 2 navy, 3 plum, 4 green, 5 brown/rust, 6 dark grey, 7 light grey, 8 white, 9 red, 10 orange, 11 yellow, 12 bright blue, 13 lavender, 14 light skin, 15 dark skin/leather

CHARACTERS = [
    ('advisor', 'Mentat Vael'),
    ('sister', 'Rev.Mother Ysolde'),
    ('naib', 'Naib Kadar'),
    ('marshal', 'Marshal Draxe'),
    ('envoy', 'Imperial Envoy'),
    ('navigator', 'Guild Navigator'),
    ('baron', 'Baron Vulkar'),
    ('smuggler', 'Tessa the Smuggler'),
    ('planetologist', 'Dr. Ophir'),
    ('heir', 'Your Heir'),
    ('swordmaster', 'Swordmaster Halder'),
    ('harvester', 'Crawler Boss Mott'),
    ('prophet', 'The Prophet'),
    ('worm', 'The Old Worm'),
    ('consort', 'Lady Imre'),
    ('physician', 'Physician Sarn'),
    ('stranger', 'A Stranger'),
]

# 16x16 portraits (shown at 3x on the parchment card), palette chars as described above.
# Flat areas + 1px outline: rows are run-length encoded, so long runs are cheap.
PORTRAITS16 = {
    "advisor": [
        "................",
        "......1111......",
        "....11eeee11....",
        "...1eeeeeeee1...",
        "...1eeeeeeee1...",
        "...1e11ee11e1...",
        "..1f1eeeeee1f1..",
        "...1eeeffeee1...",
        "...1ee9999ee1...",
        "....1eee9ee1....",
        ".....11ee11.....",
        "...1177ee7711...",
        "..177776677771..",
        ".17777666677771.",
        "1777766666677771",
        "1777766666677771",
    ],
    "sister": [
        "................",
        ".......11.......",
        ".....111111.....",
        "....11222211....",
        "...1221111221...",
        "...121eeee121...",
        "...12eceece21...",
        "...12eeeeee21...",
        "...1211111121...",
        "...1211111121...",
        "..112111111211..",
        ".11211111111211.",
        "1121111111111211",
        "12111111bb111121",
        "121111baab111121",
        "1211111111111121",
    ],
    "naib": [
        "................",
        "......1111......",
        "....11555511....",
        "...1555555551...",
        "..155555555551..",
        "..1551ffff1551..",
        "..155fcffcf551..",
        "..155666666551..",
        "..155616616551..",
        "..155516615551..",
        ".15555166155551.",
        "1555551661555551",
        "1555555665555551",
        "1f555556655555f1",
        "1555555665555551",
        "1555555665555551",
    ],
    "marshal": [
        ".......99.......",
        "......1991......",
        "....16699661....",
        "...1666666661...",
        "...1666666661...",
        "...16eeeeee61...",
        "...16e1ee1e61...",
        "...16eeeeee61...",
        "...16ee11ee61...",
        "....11eeee11....",
        ".....11ee11.....",
        "..166296622661..",
        ".16662222226661.",
        "1266222222226621",
        "1266222222226621",
        "1222222222222221",
    ],
    "envoy": [
        "................",
        ".....111111.....",
        "....12222221....",
        "....1eeeeee1....",
        "....1e1ee1e1....",
        "....1eeeeee1....",
        "...b1e1111e1b...",
        "...bb111111bb...",
        "..1bbb1ee1bbb1..",
        ".1abbbb11bbbba1.",
        ".13abbbbbbbba31.",
        ".133abbbbbba331.",
        ".1333ab33ba3331.",
        ".13333a33a33331.",
        "133333a33a333331",
        "133333a33a333331",
    ],
    "navigator": [
        ".11111111111111.",
        ".18aaaaaaaaaaa1.",
        ".18aa111111aaa1.",
        ".18a1dddddd1aa1.",
        ".1a1dddddddd1a1.",
        ".1a1dddddddd1a1.",
        ".1a1d11dd11d1a1.",
        ".1aa1dddddd1aa1.",
        ".1aaa1d11d1aaa1.",
        ".1aaaa1dd1aaaa1.",
        ".1aa11333311aa1.",
        ".1aa11333311aa1.",
        ".1aaa133331aaa1.",
        ".1aaaa1111aaaa1.",
        ".1aaaaaaaaaaaa1.",
        ".11111111111111.",
    ],
    "baron": [
        "................",
        ".....111111.....",
        "....1eeeeee1....",
        "...1eeeeeeee1...",
        "..1eeeeeeeeee1..",
        "..1ee11ee11ee1..",
        "..1eeeeeeeeee1..",
        "..1feeeeeeeef1..",
        "..1fe111111ef1..",
        "..11ffeeeeff11..",
        ".99111111111199.",
        "1999111111119991",
        "1999911111199991",
        "1999911111199991",
        "1999911111199991",
        "1999911111199991",
    ],
    "smuggler": [
        "................",
        ".....111111.....",
        "....15555551....",
        "...1555555551...",
        "..11c111111c11..",
        "...15eeeeee51...",
        "...1511ee1151...",
        "...15eeeeee51...",
        "...15e1111e51...",
        "...1599999951...",
        "..1f99999999f1..",
        ".1ff99999999ff1.",
        "1ffff999999ffff1",
        "1ffff599995ffff1",
        "1fffff5995fffff1",
        "1ffffff55ffffff1",
    ],
    "planetologist": [
        "................",
        "......1111......",
        "....11555511....",
        "....15555551....",
        "....1eeeeee1....",
        "....11c11c11....",
        "....1eeeeee1....",
        "....1e5555e1....",
        "....15555551....",
        "....15555551....",
        ".14441555514441.",
        "1444411551144441",
        "1444441881444441",
        "1444441881444441",
        "1444441881444441",
        "1444441881444441",
    ],
    "harvester": [
        "................",
        ".....111111.....",
        "....11bbbb11....",
        "...1bbbbbbbb1...",
        "..1aaaaaaaaaa1..",
        "...1eeeeeeee1...",
        "...1171111711...",
        "...1eeeeeeee1...",
        "...1eee11eee1...",
        "....11eeee11....",
        "......1ee1......",
        "...1aaaeeaaa1...",
        "..1aaaaaaaaaa1..",
        ".1aafaaaaaafaa1.",
        "1aaafaaaaaafaaa1",
        "1aaafaaaaaafaaa1",
    ],
    "prophet": [
        "................",
        ".....111111.....",
        "...1177777711...",
        "..177777777771..",
        "..177eeeeee771..",
        "..17e81ee18e71..",
        "..17eeeeeeee71..",
        "..177e7777e771..",
        "..177779977771..",
        "..177777777771..",
        ".15517777771551.",
        "1555517777155551",
        "1555551771555551",
        "1555555115555551",
        "1555555555555551",
        "1555555555555551",
    ],
    "worm": [
        "......1111......",
        "....1aaaaaa1....",
        "..11a555555a11..",
        "..1a55888855a1..",
        ".1a5883883885a1.",
        ".a558838838855a.",
        "1a588331133885a1",
        "1a588311113885a1",
        "1a588311113885a1",
        "1a588331133885a1",
        ".a558838838855a.",
        ".1a5883883885a1.",
        "..1a55888855a1..",
        "..11a555555a11..",
        "....1aaaaaa1....",
        "......1111......",
    ],
    "stranger": [
        "................",
        ".......11.......",
        ".....116611.....",
        "...1666666661...",
        "...1611111161...",
        "...1618888161...",
        "...1678888761...",
        "...1678888761...",
        "...1618888161...",
        "...1661881661...",
        "..166661166661..",
        ".16666666666661.",
        "1666666666666661",
        "1666666666666661",
        "1666666666666661",
        "1666666666666661",
    ],
    "swordmaster": [
        "............111.",
        ".....111111.111.",
        "....17777771151.",
        "....179eee71151.",
        "....169ee661661.",
        "....1e1ee1e1....",
        "....1eeeeee1....",
        "....17711771....",
        "....17777771....",
        "..1ff777777ff1..",
        ".1fff177771fff1.",
        ".fffff1771fffff.",
        "1ffffff11ffffff1",
        "1ffffffffffffff1",
        "1ffffffffffffff1",
        "1ffffffffffffff1",
    ],
}

CARD_BACK16 = [
    "......1111......",
    "...1bb1111bb1...",
    "..111222222111..",
    ".11122222222111.",
    ".b1222aaaa2221b.",
    ".b222abbbba222b.",
    "11222bbbbbb22211",
    "11222bbbbbb22211",
    "11222abbbbb22211",
    "11222a55aaaa5211",
    ".b2225aaaaaaa5b.",
    ".b15555aaaaaa1b.",
    ".11155555aaa111.",
    "..111555555111..",
    "...1bb1111bb1...",
    "......1111......",
]

# 8x8 battle sprites, two frames each (walk / attack), facing UP toward the enemy
# (the engine flips them for the enemy side).
# '.' transparent, '#' team colour (engine substitutes gold/red), '+' dark outline,
# 'o' light highlight.
UNITS = {
    "levy": [
        [
            "........",
            "......o.",
            "..+#+.#.",
            ".+#o#+#.",
            ".+###+#.",
            ".+#####.",
            ".+###.#.",
            "..#.#.+.",
        ],
        [
            "......o.",
            "......#.",
            "..+#+.#.",
            ".+#o#+#.",
            ".+#####.",
            ".+###+..",
            "..###...",
            ".+#..#+.",
        ],
    ],
    "trooper": [
        [
            "...+##+.",
            "++.#oo#.",
            "o#+####+",
            "o#+####+",
            "o#+####+",
            "++.####.",
            "...#..#.",
            "..++..++",
        ],
        [
            "++.+##+.",
            "o#+#oo#.",
            "o#+####+",
            "o#+####+",
            "++.####+",
            "...####.",
            "..#..#..",
            "..++..++",
        ],
    ],
    "rifleman": [
        [
            "....#...",
            "....#...",
            "..+#+...",
            "..#o#+..",
            ".+###+..",
            "..###...",
            "..#.#...",
            ".++.++..",
        ],
        [
            "...ooo..",
            "....#...",
            "..+#+...",
            "..#o#+..",
            ".+###+..",
            "..###...",
            "..#.#...",
            ".++.++..",
        ],
    ],
    "sandrider": [
        [
            "...++...",
            "..+##+..",
            "..#oo#..",
            ".+####+.",
            "+#+##+#+",
            ".+####+.",
            "..+..+..",
            ".o.o..o.",
        ],
        [
            "...++...",
            "..+##+..",
            "..#oo#..",
            ".+####+.",
            "+#+##+#+",
            ".+####+.",
            "..+..+..",
            "o..o.o.o",
        ],
    ],
    "thopter": [
        [
            "...##...",
            "ooo##ooo",
            ".++##++.",
            "...##...",
            "oo+##+oo",
            "..+##+..",
            "...++...",
            "...++...",
        ],
        [
            "...##...",
            "...##...",
            ".o+##+o.",
            "o.+##+.o",
            "..o##o..",
            ".o+##+o.",
            "o..++..o",
            "...++...",
        ],
    ],
    "champion": [
        [
            "......oo",
            "o..++.o+",
            "o#+##+#+",
            "########",
            "+######+",
            "+#####++",
            ".##.##..",
            ".++.++..",
        ],
        [
            "..oooo..",
            "o..++..o",
            "o#+##+#o",
            "########",
            "+######+",
            "+######+",
            "..##.##.",
            "..++.++.",
        ],
    ],
}


if __name__ == "__main__":
    import sys

    PIX = ".123456789abcdef"
    IDS = ["advisor", "sister", "naib", "marshal", "envoy", "navigator", "baron", "smuggler",
           "planetologist", "heir", "swordmaster", "harvester", "prophet", "worm", "consort",
           "physician", "stranger"]
    errs = []

    def grid(name, rows, n, chars):
        if len(rows) != n:
            errs.append("%s: %d rows, expected %d" % (name, len(rows), n))
        for y, r in enumerate(rows):
            if len(r) != n:
                errs.append("%s row %d: width %d, expected %d" % (name, y, len(r), n))
            bad = set(r) - set(chars)
            if bad:
                errs.append("%s row %d: bad chars %s" % (name, y, "".join(sorted(bad))))

    if len(PALETTE) != 16 or PALETTE[0] is not None:
        errs.append("PALETTE must have 16 entries with index 0 = None")
    for c in PALETTE[1:]:
        if not (isinstance(c, tuple) and len(c) == 3 and all(0 <= v <= 255 for v in c)):
            errs.append("bad palette entry %r" % (c,))
    if [c[0] for c in CHARACTERS] != IDS:
        errs.append("character ids/order mismatch: %r" % [c[0] for c in CHARACTERS])
    for cid, name in CHARACTERS:
        if not (0 < len(name) <= 20):
            errs.append("%s: name %r length %d" % (cid, name, len(name)))
    P16 = ["advisor", "sister", "naib", "marshal", "envoy", "navigator", "baron", "smuggler",
           "planetologist", "harvester", "prophet", "worm", "stranger", "swordmaster"]
    if list(PORTRAITS16) != P16:
        errs.append("PORTRAITS16 keys/order: %r" % list(PORTRAITS16))
    for k, rows in PORTRAITS16.items():
        if k not in IDS:
            errs.append("PORTRAITS16 %s: unknown character id" % k)
        grid("portrait16 " + k, rows, 16, PIX)
        if not any(c not in ".1" for r in rows for c in r):
            errs.append("portrait16 %s: no colour" % k)
    grid("CARD_BACK16", CARD_BACK16, 16, PIX)

    def keys(name, d, want):
        if list(d) != want:
            errs.append("%s keys/order: %r, expected %r" % (name, list(d), want))

    keys("UNITS", UNITS, ["levy", "trooper", "rifleman", "sandrider", "thopter", "champion"])
    for k, frames in UNITS.items():
        if len(frames) != 2:
            errs.append("unit %s: %d frames, expected 2" % (k, len(frames)))
        for f, rows in enumerate(frames):
            grid("unit %s f%d" % (k, f), rows, 8, ".#+o")
        if len(frames) == 2 and frames[0] == frames[1]:
            errs.append("unit %s: frames identical" % k)
    sils = {}
    for k, frames in UNITS.items():
        sil = tuple("".join("." if c == "." else "x" for c in r) for r in frames[0])
        if sil in sils:
            errs.append("unit %s: same silhouette as %s" % (k, sils[sil]))
        sils[sil] = k
    if errs:
        print("\n".join(errs))
        sys.exit(1)
    print("art.py OK: %d characters, %d portraits16, %d units"
          % (len(CHARACTERS), len(PORTRAITS16), len(UNITS)))
