"""SPICE THRONE lane-defence petition deck: one Reigns-style perk card between waves.

Pure data. Run directly to validate every content constraint and print stats.

Effects (fx, 1-2 per side): unlock:<unit>, hp:<unit>, atk:<unit>, cost:<unit>,
income, spice+N (1..3), wall+N (1..3), heal+N (1..5), hurt+N (1..5),
harder+N (1..3), worm, wormrate, nothing (sole effect of a refuse side only).
"""

CARDS = [
    # ------------------------------------------------------------ unlocks
    dict(id="naib_riders", who="naib",
         text="My riders will fight beside you, $R. The Guild hates that we are friends.",
         L=dict(label="Ride with us", fx=["unlock:sandrider", "harder+1"]),
         R=dict(label="Not today", fx=["spice+1"]),
         when=["!has:sandrider"], wave=2, once=True),
    dict(id="prophet_faithful", who="prophet",
         text="The faithful will ride the sand for you, Regent. Only bleed a little for the cause.",
         L=dict(label="Bless the riders", fx=["unlock:sandrider", "hurt+2"]),
         R=dict(label="Silence him", fx=["wall+1"]),
         when=["!has:sandrider"], wave=4, once=True),
    dict(id="smuggler_rifles", who="smuggler",
         text="Off-world rifles, $R. No marks, no questions. Vulkar will hear and arm his men too.",
         L=dict(label="Buy the rifles", fx=["unlock:rifleman", "harder+1"]),
         R=dict(label="Report her", fx=["spice+1"]),
         when=["!has:rifleman"], wave=2, once=True),
    dict(id="envoy_rifles", who="envoy",
         text="The Emperor lends you a rifle company, $R. His engineers take your best wall stones.",
         L=dict(label="Accept the guns", fx=["unlock:rifleman", "hurt+2"]),
         R=dict(label="Decline politely", fx=["wall+1"]),
         when=["!has:rifleman"], wave=3, once=True),
    dict(id="navigator_thopters", who="navigator",
         text="The Guild will fly thopters to your gate, Regent. Vulkar buys from us too, of course.",
         L=dict(label="Buy thopters", fx=["unlock:thopter", "harder+1"]),
         R=dict(label="Keep our spice", fx=["spice+1"]),
         when=["!has:thopter"], wave=3, once=True),
    dict(id="harvester_thopters", who="harvester",
         text="Strip my spotter thopters for war, $R. The crawler crews will riot at your gate.",
         L=dict(label="Arm the spotters", fx=["unlock:thopter", "hurt+2"]),
         R=dict(label="Keep them flying", fx=["income"]),
         when=["!has:thopter"], wave=5, once=True),

    # ------------------------------------------------------- unit upgrades
    dict(id="swordmaster_drill", who="swordmaster",
         text="Your troopers need drill, $R. Blade-work or shield-work? I have time for one.",
         L=dict(label="Drill the blades", fx=["atk:trooper"]),
         R=dict(label="Drill shields", fx=["hp:trooper"]),
         when=[], wave=1, once=True),
    dict(id="marshal_levy", who="marshal",
         text="Peasants will join the levy for bread. Cheap spears, or fewer spears with armor?",
         L=dict(label="Cheap spears", fx=["cost:levy"]),
         R=dict(label="Armor them", fx=["hp:levy"]),
         when=[], wave=1, once=True),
    dict(id="smuggler_scopes", who="smuggler",
         text="Long-glass scopes off a wrecked frigate. Your riflemen will never miss. Hot goods.",
         L=dict(label="Fit the scopes", fx=["atk:rifleman", "harder+1"]),
         R=dict(label="Sell the glass", fx=["spice+1"]),
         when=["has:rifleman"], wave=4, once=True),
    dict(id="advisor_rifles", who="advisor",
         text="We can halve the cost of rifles if the men go without armor, $R. Or arm them well.",
         L=dict(label="Cheap rifles", fx=["cost:rifleman"]),
         R=dict(label="Sturdy rifles", fx=["hp:rifleman"]),
         when=["has:rifleman"], wave=4, once=True),
    dict(id="naib_stillsuits", who="naib",
         text="My riders need stillsuits to last, or light saddles to come cheap. Choose, $R.",
         L=dict(label="Stillsuits", fx=["hp:sandrider"]),
         R=dict(label="Light saddles", fx=["cost:sandrider"]),
         when=["has:sandrider"], wave=4, once=True),
    dict(id="prophet_fury", who="prophet",
         text="Let my preachers ride with your sandriders. They fight like madmen. They bleed you too.",
         L=dict(label="Send preachers", fx=["atk:sandrider", "hurt+1"]),
         R=dict(label="No fanatics", fx=["heal+2"]),
         when=["has:sandrider"], wave=5, once=True),
    dict(id="navigator_rockets", who="navigator",
         text="Guild rockets for your thopters, Regent. Vulkar will want rockets too. We will sell.",
         L=dict(label="Mount rockets", fx=["atk:thopter", "harder+1"]),
         R=dict(label="Armor plates", fx=["hp:thopter"]),
         when=["has:thopter"], wave=5, once=True),
    dict(id="harvester_pilots", who="harvester",
         text="My crawler pilots fly cheap, $R. They crash into walls sometimes. Yours, mostly.",
         L=dict(label="Hire my pilots", fx=["cost:thopter", "hurt+1"]),
         R=dict(label="Trained pilots", fx=["hp:thopter"]),
         when=["has:thopter"], wave=5, once=True),
    dict(id="marshal_deserters", who="marshal",
         text="Vulkar's deserters beg for work. Cheap soldiers, $R. Some may still be his spies.",
         L=dict(label="Hire them", fx=["cost:trooper", "harder+1"]),
         R=dict(label="Hang them", fx=["spice+1"]),
         when=[], wave=4, once=True),
    dict(id="sister_levy", who="sister",
         text="We can teach your levies our fighting arts. The lessons are paid for in blood.",
         L=dict(label="Pay in blood", fx=["atk:levy", "hurt+2"]),
         R=dict(label="Plain drills", fx=["hp:levy"]),
         when=[], wave=8, once=True),

    # ---------------------------------------------------------- economy
    dict(id="guild_contract", who="navigator",
         text="Sign the Guild's spice contract. A steady flow, but every House will hear of your wealth.",
         L=dict(label="Sign it", fx=["income", "harder+1"]),
         R=dict(label="Sell on the side", fx=["spice+1"]),
         when=[], wave=2, once=True),
    dict(id="smuggler_cache", who="smuggler",
         text="I hid spice under your own wall, Regent. Digging it out will crack a few stones.",
         L=dict(label="Dig it out", fx=["spice+3", "hurt+2"]),
         R=dict(label="Leave it buried", fx=["wall+1"]),
         when=[], wave=3, once=True),
    dict(id="harvester_shifts", who="harvester",
         text="Double shifts on the crawlers! The crews will curse you, but the silos will fill.",
         L=dict(label="Double shifts", fx=["income", "hurt+1"]),
         R=dict(label="Rest the crews", fx=["heal+2"]),
         when=[], wave=3, once=True),
    dict(id="envoy_tithe", who="envoy",
         text="The Emperor wants his tithe, $R. Pay, and his masons mend your gate. Refuse at peril.",
         L=dict(label="Pay the tithe", fx=["heal+3"]),
         R=dict(label="Keep the spice", fx=["spice+2", "harder+1"]),
         when=[], wave=2, once=True),
    dict(id="navigator_final", who="navigator",
         text="A last Guild contract, Regent: endless spice. Vulkar gets Guild armor in the same deal.",
         L=dict(label="Sign it", fx=["income", "harder+2"]),
         R=dict(label="Refuse", fx=["nothing"]),
         when=[], wave=12, once=True),

    # ---------------------------------------------------------- citadel
    dict(id="advisor_reserves", who="advisor",
         text="The reserve can mend the wall or pay the troops, $R. Not both, I fear.",
         L=dict(label="Mend the walls", fx=["heal+3"]),
         R=dict(label="Pay the troops", fx=["spice+2"]),
         when=[], wave=1, once=False),
    dict(id="marshal_walls", who="marshal",
         text="Masons or soldiers' pay, $R? The treasury covers only one this season.",
         L=dict(label="Raise the walls", fx=["wall+2"]),
         R=dict(label="Pay the soldiers", fx=["spice+2"]),
         when=[], wave=2, once=False),
    dict(id="planet_windtraps", who="planetologist",
         text="Windtraps on the wall could water your troopers or harden the stones. Not both.",
         L=dict(label="Harden stones", fx=["wall+2"]),
         R=dict(label="Water the troops", fx=["hp:trooper"]),
         when=[], wave=3, once=True),
    dict(id="sister_healers", who="sister",
         text="Our healers will mend your gate, $R. Some say we sell battle plans. Lies, surely.",
         L=dict(label="Accept a Sister", fx=["heal+4", "harder+1"]),
         R=dict(label="No witches", fx=["spice+1"]),
         when=[], wave=4, once=True),
    dict(id="envoy_elite", who="envoy",
         text="His Majesty lends elite guards for your gate. His spies come with them, of course.",
         L=dict(label="Accept guards", fx=["wall+3", "harder+1"]),
         R=dict(label="Decline", fx=["heal+2"]),
         when=[], wave=9, once=True),
    dict(id="advisor_war", who="advisor",
         text="Vulkar throws everything at us now, $R. The treasury goes to walls or to war.",
         L=dict(label="All on walls", fx=["wall+3"]),
         R=dict(label="All on war", fx=["spice+3"]),
         when=[], wave=10, once=True),

    # ------------------------------------------------------------- worm
    dict(id="worm_omen", who="worm",
         text="The Old Worm rises beside your wall. It watches you a long moment. It remembers.",
         L=dict(label="Bow to it", fx=["wormrate"]),
         R=dict(label="Ignore it", fx=["spice+1"]),
         when=[], wave=2, once=True),
    dict(id="naib_thumpers", who="naib",
         text="Plant thumpers in the dunes before the next wave, $R? The Maker will come hungry.",
         L=dict(label="Plant thumpers", fx=["worm"]),
         R=dict(label="Keep it quiet", fx=["heal+2"]),
         when=[], wave=3, once=False),
    dict(id="prophet_blood", who="prophet",
         text="The Maker answers my prayers, Regent. Offer it some of your blood and it answers yours.",
         L=dict(label="Offer blood", fx=["wormrate", "hurt+2"]),
         R=dict(label="Refuse him", fx=["nothing"]),
         when=[], wave=5, once=True),
    dict(id="planet_rhythm", who="planetologist",
         text="I can map the Maker's rhythm, $R, or map the cracks in your wall. One study only.",
         L=dict(label="Map the Maker", fx=["wormrate"]),
         R=dict(label="Map the cracks", fx=["wall+2"]),
         when=[], wave=6, once=True),
    dict(id="worm_hunger", who="worm",
         text="The Old Worm circles your citadel, hungry. It may eat Vulkar's host, or your gate.",
         L=dict(label="Lure it close", fx=["worm", "hurt+2"]),
         R=dict(label="Drive it off", fx=["wall+1"]),
         when=[], wave=7, once=True),
    dict(id="stranger_thumper", who="stranger",
         text="A stranger left a jewelled thumper at your gate. Below the sand, something stirs.",
         L=dict(label="Strike it", fx=["worm"]),
         R=dict(label="Sell the jewels", fx=["spice+2"]),
         when=[], wave=6, once=True),

    # ---------------------------------------------------- Vulkar / intrigue
    dict(id="baron_taunt", who="baron",
         text="Your soldiers are cowards, little Regent. I will prove it, wave after wave.",
         L=dict(label="Make them angry", fx=["atk:trooper", "harder+1"]),
         R=dict(label="Steady the men", fx=["hp:trooper"]),
         when=[], wave=3, once=True),
    dict(id="swordmaster_champion", who="swordmaster",
         text="Vulkar's champion calls ours out. If I win, the levy fights like lions. If I lose...",
         L=dict(label="Go, Halder", fx=["atk:levy", "hurt+2"]),
         R=dict(label="Stay on the wall", fx=["heal+2"]),
         when=[], wave=4, once=True),
    dict(id="baron_armor", who="baron",
         text="I have bought Guild armor for my men, Regent. Pray your walls are thick.",
         L=dict(label="Thicken walls", fx=["wall+3", "harder+1"]),
         R=dict(label="Sharpen blades", fx=["atk:trooper", "harder+1"]),
         when=[], wave=8, once=True),
    dict(id="stranger_plans", who="stranger",
         text="A faceless man sells Vulkar's battle plans. His price: free run of your halls.",
         L=dict(label="Take the plans", fx=["atk:trooper", "hurt+2"]),
         R=dict(label="Unmask him", fx=["heal+1"]),
         when=[], wave=5, once=True),
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
    UNITS = ("levy", "trooper", "rifleman", "sandrider", "thopter")
    STARTERS = {"levy", "trooper"}
    UNLOCKABLE = [u for u in UNITS if u not in STARTERS]
    RANGES = {"spice": (1, 3), "wall": (1, 3), "heal": (1, 5), "hurt": (1, 5),
              "harder": (1, 3)}
    COSTS = {"hurt", "harder"}
    CARD_KEYS = {"id", "who", "text", "L", "R", "when", "wave", "once"}
    SIDE_KEYS = {"label", "fx"}
    FX_RE = re.compile(r"^(?:(unlock|hp|atk|cost):([a-z]+)|(spice|wall|heal|hurt|harder)\+(\d+)"
                       r"|(income|worm|wormrate|nothing))$")
    WHEN_RE = re.compile(r"^(!?)has:([a-z]+)$")
    NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")

    def ascii_ok(s):
        return isinstance(s, str) and all(32 <= ord(c) <= 126 for c in s)

    def expand(s):
        return s.replace("$R", "R" * 11)

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

    def fx_kind(f):
        """Return (kind, unit_or_None) for a valid effect, else None."""
        m = FX_RE.match(f) if isinstance(f, str) else None
        if not m:
            return None
        if m.group(1):
            return (m.group(1), m.group(2)) if m.group(2) in UNITS else None
        if m.group(3):
            lo, hi = RANGES[m.group(3)]
            return (m.group(3), None) if lo <= int(m.group(4)) <= hi else None
        return (m.group(5), None)

    if not 30 <= len(CARDS) <= 40:
        err("card count %d not in 30..40" % len(CARDS))
    seen = set()
    fx_count = {}
    per_who = {}
    unlock_cards = {u: [] for u in UNLOCKABLE}
    text_bytes = 0

    for c in CARDS:
        cid = c.get("id", "?")
        if set(c) != CARD_KEYS:
            err("%s: keys must be exactly %s" % (cid, sorted(CARD_KEYS)))
        if not isinstance(cid, str) or not NAME_RE.match(cid):
            err("%s: id not snake_case" % cid)
        if cid in seen:
            err("duplicate id %s" % cid)
        seen.add(cid)
        who = c.get("who")
        if who not in CHARACTERS:
            err("%s: unknown who %r" % (cid, who))
        per_who[who] = per_who.get(who, 0) + 1
        text = c.get("text", "")
        if not ascii_ok(text) or len(expand(text)) > 100:
            err("%s: text not ASCII or too long (%d)" % (cid, len(expand(text))))
        lines = wrap_lines(text)
        if lines is None or len(lines) > 5:
            err("%s: text wraps to %s lines" % (cid, "?" if lines is None else len(lines)))
        text_bytes += len(text)
        wave = c.get("wave")
        if not isinstance(wave, int) or isinstance(wave, bool) or not 1 <= wave <= 20:
            err("%s: wave %r not in 1..20" % (cid, wave))
        if not isinstance(c.get("once"), bool):
            err("%s: once must be bool" % cid)
        when = c.get("when")
        if not isinstance(when, list) or len(when) > 2:
            err("%s: when must be a list of <=2" % cid)
            when = []
        has, has_not = set(), set()
        for w in when:
            m = WHEN_RE.match(w) if isinstance(w, str) else None
            if not m or m.group(2) not in UNITS:
                err("%s: bad when %r" % (cid, w))
                continue
            if m.group(2) in STARTERS:
                err("%s: %s is a starter unit; condition is constant" % (cid, w))
            (has_not if m.group(1) else has).add(m.group(2))
        if has & has_not:
            err("%s: contradictory when" % cid)

        kinds = {}
        for s in "LR":
            side = c.get(s)
            if not isinstance(side, dict):
                err("%s: missing side %s" % (cid, s))
                continue
            if set(side) != SIDE_KEYS:
                err("%s.%s: keys must be exactly label, fx" % (cid, s))
            lab = side.get("label")
            if not lab or not ascii_ok(lab) or len(lab) > 16:
                err("%s.%s: bad label %r" % (cid, s, lab))
            else:
                text_bytes += len(lab)
            fx = side.get("fx")
            if not isinstance(fx, list) or not 1 <= len(fx) <= 2:
                err("%s.%s: fx must be a list of 1-2 effects" % (cid, s))
                fx = []
            if len(set(fx)) != len(fx):
                err("%s.%s: duplicate fx" % (cid, s))
            ks = []
            for f in fx:
                k = fx_kind(f)
                if k is None:
                    err("%s.%s: bad fx %r" % (cid, s, f))
                    continue
                ks.append(k)
                key = f if k[1] else k[0]
                fx_count[key] = fx_count.get(key, 0) + 1
                kind, unit = k
                if kind == "unlock":
                    if unit in STARTERS:
                        err("%s.%s: %s is already unlocked at start" % (cid, s, unit))
                    elif unit not in has_not:
                        err("%s.%s: unlock:%s needs when !has:%s" % (cid, s, unit, unit))
                    else:
                        unlock_cards[unit].append((cid, wave))
                elif kind in ("hp", "atk", "cost") and unit not in STARTERS:
                    unlocked_here = any(g == "unlock:" + unit for g in fx)
                    if unit not in has and not unlocked_here:
                        err("%s.%s: %s needs when has:%s" % (cid, s, f, unit))
            kinds[s] = ks
        if len(kinds) == 2:
            for s, o in (("L", "R"), ("R", "L")):
                if ("nothing", None) in kinds[s]:
                    if len(kinds[s]) != 1:
                        err("%s.%s: 'nothing' must be the sole effect" % (cid, s))
                    if not any(k[0] in COSTS for k in kinds[o]):
                        err("%s.%s: 'nothing' needs a trade-off (hurt/harder) on the other side"
                            % (cid, s))
            if sorted(c["L"]["fx"]) == sorted(c["R"]["fx"]):
                err("%s: both sides identical" % cid)
            boons = [any(k[0] not in COSTS and k[0] != "nothing" for k in kinds[s]) for s in "LR"]
            if not any(boons):
                err("%s: neither side offers a boon" % cid)

    for u in UNLOCKABLE:
        early = [cid for cid, w in unlock_cards[u] if isinstance(w, int) and 2 <= w <= 6]
        if len(early) < 2:
            err("unit %s needs >=2 unlock cards with wave 2..6, has %s" % (u, early))

    # ---- stats
    print("cards: %d" % len(CARDS))
    print("unlocks: " + ", ".join("%s via %s" % (u, "/".join("%s@w%d" % x for x in unlock_cards[u]))
                                  for u in UNLOCKABLE))
    print("effects:")
    for k in sorted(fx_count):
        print("  %-18s %d" % (k, fx_count[k]))
    print("per character:")
    for who in sorted(per_who):
        print("  %-14s %d" % (who, per_who[who]))
    print("text bytes: %d (texts + labels)" % text_bytes)
    if errors:
        print("\n%d ERRORS:" % len(errors))
        for e in errors:
            print("  " + e)
        sys.exit(1)
    print("OK")
