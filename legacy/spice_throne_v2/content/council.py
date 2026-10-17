"""SPICE THRONE v2 Council deck: petitioners arrive every ~75 s and step in when you are idle.

Pure data. Run directly to validate every content constraint and print stats.

Side effects: influence meters o (Sisterhood), t (Tribes), l (Legion morale),
g (Guild standing); resources spice/water/plasteel; troops="<unit><+|-><n>";
set/clear flag; next=card id; do=charge+1|charge+2|attack|lose_node|gain_node|
win|lose|chance.
"""

CARDS = [
    # ----------------------------------------------------------------- opener
    dict(id="opening", who="advisor",
         text="Kessar is ours, $R. Vulkar's spies watch the spaceport. Where does our first spice go?",
         L=dict(label="Walls and soldiers", l=10, spice=-50, troops="trooper+4"),
         R=dict(label="Gifts to tribes", t=15, water=-50),
         when=["first"], weight=2, once=True, chain=False, priority=True),

    # ----------------------------------------------------------- meter crises
    dict(id="crisis_sister_high", who="sister",
         text="Your council answers to us now, $R. You sign what we write. Unless you resist?",
         L=dict(label="Expel the Sisters", o=-35, l=10, troops="trooper-4"),
         R=dict(label="Bow to them", o=-20, g=-10, do="lose_node"),
         when=["o>=95"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_sister_low", who="sister",
         text="The Sisterhood names you heretic. Every outpost priest has stopped the water rites.",
         L=dict(label="Do penance", o=25, spice=-150),
         R=dict(label="Cede a temple", o=20, do="lose_node"),
         when=["o<20"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_tribes_high", who="advisor",
         text="The Emperor fears your native host, $R. Hand him their chiefs, or disband it.",
         L=dict(label="Hand them over", t=-30, g=10, l=-5),
         R=dict(label="Disband the host", t=-20, water=100, troops="sandrider-8"),
         when=["t>=95"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_tribes_low", who="naib",
         text="Raiders hit your outposts every night now. The desert has chosen a side, $R.",
         L=dict(label="Pay blood-water", t=30, water=-200),
         R=dict(label="Cede an outpost", t=20, do="lose_node"),
         when=["t<20"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_legion_low", who="baron",
         text="Your soldiers go unpaid, $R. My recruiters offer double. Shall I take them all?",
         L=dict(label="Pay the arrears", l=25, spice=-250),
         R=dict(label="Let them desert", l=20, troops="trooper-10"),
         when=["l<20"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_legion_high", who="advisor",
         text="The troops chant Draxe's name, not yours. A coup is one dawn away, $R.",
         L=dict(label="Purge officers", l=-35, plasteel=100, troops="rifleman-5"),
         R=dict(label="Exile Draxe", l=-25, o=5, spice=-100),
         when=["l>=95"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_guild_low", who="navigator",
         text="Embargo. No Guild ship will lift your spice until the insult is paid in full.",
         L=dict(label="Pay the penalty", g=25, spice=-250),
         R=dict(label="Accept inspectors", g=20, l=-10, spice=-100),
         when=["g<20"], weight=2, once=False, chain=False, priority=True),
    dict(id="crisis_guild_high", who="navigator",
         text="The Guild holds your debts and your docks, Regent. We set your prices now.",
         L=dict(label="Buy them back", g=-20, spice=-250),
         R=dict(label="Seize the docks", g=-30, l=10, plasteel=200),
         when=["g>=95"], weight=2, once=False, chain=False, priority=True),

    # ------------------------------------------------------- arc: terraforming
    dict(id="terra_survey", who="planetologist",
         text="Kessar was green once, $R. Fund my survey and I will show you where the water hides.",
         L=dict(label="Fund the survey", t=10, plasteel=-100, set="terra1"),
         R=dict(label="Spice pays bills", g=10, spice=150),
         when=["day5", "!terra1"], weight=2, once=False, chain=False, priority=False),
    dict(id="terra_windtraps", who="planetologist",
         text="Windtraps on the Shield Wall could drink the dawn mist. I need plasteel. A mountain of it.",
         L=dict(label="Build windtraps", t=10, g=-5, plasteel=-250, water=200, set="terra2"),
         R=dict(label="Too costly", l=5, plasteel=50),
         when=["terra1", "!terra2", "day15"], weight=3, once=False, chain=False, priority=False),
    dict(id="terra_dew", who="planetologist",
         text="The tribes will plant dew-grass if you pay them in water now. It takes patience.",
         L=dict(label="Plant the grass", t=15, o=-5, water=-200, set="terra3"),
         R=dict(label="Sell the water", t=-10, spice=250),
         when=["terra2", "!terra3", "t>=50"], weight=3, once=False, chain=False, priority=False),
    dict(id="terra_seed", who="planetologist",
         text="Seed the open dunes with hardy shrubs. The worms will retreat. So will the spice.",
         L=dict(label="Seed the dunes", t=10, g=-10, plasteel=-200, spice=-150, set="terra4"),
         R=dict(label="Keep the spice", t=-10, g=10, spice=250),
         when=["terra3", "!terra4", "day30"], weight=3, once=False, chain=False, priority=False),
    dict(id="terra_rain", who="planetologist",
         text="Clouds gather over Kessar at last. Open the water reserves and let it rain.",
         L=dict(label="Let it rain", water=-300, plasteel=-200, do="win"),
         R=dict(label="Not yet", g=10, spice=150),
         when=["terra4", "t>=60"], weight=3, once=False, chain=False, priority=False),

    # ---------------------------------------------------------- arc: Vulkar
    dict(id="vulkar_threat", who="baron",
         text="Your manners bore me, $R. Send me 200 spice, or my legions will collect it.",
         L=dict(label="Pay him", l=-10, spice=-200),
         R=dict(label="Come and try", l=15, do="attack", set="vfeud"),
         when=["day5", "!war"], weight=2, once=False, chain=False, priority=False),
    dict(id="sword_training", who="naib",
         text="Vulkar's blade is fast, $R. Dance with our knives each dawn and yours will be faster.",
         L=dict(label="Train me", l=10, o=-5, spice=-50, set="trained"),
         R=dict(label="No time", l=-5, spice=100),
         when=["day5", "!trained"], weight=2, once=False, chain=False, priority=False),
    dict(id="vulkar_duel", who="baron",
         text="Single combat, $R, for all of Kessar. Kadar's grim face says you are not ready.",
         L=dict(label="Fight him now", do="lose"),
         R=dict(label="Refuse the duel", l=-15, o=5),
         when=["vfeud", "day15", "!trained"], weight=1, once=False, chain=False, priority=False),
    dict(id="vulkar_duel_ready", who="baron",
         text="Single combat, $R, for all of Kessar. Kadar nods to you: this time you are ready.",
         L=dict(label="Draw steel", l=20, g=-10, spice=300, clear="vfeud"),
         R=dict(label="Refuse the duel", l=-15, o=10),
         when=["vfeud", "day15", "trained"], weight=2, once=False, chain=False, priority=False),

    # ------------------------------------------------------------ arc: Guild
    dict(id="guild_monopoly", who="navigator",
         text="Sell your spice through the Guild alone. A fair price, safe passage, no questions.",
         L=dict(label="Sign it", g=20, spice=-200, plasteel=250),
         R=dict(label="Sell freely", g=-15, spice=250),
         when=["rich"], weight=2, once=False, chain=False, priority=False),
    dict(id="guild_smugglers", who="navigator",
         text="Smugglers carry your spice off-world. Stop them, Regent, or we stop you.",
         L=dict(label="Crack down", g=20, t=-10, troops="trooper-2"),
         R=dict(label="Protect them", g=-15, spice=200),
         when=["g<40", "fields"], weight=2, once=False, chain=False, priority=False),

    # -------------------------------------------------- arc: faceless stranger
    dict(id="stranger_arrives", who="stranger",
         text="A trader with forgettable eyes offers plasteel and asks to serve as your scribe.",
         L=dict(label="Hire him", o=-5, plasteel=100, water=100, set="faceless"),
         R=dict(label="Send him away", o=5),
         when=["day5", "!faceless"], weight=1, once=False, chain=False, priority=False),
    dict(id="stranger_doubt", who="advisor",
         text="The new scribe never blinks, $R. Shall I test him? It costs, and he may flee.",
         L=dict(label="Test him", o=10, spice=-50, clear="faceless"),
         R=dict(label="Leave him be", spice=100, do="chance", next="stranger_strike"),
         when=["faceless"], weight=3, once=False, chain=False, priority=False),
    dict(id="stranger_strike", who="stranger",
         text="Your scribe wore your captain's face. Tonight an outpost gate stood open for Vulkar.",
         L=dict(label="Hold the keep", l=-10, troops="trooper-8", clear="faceless"),
         R=dict(label="Save the garrison", l=5, do="lose_node", clear="faceless"),
         when=[], weight=2, once=False, chain=True, priority=False),

    # ----------------------------------------------------------- arc: Prophet
    dict(id="prophet_rises", who="prophet",
         text="I have seen you ride the Maker, Regent. Bless my sermons and the sand will rise.",
         L=dict(label="Bless him", t=15, o=-15, water=100, troops="sandrider+3", set="prophet"),
         R=dict(label="Silence him", t=-15, o=10),
         when=["day5", "t>=40", "!prophet"], weight=2, once=False, chain=False, priority=False),
    dict(id="prophet_war", who="prophet",
         text="The faithful burn to carry your name to the stars. Unleash them, $R!",
         L=dict(label="Unleash them", t=20, g=-20, o=-20, troops="sandrider+10"),
         R=dict(label="Restrain them", t=-20, l=5, clear="prophet"),
         when=["prophet", "day15"], weight=2, once=False, chain=False, priority=False),
    dict(id="worm_lure", who="prophet",
         text="Walk to the Maker alone, $R, with no rhythm. It will kneel to you. I have seen it.",
         L=dict(label="Walk out alone", do="lose"),
         R=dict(label="Stay on the rock", t=-10, o=10),
         when=["prophet"], weight=1, once=False, chain=False, priority=False),

    # --------------------------------------------------------- arc: smuggler
    dict(id="smug_water", who="smuggler",
         text="Fresh ice from the polar caps, $R. Cheap, as long as the Guild never asks.",
         L=dict(label="Buy the ice", g=-10, water=300, spice=-100),
         R=dict(label="Report her", g=15, spice=50),
         when=["day5"], weight=2, once=False, chain=False, priority=False),
    dict(id="smug_thopters", who="smuggler",
         text="Two Guild-stamped thopters, barely stolen. Yours for spice and a blind eye.",
         L=dict(label="Buy them", g=-10, spice=-150, troops="thopter+2"),
         R=dict(label="Pass", g=10, l=-5),
         when=["day15"], weight=2, once=False, chain=False, priority=False),

    # --------------------------------------------------- fields (smuggler/naib)
    dict(id="field_newblow", who="smuggler",
         text="My crews found a fresh spice blow out in worm country. Claim it before Ordane does?",
         L=dict(label="Claim it", troops="levy-4", do="gain_node"),
         R=dict(label="Sell the map", g=10, spice=150),
         when=["day5"], weight=3, once=False, chain=False, priority=False),
    dict(id="field_wormsign", who="naib",
         text="Wormsign at your field! A carryall can lift the crew or the crawler. Not both.",
         L=dict(label="Save the crew", t=15, l=5, plasteel=-100),
         R=dict(label="Save the crawler", t=-10, spice=250, plasteel=100, troops="levy-5"),
         when=["fields"], weight=2, once=False, chain=False, priority=False),

    # ------------------------------------------------------- arc: worm omens
    dict(id="worm_omen", who="worm",
         text="The Old Worm rises beside your walls. It watches you a long moment, then sinks.",
         L=dict(label="Bow to it", t=10, l=-10, do="charge+1"),
         R=dict(label="Fire on it", l=10, t=-15),
         when=["day5"], weight=2, once=False, chain=False, priority=False),
    dict(id="worm_feast", who="worm",
         text="The Maker swallowed a whole crawler. The tribes whisper that it chose you.",
         L=dict(label="Honor the sign", t=10, spice=-100, do="charge+2"),
         R=dict(label="Mourn the crew", l=5, t=-10, water=100),
         when=["fields", "day15"], weight=2, once=False, chain=False, priority=False),
]


if __name__ == "__main__":
    import re
    import sys

    errors = []

    def err(msg):
        errors.append(msg)

    CHARACTERS = {
        "advisor", "sister", "naib", "marshal", "envoy", "navigator", "baron",
        "smuggler", "planetologist", "harvester", "prophet", "worm", "stranger",
        "swordmaster",
    }
    BUILTIN = {"day5", "day15", "day30", "allied", "fields", "war", "seer",
               "first", "rich", "poor", "big_army"}
    METERS = "otlg"
    METER_NAMES = dict(o="SISTERHOOD", t="TRIBES", l="LEGION", g="GUILD")
    RESOURCES = ("spice", "water", "plasteel")
    UNITS = ("levy", "trooper", "rifleman", "sandrider", "thopter")
    DOS = {"charge+1", "charge+2", "attack", "lose_node", "gain_node", "win",
           "lose", "chance"}
    CARD_KEYS = {"id", "who", "text", "L", "R", "when", "weight", "once",
                 "chain", "priority"}
    SIDE_KEYS = {"label", "troops", "set", "clear", "next", "do"} | set(METERS) | set(RESOURCES)
    COND_RE = re.compile(r"^([otlg])(>=|<)(\d+)$")
    TROOP_RE = re.compile(r"^(%s)([+-])(\d+)$" % "|".join(UNITS))
    NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")
    CRISIS = ["o>=95", "o<20", "t>=95", "t<20", "l>=95", "l<20", "g>=95", "g<20"]

    def ascii_ok(s):
        return isinstance(s, str) and all(32 <= ord(c) <= 126 for c in s)

    def expand(s):
        return s.replace("$R", "R" * 11).replace("$D", "D" * 7)

    def wrap_lines(s, width=21):
        lines, cur = [], ""
        for word in expand(s).split(" "):
            if not word:
                continue
            if len(word) > width:
                return None
            if not cur:
                cur = word
            elif len(cur) + 1 + len(word) <= width:
                cur += " " + word
            else:
                lines.append(cur)
                cur = word
        if cur:
            lines.append(cur)
        return lines

    if not 30 <= len(CARDS) <= 36:
        err("card count %d not in 30..36" % len(CARDS))
    if len({c.get("who") for c in CARDS}) > 10:
        err("too many distinct characters: %d > 10" % len({c.get("who") for c in CARDS}))
    by_id = {}
    for c in CARDS:
        if c.get("id") in by_id:
            err("duplicate id %s" % c.get("id"))
        by_id[c.get("id")] = c

    flags_set, flags_clear, flags_when = set(), set(), set()
    next_targets = set()
    up = dict.fromkeys(METERS, 0)
    dn = dict.fromkeys(METERS, 0)
    res_sum = dict.fromkeys(RESOURCES, 0)
    troop_sum = dict.fromkeys(UNITS, 0)
    do_count = {}
    per_who = {}

    for c in CARDS:
        cid = c.get("id", "?")
        for k in c:
            if k not in CARD_KEYS:
                err("%s: unknown key %s" % (cid, k))
        for k in CARD_KEYS:
            if k not in c:
                err("%s: missing key %s" % (cid, k))
        if not isinstance(cid, str) or not NAME_RE.match(cid):
            err("%s: id not snake_case" % cid)
        who = c.get("who")
        if who not in CHARACTERS:
            err("%s: unknown who %r" % (cid, who))
        per_who[who] = per_who.get(who, 0) + 1
        text = c.get("text", "")
        if not ascii_ok(text):
            err("%s: text not printable ASCII" % cid)
        if len(expand(text)) > 100:
            err("%s: text too long (%d)" % (cid, len(expand(text))))
        lines = wrap_lines(text)
        if lines is None or len(lines) > 5:
            err("%s: text wraps to %s lines" % (cid, "?" if lines is None else len(lines)))
        w = c.get("weight")
        if not isinstance(w, int) or isinstance(w, bool) or not 1 <= w <= 4:
            err("%s: weight %r" % (cid, w))
        for bk in ("once", "chain", "priority"):
            if not isinstance(c.get(bk), bool):
                err("%s: %s must be bool" % (cid, bk))
        when = c.get("when")
        if not isinstance(when, list) or len(when) > 3:
            err("%s: when must be list of <=3" % cid)
            when = []
        if c.get("chain") and when:
            err("%s: chain card must not have when conditions" % cid)
        for w_ in when:
            m = COND_RE.match(w_)
            if m:
                x = int(m.group(3))
                if x % 5 or not 20 <= x <= 95:
                    err("%s: bad threshold %s" % (cid, w_))
                continue
            name = w_[1:] if w_.startswith("!") else w_
            if not NAME_RE.match(name):
                err("%s: bad when entry %r" % (cid, w_))
            elif name not in BUILTIN:
                flags_when.add(name)
        if c.get("priority"):
            tight = any(COND_RE.match(w_) or w_ == "first" or
                        (w_.lstrip("!") not in BUILTIN) for w_ in when)
            if not (tight and c.get("once") or any(w_ in CRISIS for w_ in when)):
                err("%s: priority card needs tight conditions" % cid)

        nums = {}
        for s in "LR":
            side = c.get(s)
            if not isinstance(side, dict):
                err("%s: missing side %s" % (cid, s))
                continue
            for k in side:
                if k not in SIDE_KEYS:
                    err("%s.%s: unknown key %s" % (cid, s, k))
            lab = side.get("label")
            if not lab or not ascii_ok(lab) or len(expand(lab)) > 18:
                err("%s.%s: bad label %r" % (cid, s, lab))
            num = {}
            for m in METERS:
                if m in side:
                    v = side[m]
                    if not isinstance(v, int) or v == 0 or v % 5 or not -40 <= v <= 35:
                        err("%s.%s: bad %s=%r" % (cid, s, m, v))
                        continue
                    num[m] = v
                    if v > 0:
                        up[m] += v
                    else:
                        dn[m] -= v
            for r in RESOURCES:
                if r in side:
                    v = side[r]
                    if not isinstance(v, int) or v == 0 or v % 10 or not -300 <= v <= 300:
                        err("%s.%s: bad %s=%r" % (cid, s, r, v))
                        continue
                    num[r] = v
                    res_sum[r] += v
            if "troops" in side:
                m = TROOP_RE.match(str(side["troops"]))
                if not m or not 1 <= int(m.group(3)) <= 15:
                    err("%s.%s: bad troops %r" % (cid, s, side["troops"]))
                else:
                    n = int(m.group(3)) * (1 if m.group(2) == "+" else -1)
                    troop_sum[m.group(1)] += n
            if "set" in side and "clear" in side:
                err("%s.%s: both set and clear" % (cid, s))
            for k, bucket in (("set", flags_set), ("clear", flags_clear)):
                if k in side:
                    f = side[k]
                    if f in BUILTIN or not NAME_RE.match(str(f)):
                        err("%s.%s: cannot %s %r" % (cid, s, k, f))
                    bucket.add(f)
            if "next" in side:
                next_targets.add(side["next"])
                if side["next"] not in by_id:
                    err("%s.%s: next %r missing" % (cid, s, side["next"]))
            if "do" in side:
                d = side["do"]
                if d not in DOS:
                    err("%s.%s: bad do %r" % (cid, s, d))
                do_count[d] = do_count.get(d, 0) + 1
                if d == "chance" and "next" not in side:
                    err("%s.%s: chance requires next" % (cid, s))
                if d == "win" and not any(f.lstrip("!") not in BUILTIN and not
                                          COND_RE.match(f) for f in when):
                    err("%s.%s: win must be flag-gated" % (cid, s))
            if not (set(side) - {"label", "next"}):
                err("%s.%s: side has no effect" % (cid, s))
            nums[s] = (num, set(side) - {"label"} - set(num))
        if len(nums) == 2:
            (a, ea), (b, eb) = nums["L"], nums["R"]
            if not ea and not eb:
                keys = set(a) | set(b)
                if all(a.get(k, 0) >= b.get(k, 0) for k in keys) or \
                   all(a.get(k, 0) <= b.get(k, 0) for k in keys):
                    err("%s: one side strictly dominates" % cid)

    # ---- global structure
    for f in flags_set | flags_clear:
        if not NAME_RE.match(str(f)):
            err("bad flag name %r" % f)
    for f in sorted((flags_when | flags_clear) - flags_set):
        err("flag %s used but never set" % f)
    for f in sorted(flags_set - flags_when):
        err("flag %s set but never tested" % f)
    for c in CARDS:
        if c.get("chain") and c["id"] not in next_targets:
            err("chain card %s is unreachable" % c["id"])
    for t in sorted(next_targets):
        if t in by_id and not by_id[t].get("chain"):
            err("next target %s should be a chain card" % t)

    openers = [c for c in CARDS if c["when"] == ["first"] and c["priority"] and c["once"]]
    if len(openers) != 1:
        err("need exactly one opener when=[first], priority, once")
    for cond in CRISIS:
        if not any(cond in c["when"] and c["priority"] for c in CARDS):
            err("missing crisis card for %s" % cond)
    crisis_n = sum(1 for c in CARDS if c["priority"] and any(w in CRISIS for w in c["when"]))
    if crisis_n != 8:
        err("expected 8 crisis cards, got %d" % crisis_n)
    if do_count.get("win", 0) != 1:
        err("exactly one win side required")
    if not 2 <= do_count.get("lose", 0) <= 3:
        err("need 2-3 lose sides, got %d" % do_count.get("lose", 0))
    terra = [f for f in flags_set if f.startswith("terra")]
    if len(terra) < 4:
        err("terraforming arc needs >=4 stage flags")
    for d in ("charge+1", "attack", "lose_node", "gain_node", "chance"):
        if not do_count.get(d):
            err("do %s never used" % d)
    for u in UNITS:
        if not any(u in str(c[s].get("troops", "")) for c in CARDS for s in "LR"):
            err("unit %s never appears" % u)

    for m in METERS:
        a, b = up[m], dn[m]
        if abs(a - b) > 0.20 * max(a, b):
            err("meter %s unbalanced: up %d down %d" % (m, a, b))
    total_res = sum(res_sum.values())
    if not 0 <= total_res <= 1000:
        err("resource total %+d not neutral-to-slightly-positive (0..+1000)" % total_res)
    for r in RESOURCES:
        if not -500 <= res_sum[r] <= 800:
            err("resource %s sum %+d out of -500..+800" % (r, res_sum[r]))

    # ---- stats
    print("cards: %d  flags: %s" % (len(CARDS), ", ".join(sorted(flags_set))))
    print("meters (up / down):")
    for m in METERS:
        print("  %-10s up %4d  down %4d  net %+d" % (METER_NAMES[m], up[m], dn[m], up[m] - dn[m]))
    print("resources (sum over all sides):")
    for r in RESOURCES:
        print("  %-9s %+d" % (r, res_sum[r]))
    print("  %-9s %+d" % ("total", total_res))
    print("troops: " + ", ".join("%s %+d" % (u, troop_sum[u]) for u in UNITS))
    print("do: " + ", ".join("%s x%d" % (d, do_count[d]) for d in sorted(do_count)))
    print("per character:")
    for who in sorted(per_who):
        print("  %-14s %d" % (who, per_who[who]))
    text_bytes = sum(len(c["text"]) for c in CARDS)
    label_bytes = sum(len(c[s]["label"]) for c in CARDS for s in "LR")
    print("text bytes: %d (texts %d + labels %d)" % (text_bytes + label_bytes, text_bytes, label_bytes))
    if errors:
        print("\n%d ERRORS:" % len(errors))
        for e in errors:
            print("  " + e)
        sys.exit(1)
    print("OK")
