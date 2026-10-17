"""SPICE THRONE card content: fates, dynasty flags and the card deck.

Pure data. Run directly to validate every content constraint and print stats.
"""

FATES = [
    ("order_low", "Heretic's Cup",
     "The Sisterhood named you an abomination. The supper wine tasted of almonds. Your truth-reader smiled."),
    ("order_high", "The Puppet Throne",
     "The Sisterhood owned every word you spoke. One night they decided a quieter Regent would serve. You never woke."),
    ("tribes_low", "Swallowed by Sand",
     "The tribes rose as one. They took your water, then your palace, then you. Your body feeds a dune now."),
    ("tribes_high", "Gone Native",
     "You took a tribal name and walked into the deep desert. The Emperor named a new Regent. You did not care."),
    ("legion_low", "Open Gates",
     "No soldier stood on the wall when Vulkar's men came. They hung you from your own balcony."),
    ("legion_high", "The Marshal's Knife",
     "The army loved Draxe more than you. A short coup, a long fall from the tower."),
    ("spice_low", "Empty Coffers",
     "No spice, no tribute. The Emperor sent his elite to collect what remained of the fief. That was you."),
    ("spice_high", "A Gift of Silk",
     "So much spice that the Emperor grew afraid. His gift arrived: a silk robe, laced with poison."),
    ("worm", "The Maker's Mouth",
     "You walked without the rhythm. The Maker heard. Somewhere beneath the dunes, you are still falling."),
    ("old_age", "A Quiet Death",
     "You died in bed, old and tired, an heir at your side. On Kessar, that is the rarest fate of all."),
    ("duel", "The Left Hand",
     "Vulkar's blade came from the left. You never saw it. He wiped it clean on your banner."),
    ("faceless", "A Friend's Face",
     "The thing wearing your friend's face poured the wine. Now it wears yours, and rules rather well."),
    ("hunter_seeker", "The Needle",
     "A sliver of metal, no longer than a finger, followed the motion of your breath."),
    ("holy_war", "The Holy War",
     "Your word loosed the tribes on a thousand worlds. Billions died in your name. Then they came for you."),
    ("storm", "Into the Storm",
     "The storm took your ornithopter and scoured it to bright metal. They found a single boot."),
    ("green_world", "The Green World",
     "Rain falls on Kessar. Rivers run where dunes stood. The worms retreat and House $D rules a garden. You won."),
]

FLAGS_DYNASTY = [
    "d_survey", "d_windtraps", "d_dew", "d_pact",
    "d_heir_desert", "d_heir_sword", "d_heir_sister",
    "d_guild_pact",
]

CARDS = [
    # ---------------------------------------------------------------- tutorial
    dict(id="tut_1", who="advisor",
         text="I am Vael, your Mentat. Tap LEFT or RIGHT to lean to an answer; press again or POWER to decide.",
         L=dict(label="Like this?", next="tut_2"),
         R=dict(label="Understood", next="tut_2"),
         when=["tutorial", "newreign"], priority=True, once=True),
    dict(id="tut_2", who="advisor",
         text="The yellow light is a spice vision. Press POWER before leaning and it shows what each choice does.",
         L=dict(label="A seer's trick", next="tut_3"),
         R=dict(label="Useful", next="tut_3"),
         chain=True),
    dict(id="tut_3", who="advisor",
         text="The green lights are your bloodline. When all go dark, House $D is dust. Shall we begin?",
         L=dict(label="Begin", next="coronation_first"),
         R=dict(label="Let us rule", next="coronation_first"),
         chain=True),

    # ------------------------------------------------------------- coronation
    dict(id="coronation_first", who="advisor",
         text="$R, the Emperor grants House $D the fief of Kessar. First decree?",
         L=dict(label="Feed the tribes", t=10, s=-10),
         R=dict(label="Fill the coffers", s=10, t=-10),
         when=["newreign", "first"], priority=True, reign_once=True),
    dict(id="coronation_heir", who="sister",
         text="The old Regent is ash. $R takes the throne. The Sisterhood offers its oil.",
         L=dict(label="Crown myself", o=-10, l=10),
         R=dict(label="Kneel for the oil", o=10, l=-5, s=-5),
         when=["newreign", "!first"], priority=True, reign_once=True),
    dict(id="heir_raised_desert", who="naib",
         text="You grew up eating sand with our children, $R. The tribes remember. Do you?",
         L=dict(label="My duty", l=10, t=-5, clear="d_heir_desert"),
         R=dict(label="Your kindness", t=15, l=-10, clear="d_heir_desert"),
         when=["!first", "d_heir_desert"], priority=True, reign_once=True),
    dict(id="heir_raised_sword", who="swordmaster",
         text="I trained you since you could walk, $R. The soldiers know it. Remind them?",
         L=dict(label="Keep it humble", l=5, o=5, clear="d_heir_sword"),
         R=dict(label="Shout it", l=15, t=-5, o=-5, clear="d_heir_sword"),
         when=["!first", "d_heir_sword"], priority=True, reign_once=True),
    dict(id="heir_raised_sister", who="sister",
         text="We taught you the Voice, $R. The Sisterhood expects a small favor. For now.",
         L=dict(label="I owe you nothing", o=-10, l=5, clear="d_heir_sister"),
         R=dict(label="Name the favor", o=15, s=-5, clear="d_heir_sister"),
         when=["!first", "d_heir_sister"], priority=True, reign_once=True),

    # ------------------------------------------------------- heir upbringing
    dict(id="heir_fostering", who="naib",
         text="Give your heir to my camp for a year. The child will learn the sand, or be eaten by it.",
         L=dict(label="Too dangerous", t=-10, o=5),
         R=dict(label="Take the child", t=10, l=-5, set="d_heir_desert"),
         when=["!d_heir_desert"], reign_once=True),
    dict(id="heir_sword", who="swordmaster",
         text="Your heir holds a blade like a soup spoon. Give me the child every dawn, rain or no rain.",
         L=dict(label="Books, not blades", l=-10, o=5),
         R=dict(label="Train them hard", l=10, o=-5, set="d_heir_sword"),
         when=["!d_heir_sword"], reign_once=True),
    dict(id="heir_sister", who="sister",
         text="Let the Sisterhood teach your heir the Voice. Small price: a little obedience to us.",
         L=dict(label="Absolutely not", o=-10, l=5),
         R=dict(label="Teach them", o=10, t=-5, set="d_heir_sister"),
         when=["!d_heir_sister"], reign_once=True),
    dict(id="heir_thopter", who="heir",
         text="I want my own ornithopter. Every noble child at court has one. Even Vulkar's ugly nephew.",
         L=dict(label="Earn it first", o=5, l=-5),
         R=dict(label="Buy two", s=-5, l=5)),
    dict(id="heir_runaway", who="heir",
         text="I sneaked out to watch the worm riders! They let me touch a hook. Can I go back? Please?",
         L=dict(label="Lock the tower", t=-10, o=5),
         R=dict(label="Go, with guards", t=10, l=-10)),
    dict(id="heir_question", who="heir",
         text="Why do we take the tribes' water and call it tribute?",
         L=dict(label="Because we can", t=-10, s=10),
         R=dict(label="Good question", t=10, l=-5, o=-5)),
    dict(id="heir_orphan", who="heir",
         text="A war orphan follows me everywhere. She bites the guards. Can we keep her? Please?",
         L=dict(label="Send her away", t=-5, s=5),
         R=dict(label="Adopt her", t=5, s=-10, do="heir+1"),
         when=["!heir_full"], reign_once=True),
    dict(id="heir_vision", who="heir",
         text="I ate the spice raw, like the tribes do. I saw you die, $R. Shall I tell you how?",
         L=dict(label="Don't you dare", o=5, l=-5),
         R=dict(label="Tell me", o=-10, t=5, do="presc+1")),
    dict(id="heir_marriage", who="envoy",
         text="A match for your heir: Vulkar's daughter. Peace between Houses, or a viper in your nest.",
         L=dict(label="Never", l=10, s=-10),
         R=dict(label="Peace", s=10, l=-10, clear="baron_threat"),
         when=["baron_threat"]),
    dict(id="heir_tutor", who="advisor",
         text="Your heir corrected my arithmetic today. Twice. I recommend a tutor, or exile.",
         L=dict(label="A tutor", s=-5, o=5),
         R=dict(label="Make them my aide", o=-5, l=5)),

    # ------------------------------------------------------------- baron arc
    dict(id="baron_insult", who="baron",
         text="Your spice yields are pathetic, $R. Sell me the southern erg before you lose it.",
         L=dict(label="Get out", l=5, o=-5, set="baron_threat"),
         R=dict(label="Name a price", s=15, l=-10, t=-5),
         when=["!baron_threat"]),
    dict(id="baron_raid", who="marshal",
         text="Vulkar's raiders burned a harvester and laughed about it. Strike back?",
         L=dict(label="Hold the line", l=-10, s=-5),
         R=dict(label="Burn his outpost", l=10, s=10, o=-10),
         when=["baron_threat"]),
    dict(id="baron_poison", who="physician",
         text="Your cup was poisoned, $R. Vulkar's taste in toxins is rather showy.",
         L=dict(label="Hush it up", o=5, l=-10),
         R=dict(label="Accuse him", l=10, o=-5),
         when=["baron_threat"]),
    dict(id="baron_challenge", who="baron",
         text="A blood feud, $R. You and me, blades, before the Emperor. Unless you are a coward.",
         L=dict(label="Refuse", o=-10, l=-10),
         R=dict(label="Accept", l=10, o=5, set="challenged"),
         when=["baron_threat", "!challenged"], reign_once=True),
    dict(id="swordmaster_training", who="swordmaster",
         text="Vulkar fights dirty and left-handed. Give me a month and I'll make you ugly enough to win.",
         L=dict(label="No time", l=-5, s=5),
         R=dict(label="Train me", l=5, s=-10, set="trained"),
         when=["baron_threat", "!trained"], weight=3),
    dict(id="duel_untrained", who="baron",
         text="The ring is drawn. Vulkar smiles and flexes his left hand. Your blade feels very heavy.",
         L=dict(label="Send a champion", l=-15, o=-10, clear="challenged"),
         R=dict(label="Fight him myself", l=5, do="fate:duel"),
         when=["challenged", "!trained"], priority=True),
    dict(id="duel_trained", who="baron",
         text="The ring is drawn. Vulkar lunges from the left. Halder drilled you for exactly this.",
         L=dict(label="Disarm, spare him", o=10, s=10, l=-5, clear="challenged"),
         R=dict(label="Finish him", l=20, s=15, o=-10, t=-5, clear="challenged"),
         when=["challenged", "trained"], priority=True),
    dict(id="baron_bribe", who="envoy",
         text="Baron Vulkar offers the Emperor a lavish gift for your fief. Will you outbid him?",
         L=dict(label="Let him try", s=5, o=-10),
         R=dict(label="Outbid him", s=-15, o=10, l=5)),
    dict(id="baron_spy", who="advisor",
         text="One of your cooks reports to Vulkar. Feed him lies, or feed him to the worms?",
         L=dict(label="Feed him lies", s=10, l=-5),
         R=dict(label="To the worms", l=10, o=-5)),
    dict(id="baron_nephew", who="baron",
         text="My nephew needs a wife and I need your spice fields. Shall we discuss both over dinner?",
         L=dict(label="Bring a taster", l=5, o=-5, set="baron_threat"),
         R=dict(label="Sell some fields", s=10, l=-10)),

    # ----------------------------------------------------- faceless intrigue
    dict(id="stranger_arrives", who="stranger",
         text="A traveller with no shadow begs for shelter. He hums your mother's lullaby, note for note.",
         L=dict(label="Throw him out", o=5, t=-10),
         R=dict(label="Let him stay", t=5, o=-5, set="stranger_in"),
         when=["!stranger_in"]),
    dict(id="faceless_odd1", who="swordmaster",
         text="Halder bows and calls you 'my liege'. In thirty years he has only ever called you 'pup'.",
         L=dict(label="Arrest him", l=-10, o=5, next="faceless_reveal"),
         R=dict(label="He's just tired", l=5, o=-5, set="faceless1"),
         when=["stranger_in", "!faceless1"], reign_once=True, weight=3),
    dict(id="faceless_odd2", who="consort",
         text="Lady Imre eats with her left hand tonight. At breakfast she was right-handed.",
         L=dict(label="Seize her", o=-10, l=5, next="faceless_reveal"),
         R=dict(label="Kiss her anyway", o=5, l=-5, set="faceless2"),
         when=["faceless1", "!faceless2"], weight=3),
    dict(id="faceless_odd3", who="advisor",
         text="Vael pours your wine himself. Vael never pours. Vael counts the drops.",
         L=dict(label="You drink first", l=10, o=-5, next="faceless_reveal"),
         R=dict(label="Drink", o=5, s=5, do="fate:faceless"),
         when=["faceless1", "faceless2"], priority=True),
    dict(id="faceless_reveal", who="stranger",
         text="The face slides like warm wax. Beneath it, nothing at all. A Faceless, wearing your friend.",
         L=dict(label="Burn it", l=10, t=-5, clear="faceless1"),
         R=dict(label="Interrogate it", o=10, s=-10, clear="faceless1"),
         chain=True),
    dict(id="stranger_vial", who="stranger",
         text="The traveller offers a vial of liquid spice. 'For your dreams,' he says, never blinking.",
         L=dict(label="Refuse politely", o=5, t=-5),
         R=dict(label="Drink it", l=-10, o=-5, do="presc+1"),
         when=["stranger_in"]),
    dict(id="stranger_stones", who="stranger",
         text="A hooded stranger offers a game of stones. Stake: a harvester against a map of hidden water.",
         L=dict(label="No games", s=5, t=-5),
         R=dict(label="Play", s=-10, l=5, do="chance", next="stranger_win")),
    dict(id="stranger_win", who="stranger",
         text="You won. The map shows a cistern the tribes would kill for. They may yet.",
         L=dict(label="Sell it to them", s=15, t=-5),
         R=dict(label="Give it back", t=15, s=-5),
         chain=True),
    dict(id="stranger_warning", who="stranger",
         text="A beggar grips your sleeve: 'Your marshal will betray you before the next storm.'",
         L=dict(label="Lies", l=5, o=-5),
         R=dict(label="Watch Draxe", l=-10, o=5, set="marshal_plot")),

    # ------------------------------------------------------------ holy war
    dict(id="prophet_preach", who="prophet",
         text="Your heir walked into the sand and came back with blue eyes. The Promised One is among us!",
         L=dict(label="Just a child", t=-10, o=5),
         R=dict(label="Let them believe", t=15, o=-10, set="messiah1"),
         when=["!messiah1"]),
    dict(id="prophet_relics", who="prophet",
         text="Pilgrims tear your heir's robes for relics. They demand a holy name for the child.",
         L=dict(label="Forbid it", t=-10, o=10, clear="messiah1"),
         R=dict(label="Grant the name", t=15, o=-15, l=5, set="messiah2"),
         when=["messiah1", "!messiah2"], weight=3),
    dict(id="prophet_war", who="prophet",
         text="A million blades wait for one word from the Promised One. Spoken by you, $R.",
         L=dict(label="Silence him", t=-25, o=15, l=-5, clear="messiah2"),
         R=dict(label="Say the word", t=20, l=10, do="fate:holy_war"),
         when=["messiah2"], priority=True),
    dict(id="sister_messiah", who="sister",
         text="That prophet is ours, $R. A legend we seeded centuries ago. Do not feed it.",
         L=dict(label="I'll feed it", t=10, o=-10),
         R=dict(label="Starve it", o=10, t=-10, clear="messiah1"),
         when=["messiah1"]),
    dict(id="prophet_rant", who="prophet",
         text="Repent! The Maker will swallow the palace of the water thieves, and their latrines too!",
         L=dict(label="Arrest him", t=-10, l=5),
         R=dict(label="Let him rant", t=5, l=-5)),
    dict(id="prophet_miracle", who="prophet",
         text="The Prophet walked through a sandstorm without a scratch. The crowd kneels. Will you?",
         L=dict(label="Never", o=10, t=-10),
         R=dict(label="Kneel", t=10, o=-10)),
    dict(id="prophet_sacrifice", who="prophet",
         text="Pilgrims want to feed a condemned man to the Maker. Theirs, preferably. Or yours.",
         L=dict(label="Forbid it", t=-10, o=10),
         R=dict(label="Feed the worm", t=10, o=-10, l=-5)),

    # ----------------------------------------------------- guild & embargo
    dict(id="navigator_tribute", who="navigator",
         text="The Guild requires twice the spice this year. Or no ship will fold space for Kessar.",
         L=dict(label="Refuse", s=10, o=-5, set="embargo"),
         R=dict(label="Pay", s=-15, l=5),
         when=["!embargo"]),
    dict(id="embargo_envoy", who="envoy",
         text="No ships means no tribute. The Emperor wonders if he gave Kessar to a fool.",
         L=dict(label="The Guild cheats", o=-10, l=-5),
         R=dict(label="Pay the Guild", s=-20, o=10, clear="embargo"),
         when=["embargo"]),
    dict(id="embargo_smuggler", who="smuggler",
         text="Guild's closed? I have a ship and no questions. Thirty percent, darling.",
         L=dict(label="Too much", s=-5, l=5),
         R=dict(label="Deal", s=15, o=-10, set="smuggler_deal"),
         when=["embargo"]),
    dict(id="embargo_navigator", who="navigator",
         text="We starve too, Regent. Peace, on one condition: our tanks are filled first. Always.",
         L=dict(label="Never", s=5, l=-10),
         R=dict(label="Agreed", s=-10, o=5, clear="embargo"),
         when=["embargo"]),
    dict(id="navigator_vision", who="navigator",
         text="I have folded a thousand stars. In your path I see a fork. Breathe my gas and see it too.",
         L=dict(label="Decline", o=5, t=-5),
         R=dict(label="Breathe deep", l=-5, o=-5, do="presc+1")),
    dict(id="navigator_monopoly", who="navigator",
         text="The Guild will pay well for a monopoly on your spice. For you and every heir after you.",
         L=dict(label="No", t=5, o=-5),
         R=dict(label="Sign forever", s=25, o=-10, t=-10, set="d_guild_pact"),
         when=["!d_guild_pact"]),
    dict(id="guild_pact_strain", who="navigator",
         text="The pact says ALL spice, Regent. Even what the tribes burn in their rites.",
         L=dict(label="Make them give", s=10, t=-15),
         R=dict(label="Break the pact", s=-15, t=10, clear="d_guild_pact"),
         when=["d_guild_pact"]),
    dict(id="navigator_tank", who="navigator",
         text="My tank runs thin. My mind folds poorly. Your cargo may arrive... elsewhere.",
         L=dict(label="Ration it", s=10, l=-5, o=-5),
         R=dict(label="Fill it", s=-10, o=5)),
    dict(id="guild_passage", who="navigator",
         text="A Guild ship offers you passage to the Imperial court. You would be gone a year.",
         L=dict(label="Stay home", o=-5, l=5),
         R=dict(label="Go to court", o=15, l=-10, do="age+1")),
    dict(id="guild_windtraps", who="navigator",
         text="Water on Kessar means no spice. The Guild sees all. Stop the windtraps, or be stopped.",
         L=dict(label="Slow the work", s=10, t=-10),
         R=dict(label="Defy the Guild", s=-15, o=5, set="embargo"),
         when=["d_windtraps", "!embargo"]),

    # ------------------------------------------------ marriage, heirs, loss
    dict(id="envoy_marriage", who="envoy",
         text="The Emperor offers his niece, Lady Imre, as your consort. Refusal would be... noted.",
         L=dict(label="Refuse", o=-10, l=5),
         R=dict(label="Accept", o=10, s=10, l=-10, set="married"),
         when=["!married"], reign_once=True),
    dict(id="consort_child", who="consort",
         text="I carry your child, $R. The Sisterhood wants a daughter. I want a son. Pray for one.",
         L=dict(label="A daughter", o=10, l=-5, do="heir+1"),
         R=dict(label="A son", l=10, o=-10, do="heir+1"),
         when=["married", "!heir_full"], reign_once=True, weight=3),
    dict(id="consort_marshal", who="consort",
         text="Your marshal looks at your throne the way I once looked at you.",
         L=dict(label="Jealousy", l=5, o=-5),
         R=dict(label="Watch him", l=-10, o=5, set="marshal_plot"),
         when=["married"]),
    dict(id="consort_rumor", who="consort",
         text="The court whispers I have a lover. I don't. Prove you trust me, $R.",
         L=dict(label="Prove it first", o=5, l=-5),
         R=dict(label="Exile gossips", o=-10, l=5, s=5),
         when=["married"]),
    dict(id="consort_garden", who="consort",
         text="I want a garden. A real one. Water from the cistern, trees from off-world.",
         L=dict(label="Water is life", t=5, o=-5),
         R=dict(label="Build it", t=-10, s=-5, o=10)),
    dict(id="consort_perfume", who="consort",
         text="Someone poisoned my perfume. I suspect your Sisterhood friend. She smiles too much.",
         L=dict(label="Accuse Ysolde", o=-15, l=5),
         R=dict(label="Protect Ysolde", o=10, l=-5),
         when=["married"]),
    dict(id="plague", who="physician",
         text="Water fever in the nursery. Seal the palace and the city starves. Keep it open and...",
         L=dict(label="Seal the palace", s=-15, t=-10),
         R=dict(label="Keep it open", s=5, t=5, do="heir-1"),
         when=["!heir_last"], reign_once=True),
    dict(id="heir_assassins", who="swordmaster",
         text="Assassins in the nursery! I can save your heir or hold the gate. Not both, pup!",
         L=dict(label="Save the heir", l=-15, o=-5),
         R=dict(label="Hold the gate", l=10, do="heir-1"),
         when=["!heir_last", "baron_threat"], reign_once=True),
    dict(id="sister_breeding", who="sister",
         text="Your bloodline interests us. Let one of our daughters into your bed. For the plan.",
         L=dict(label="No", o=-10, l=5),
         R=dict(label="For the plan", o=10, l=-10, do="heir+1"),
         when=["!heir_full"]),

    # ------------------------------------------------------ deep desert/worm
    dict(id="naib_ride", who="naib",
         text="You have earned the hooks, $R. Ride the Maker with us into the deep desert.",
         L=dict(label="Another time", t=-10, l=5),
         R=dict(label="Call the worm", t=15, o=-5, l=-5, do="presc+1"),
         when=["t>=60"], reign_once=True),
    dict(id="walk_sand", who="harvester",
         text="Your ornithopter's down in open erg, sire. The crawler is far. Walk the sand?",
         L=dict(label="Wait for rescue", l=-10, s=-10),
         R=dict(label="Start walking", t=5, l=5, do="chance", next="worm_rises"),
         when=["t<60"]),
    dict(id="worm_rises", who="worm",
         text="Your steps had no rhythm. The sand swells. The Maker opens a mouth like a cathedral.",
         L=dict(label="Stand dead still", l=-15, s=-10),
         R=dict(label="Run!", t=10, do="fate:worm"),
         chain=True),
    dict(id="worm_sign", who="worm",
         text="Wormsign on the horizon, wide as a city. The harvest crew begs to flee.",
         L=dict(label="Flee", s=-10, l=5),
         R=dict(label="Harvest on", s=20, l=-5, do="chance", next="worm_crawler")),
    dict(id="worm_crawler", who="worm",
         text="The Maker swallows a crawler whole. Mott is still on the roof, cursing.",
         L=dict(label="Save Mott", l=-10, s=-5),
         R=dict(label="Save the spice", s=5, t=-5, l=-5),
         chain=True),
    dict(id="worm_omen", who="worm",
         text="Sand ripples form an eye around the palace. The tribes say the Maker is watching you.",
         L=dict(label="Ignore it", t=-10, s=5),
         R=dict(label="Offer water", t=10, s=-10)),
    dict(id="worm_dream", who="worm",
         text="In a dream, the Old Worm speaks: 'Swallow what I leave behind, and see.'",
         L=dict(label="Wake up", o=5, t=-5),
         R=dict(label="Swallow", l=-5, o=-5, do="presc+1")),
    dict(id="worm_bones", who="worm",
         text="A dead worm, a mile long, lies in the open sand. Its teeth are worth a fortune.",
         L=dict(label="Leave it be", t=10, s=-5),
         R=dict(label="Harvest teeth", s=20, t=-15)),

    # --------------------------------------------------------- harvesters
    dict(id="spice_blow", who="harvester",
         text="Sire, a spice blow in the southern erg! Send the crawlers before the worm comes?",
         L=dict(label="Too risky", s=-5, t=5),
         R=dict(label="Send them all", s=20, t=-10, l=-5),
         when=["s<60"], weight=2),
    dict(id="crawler_broken", who="harvester",
         text="Three crawlers dead, sand in the bearings. Buy off-world parts or let the tribes fix them?",
         L=dict(label="Off-world parts", s=-5, o=5),
         R=dict(label="Tribal mechanics", t=10, s=-5, l=-5)),
    dict(id="harvester_strike", who="harvester",
         text="The crews want water rations doubled. Or they sit in the shade and sing rude songs.",
         L=dict(label="Double it", s=-5, o=5),
         R=dict(label="Whip them", s=10, l=5, t=-10)),
    dict(id="harvester_drunk", who="harvester",
         text="The carrier pilot is drunk. Fly him anyway, or lose this shift's spice?",
         L=dict(label="Ground him", s=-5, l=5),
         R=dict(label="Fly", s=15, do="chance", next="carrier_crash")),
    dict(id="carrier_crash", who="harvester",
         text="The carrier dropped a full crawler from a great height. Crew and all. I warned you.",
         L=dict(label="Pay the widows", s=-10, t=5),
         R=dict(label="Blame the pilot", l=-5, o=-5),
         chain=True),
    dict(id="harvester_skim", who="harvester",
         text="Boss, I've been skimming. Everyone does. Want a cut? It's a generous cut.",
         L=dict(label="Arrest Mott", s=-10, l=5),
         R=dict(label="Take a cut", s=20, o=-10)),
    dict(id="harvester_sabotage", who="harvester",
         text="The tribes keep sabotaging my crawlers. Give me guards, or give them a share.",
         L=dict(label="Send guards", l=10, s=5, t=-10),
         R=dict(label="Share the spice", t=10, s=-10)),
    dict(id="spice_essence", who="harvester",
         text="Mott found a pocket of pure spice essence. One sip shows the future. Two sips kill.",
         L=dict(label="Sell it", s=10, o=-5),
         R=dict(label="One sip", l=-10, do="presc+1")),
    dict(id="harvester_record", who="harvester",
         text="Record harvest! The crews want a feast. The Emperor wants it all. I want a nap.",
         L=dict(label="Feast", s=-5, l=5, t=5),
         R=dict(label="Ship it all", s=20, l=-5, t=-5),
         when=["s<70"]),

    # ------------------------------------------------------------ smuggler
    dict(id="smuggler_offer", who="smuggler",
         text="Spice off the books, no Imperial tax. You and me, partners?",
         L=dict(label="No", o=5, t=-5),
         R=dict(label="Partners", s=20, o=-10, set="smuggler_deal"),
         when=["!smuggler_deal"]),
    dict(id="smuggler_caught", who="marshal",
         text="We seized Tessa's ship. She claims she works for you. Does she?",
         L=dict(label="Never met her", s=-15, l=10, clear="smuggler_deal"),
         R=dict(label="She does", s=5, l=-10, o=-5),
         when=["smuggler_deal"]),
    dict(id="smuggler_rifles", who="smuggler",
         text="I can get beam rifles. Military grade. Don't ask from whose military.",
         L=dict(label="Pass", l=-5, o=5),
         R=dict(label="Buy a crate", l=15, s=-10, o=-10)),
    dict(id="smuggler_tip", who="smuggler",
         text="A tip from the docks: Vulkar is moving troops. Tips cost extra, darling.",
         L=dict(label="Not paying", l=-10, s=5),
         R=dict(label="Pay her", s=-10, l=10),
         when=["smuggler_deal"]),
    dict(id="smuggler_water", who="smuggler",
         text="Off-world water, sold cheap to the tribes. All you need to do is look away.",
         L=dict(label="Stop her", t=-10, l=5),
         R=dict(label="Look away", t=10, s=-10)),
    dict(id="smuggler_escape", who="smuggler",
         text="If it ever goes bad, I have a ship waiting. For a fee, you could be on it.",
         L=dict(label="Never run", l=5, o=-5),
         R=dict(label="Pay the fee", s=-10, o=5, t=5)),

    # --------------------------------------------------------------- storms
    dict(id="storm_flight", who="advisor",
         text="A Coriolis storm is coming. The council meets beyond it. Fly through, or miss the vote?",
         L=dict(label="Miss the vote", o=-10, s=5),
         R=dict(label="Fly through", o=10, do="chance", next="storm_caught")),
    dict(id="storm_caught", who="advisor",
         text="The storm rips a wing off. Sand screams across the canopy at seven hundred kph.",
         L=dict(label="Crash into rocks", l=-15, s=-10),
         R=dict(label="Ride the wind", o=5, do="fate:storm"),
         chain=True),
    dict(id="storm_city", who="advisor",
         text="Sandstorm over the city. Open the palace shelters to the poor?",
         L=dict(label="Keep them sealed", t=-10, l=5),
         R=dict(label="Open the doors", t=10, l=-5, s=-5)),
    dict(id="storm_harvest", who="harvester",
         text="Storm's coming. Pull the crawlers in, or squeeze one more hour of spice?",
         L=dict(label="Pull them in", s=-10, l=5),
         R=dict(label="One more hour", s=20, l=-5, t=-5)),

    # --------------------------------------------------------- hunter drone
    dict(id="hs_warning", who="swordmaster",
         text="A seeker drone is loose in the walls. Sleep in the stone vault tonight, pup.",
         L=dict(label="Nonsense", l=5, o=5, do="chance", next="hs_strike"),
         R=dict(label="The vault", l=-5, o=-5)),
    dict(id="hs_strike", who="swordmaster",
         text="A needle of metal hums above your pillow, seeking motion. Halder is still running.",
         L=dict(label="Hold your breath", l=-10, o=-5),
         R=dict(label="Swat it", l=5, do="fate:hunter_seeker"),
         chain=True),

    # ---------------------------------------------------------------- old age
    dict(id="old_heart", who="physician",
         text="Your heart is tired, $R. Rest now and die in bed. Or rule on, fueled by spice.",
         L=dict(label="Rule on", s=-15, o=-5),
         R=dict(label="Rest at last", o=10, t=5, do="fate:old_age"),
         when=["old"], weight=3),
    dict(id="old_retreat", who="advisor",
         text="You look grey, $R. Retreat to the mountain springs for a few years? The council can rule.",
         L=dict(label="Retreat", o=-10, t=10, do="age+5"),
         R=dict(label="Stay", l=5, s=-5),
         when=["old"]),
    dict(id="jubilee", who="advisor",
         text="Twenty years on the throne, $R! A jubilee? The treasury trembles at the word.",
         L=dict(label="Modest dinner", s=5, o=-5),
         R=dict(label="Lavish jubilee", s=-15, o=10, t=5),
         when=["yr20"], reign_once=True),

    # ---------------------------------------------------------------- marshal
    dict(id="marshal_troops", who="marshal",
         text="Give me more troops and I'll make the tribes kneel by summer.",
         L=dict(label="No", l=-10, t=5, s=5),
         R=dict(label="Yes", l=15, t=-15)),
    dict(id="marshal_title", who="marshal",
         text="The army loves me, $R. Perhaps it should love me officially. Regent-General?",
         L=dict(label="Never", l=-15, o=5, set="marshal_plot"),
         R=dict(label="Fine, a title", l=10, o=-10),
         when=["l>=65"]),
    dict(id="marshal_plotting", who="advisor",
         text="Draxe meets your officers at night, in the old cistern. Strike first?",
         L=dict(label="Arrest him", l=-20, o=10, clear="marshal_plot"),
         R=dict(label="Wait and watch", l=10, s=-10),
         when=["marshal_plot"], weight=3),
    dict(id="marshal_thopters", who="marshal",
         text="New ornithopters, faster than the Baron's. Expensive, but so is losing.",
         L=dict(label="Too expensive", l=-10, s=10),
         R=dict(label="Buy a wing", l=10, s=-10)),
    dict(id="marshal_raiders", who="marshal",
         text="We caught tribal raiders. Hang them on the walls as a lesson?",
         L=dict(label="Free them", t=10, l=-10),
         R=dict(label="Hang them", l=10, t=-15, s=5)),
    dict(id="marshal_parade", who="marshal",
         text="A parade for the troops! The tribes find our parades... provocative.",
         L=dict(label="No parade", l=-10, t=5),
         R=dict(label="Parade!", l=10, t=-5, s=-5)),
    dict(id="marshal_desertion", who="marshal",
         text="Desertions! Soldiers flee to the tribes with their rifles. Raise pay, or hang a few?",
         L=dict(label="Hang a few", l=5, t=-10, s=5),
         R=dict(label="Raise pay", s=-15, l=10)),
    dict(id="tribal_raid", who="marshal",
         text="Raiders hit the spice silos at dawn. Retaliate, or talk to the Naib?",
         L=dict(label="Talk", t=10, s=-10),
         R=dict(label="Retaliate", l=10, t=-15, s=10)),

    # ---------------------------------------------------------------- advisor
    dict(id="advisor_tax", who="advisor",
         text="Raise the water tax by a tenth. The numbers say yes. The tribes will say otherwise.",
         L=dict(label="Leave it", s=-5, t=5),
         R=dict(label="Raise it", s=20, t=-15)),
    dict(id="advisor_census", who="advisor",
         text="A census of the desert camps would let us tax them properly. And find them.",
         L=dict(label="Leave them be", t=5, s=-5),
         R=dict(label="Count them", s=15, t=-10, l=5)),
    dict(id="advisor_juice", who="advisor",
         text="My computations slow. More red root juice, please. It stains, but it computes.",
         L=dict(label="Cut you off", s=5, o=-5),
         R=dict(label="Drink up", s=-5, do="presc+1")),
    dict(id="advisor_audit", who="advisor",
         text="Imperial auditors arrive tomorrow. Our books are... creative.",
         L=dict(label="Bribe them", s=-10, o=5),
         R=dict(label="Cook harder", s=15, o=-5)),
    dict(id="advisor_wells", who="advisor",
         text="Deep wells would end the water riots. They would also end our water monopoly.",
         L=dict(label="Keep monopoly", s=10, t=-10),
         R=dict(label="Dig wells", t=10, s=-10)),

    # ----------------------------------------------------------------- sister
    dict(id="sister_ritual", who="sister",
         text="The Bitter Water, $R. Drink, change the poison within you, and see. Or die trying.",
         L=dict(label="Not today", o=-5, l=5),
         R=dict(label="Drink", o=10, l=-10, do="presc+2"),
         reign_once=True),
    dict(id="sister_truth", who="sister",
         text="Let a truth-reader sit at your council. Lies will become... difficult.",
         L=dict(label="No", o=-10, s=5),
         R=dict(label="Yes", o=10, s=-5, l=-5)),
    dict(id="sister_school", who="sister",
         text="Grant us land for a new school. The tribes graze goats there. Had goats.",
         L=dict(label="Goats first", t=10, o=-10),
         R=dict(label="School first", o=10, t=-10)),
    dict(id="sister_box", who="sister",
         text="Put your hand in the box, $R. It is only pain. Are you human, or animal?",
         L=dict(label="Refuse", o=-15, l=5, s=5),
         R=dict(label="Hand in the box", o=15, l=-10)),
    dict(id="sister_legend", who="sister",
         text="We could spread a legend of your mercy among the tribes. Legends are cheap.",
         L=dict(label="No lies", o=-5, t=-5, s=5),
         R=dict(label="Spread it", t=10, o=5, s=-10)),
    dict(id="sister_tithe", who="sister",
         text="The Sisterhood requests a tithe of spice. For the faithful. And our sandals.",
         L=dict(label="No", o=-10, s=15),
         R=dict(label="Tithe", o=10, s=-10)),
    dict(id="sister_seer", who="sister",
         text="Your eyes are blue at the edges now. The visions have a price, $R. Pay more?",
         L=dict(label="Enough visions", o=-5, l=5),
         R=dict(label="Show me more", o=-5, l=-5, do="presc+1"),
         when=["seer"]),

    # ------------------------------------------------------------------- naib
    dict(id="naib_water", who="naib",
         text="Your soldiers took water from a dead man. That water belongs to the tribe.",
         L=dict(label="Soldier's spoils", l=10, t=-15),
         R=dict(label="Return it", t=10, l=-10)),
    dict(id="naib_water_debt", who="naib",
         text="We pulled your patrol out of the sand. Now you owe us water. Pay now, or owe it.",
         L=dict(label="Pay in spice", s=-10, t=5),
         R=dict(label="Owe it", t=5, l=5, set="water_debt"),
         when=["!water_debt"]),
    dict(id="water_debt_due", who="naib",
         text="The water debt is due. Bring the cistern keys, or bring your blood.",
         L=dict(label="Refuse", t=-20, l=5, clear="water_debt"),
         R=dict(label="Pay the debt", t=10, s=-15, clear="water_debt"),
         when=["water_debt"]),
    dict(id="naib_council", who="naib",
         text="Give the tribes a seat on your council. We promise to only stab outside.",
         L=dict(label="No", t=-10, s=10, o=5),
         R=dict(label="Yes", t=15, l=-10, o=-5)),
    dict(id="naib_well", who="naib",
         text="Two camps feud over a well. Judge between them, $R. The loser will hate you.",
         L=dict(label="The elders", t=-5, o=10),
         R=dict(label="Split the well", t=10, s=-5, l=-5)),
    dict(id="naib_drums", who="naib",
         text="Our children want to beat drums near your crawlers to lure the worm away.",
         L=dict(label="Forbid it", s=5, t=-10),
         R=dict(label="Allow it", t=10, s=-5)),
    dict(id="naib_knife", who="naib",
         text="The tribes give you a worm-tooth knife. Once unsheathed, it must taste blood.",
         L=dict(label="Decline", t=-10, o=5),
         R=dict(label="Accept", t=10, l=5, o=-10),
         when=["t>=55"], reign_once=True),
    dict(id="naib_stillsuits", who="naib",
         text="Your soldiers wear cheap stillsuits. They leak. They die. Buy ours.",
         L=dict(label="Ours are fine", l=-10, s=5),
         R=dict(label="Buy yours", l=10, t=5, s=-15)),

    # ------------------------------------------------------------------ envoy
    dict(id="envoy_tribute", who="envoy",
         text="The Emperor raises the tribute. He is building a new palace. Twelve, actually.",
         L=dict(label="Refuse", s=15, o=-10, l=-5),
         R=dict(label="Pay", s=-15, o=10)),
    dict(id="envoy_garrison", who="envoy",
         text="The Emperor's elite will 'visit' Kessar. You will house and feed a legion of them.",
         L=dict(label="Refuse", l=5, o=-10),
         R=dict(label="House them", l=-10, o=10)),
    dict(id="envoy_numbers", who="envoy",
         text="His Majesty notes your spice numbers are... optimistic. Someone must be blamed.",
         L=dict(label="Pay the gap", s=-10, o=5),
         R=dict(label="Blame Vulkar", o=-5, s=5, set="baron_threat")),
    dict(id="envoy_title", who="envoy",
         text="Buy a grander title? 'Archduke' sounds well. The price is merely ruinous.",
         L=dict(label="Pass", o=-5, s=5),
         R=dict(label="Archduke!", s=-15, o=10, l=5)),
    dict(id="envoy_report", who="envoy",
         text="The Emperor would like a report on your marshal. Discreetly. Very discreetly.",
         L=dict(label="Refuse", o=-10, l=10),
         R=dict(label="Report", o=10, l=-10, s=5)),

    # -------------------------------------------------------- terraforming
    dict(id="tf_survey", who="planetologist",
         text="Kessar has water, $R. Deep down. Fund my survey and I will prove it.",
         L=dict(label="Fairy tales", s=5, t=-5),
         R=dict(label="Fund it", s=-15, t=10, set="d_survey"),
         when=["!d_survey", "s>=40"], reign_once=True, weight=4),
    dict(id="tf_windtraps", who="planetologist",
         text="The survey is done. Now windtraps, thousands of them, catching every wet breath of air.",
         L=dict(label="Too costly", s=5, t=-10),
         R=dict(label="Build them", s=-20, t=15, set="d_windtraps"),
         when=["d_survey", "!d_windtraps", "s>=40"], reign_once=True, weight=4),
    dict(id="tf_dew", who="planetologist",
         text="Your forebear's windtraps hum. Next: dew collectors on every dune crest. Fund them?",
         L=dict(label="Enough water", s=10, t=-10),
         R=dict(label="Build collectors", s=-15, t=10, set="d_dew"),
         when=["d_windtraps", "!d_dew", "!first"], reign_once=True, weight=4),
    dict(id="tf_pact", who="naib",
         text="The doctor's dream is ours too. Swear the Maker keeps the deep desert and we share our caches.",
         L=dict(label="Kessar is mine", t=-15, l=10),
         R=dict(label="I swear it", t=15, s=-15, set="d_pact"),
         when=["d_dew", "!d_pact", "t>=50"], reign_once=True, weight=4),
    dict(id="tf_green", who="planetologist",
         text="Rain. On Kessar. The tribes dance in mud and the Emperor is livid. Let it fall?",
         L=dict(label="Not yet", s=10, t=-10),
         R=dict(label="Let it rain", t=20, s=-20, do="fate:green_world"),
         when=["d_pact", "yr5", "o>=30"], priority=True, reign_once=True),
    dict(id="tf_repairs", who="planetologist",
         text="Sand eats the windtraps. Pay for repairs, or let them silt up and die?",
         L=dict(label="Let them silt", s=10, t=-10, clear="d_windtraps"),
         R=dict(label="Repair them", s=-10, t=5),
         when=["d_windtraps", "!d_dew"]),
    dict(id="tf_lessons", who="planetologist",
         text="The tribal children learn ecology from me now. The Sisterhood calls it heresy.",
         L=dict(label="Stop lessons", o=5, t=-10),
         R=dict(label="Keep teaching", t=10, o=-10),
         when=["d_survey"]),
    dict(id="tf_dream", who="planetologist",
         text="Imagine it, $R: grass on the dunes, rain on your face. Help me dream it into a plan.",
         L=dict(label="Dreams are cheap", s=5, t=-5),
         R=dict(label="Draft the plan", s=-5, t=5, o=5),
         when=["!d_survey"]),
    dict(id="tf_plants", who="planetologist",
         text="My poverty grass holds the dunes in place. It also chokes the spice beds.",
         L=dict(label="Burn the grass", s=10, t=-10),
         R=dict(label="Plant more", t=10, s=-10),
         when=["d_windtraps"]),

    # -------------------------------------------------------------- physician
    dict(id="physician_hostage", who="physician",
         text="Vulkar holds my wife hostage. Lend me soldiers, or I must... cooperate with him.",
         L=dict(label="Cooperate?", l=5, o=-10, set="baron_threat"),
         R=dict(label="Take soldiers", l=-15, o=10)),
    dict(id="physician_tonic", who="physician",
         text="A new tonic for the troops: they march longer, but their eyes turn blue.",
         L=dict(label="No", l=-5, o=5),
         R=dict(label="Dose them", l=15, s=-10, t=-5)),
    dict(id="physician_leeches", who="physician",
         text="Your blood is thin, $R. Leeches, or a spice tonic and very strange dreams?",
         L=dict(label="Leeches", l=-5, o=5),
         R=dict(label="Spice tonic", l=-5, do="presc+1")),

    # ------------------------------------------------------------ swordmaster
    dict(id="swordmaster_bet", who="swordmaster",
         text="Spar with me, pup. Lose and you buy my wine for a year. Win and I buy yours.",
         L=dict(label="Not today", l=-5, s=5),
         R=dict(label="En garde", l=10, s=-10)),
    dict(id="swordmaster_retire", who="swordmaster",
         text="My sword arm shakes. Let me retire to the open sand, or let me die in your service.",
         L=dict(label="Retire, old friend", l=-10, t=5),
         R=dict(label="Serve to the end", l=10, o=-5),
         when=["yr10"], reign_once=True),
    dict(id="swordmaster_shields", who="swordmaster",
         text="New body shields for the guard. They hum and attract worms, but stop blades cold.",
         L=dict(label="Too loud", l=-5, t=5),
         R=dict(label="Buy them", l=10, s=-5, t=-5)),
]


if __name__ == "__main__":
    import re
    import sys

    errors = []

    def err(msg):
        errors.append(msg)

    CHARACTERS = {
        "advisor", "sister", "naib", "marshal", "envoy", "navigator", "baron",
        "smuggler", "planetologist", "heir", "swordmaster", "harvester",
        "prophet", "worm", "consort", "physician", "stranger",
    }
    BUILTIN = {"yr5", "yr10", "yr20", "old", "first", "heir_full", "heir_last",
               "seer", "newreign", "tutorial"}
    FATE_ORDER = ["order_low", "order_high", "tribes_low", "tribes_high",
                  "legion_low", "legion_high", "spice_low", "spice_high",
                  "worm", "old_age", "duel", "faceless", "hunter_seeker",
                  "holy_war", "storm", "green_world"]
    METERS = "otls"
    CARD_KEYS = {"id", "who", "text", "L", "R", "when", "weight", "once",
                 "reign_once", "chain", "priority"}
    SIDE_KEYS = {"label", "o", "t", "l", "s", "set", "clear", "next", "do"}
    COND_RE = re.compile(r"^([otls])(>=|<)(\d+)$")
    DO_RE = re.compile(r"^(presc\+[123]|heir\+1|heir-1|fate:([a-z_]+)|age\+(\d+)|chance)$")

    def tlen(s):
        return len(s.replace("$R", "R" * 8).replace("$D", "D" * 8))

    def ascii_ok(s):
        return isinstance(s, str) and all(32 <= ord(c) <= 126 for c in s)

    # ---- fates
    if len(FATES) != 16:
        err("FATES must have 16 entries, has %d" % len(FATES))
    for i, f in enumerate(FATES):
        if len(f) != 3:
            err("fate %d malformed" % i)
            continue
        fid, title, text = f
        if i < len(FATE_ORDER) and fid != FATE_ORDER[i]:
            err("fate %d id %r should be %r" % (i, fid, FATE_ORDER[i]))
        if not ascii_ok(title) or tlen(title) > 20:
            err("fate %s title bad/too long (%d)" % (fid, tlen(title)))
        if not ascii_ok(text) or tlen(text) > 150:
            err("fate %s text bad/too long (%d)" % (fid, tlen(text)))
    fate_ids = [f[0] for f in FATES]

    # ---- cards
    if not 100 <= len(CARDS) <= 160:
        err("card count %d not in 100..160" % len(CARDS))
    ids = [c.get("id") for c in CARDS]
    by_id = {}
    for c in CARDS:
        if c.get("id") in by_id:
            err("duplicate id %s" % c.get("id"))
        by_id[c.get("id")] = c

    flags_set, flags_clear, flags_when = set(), set(), set()
    next_targets = set()
    fates_used = set()
    sums_up = dict.fromkeys(METERS, 0)
    sums_dn = dict.fromkeys(METERS, 0)
    per_who = {}
    text_bytes = sum(len(f[1]) + len(f[2]) for f in FATES)

    tutorial_ids = set()
    start = [c for c in CARDS if "tutorial" in c.get("when", [])]
    if len(start) != 1:
        err("exactly one tutorial entry card required, found %d" % len(start))
    else:
        t0 = start[0]
        if not (t0.get("priority") and t0.get("once") and t0.get("who") == "advisor"
                and set(t0.get("when", [])) == {"tutorial", "newreign"}):
            err("tutorial entry card must be advisor, priority, once, when=[tutorial,newreign]")
        cur = t0
        while cur is not None and cur["id"] not in tutorial_ids:
            if cur is not t0 and not cur.get("chain"):
                break
            tutorial_ids.add(cur["id"])
            nxt = {cur[s].get("next") for s in "LR"} - {None}
            cur = by_id.get(next(iter(nxt))) if len(nxt) == 1 else None
        if not 2 <= len(tutorial_ids) <= 3:
            err("tutorial must be 2-3 chained cards, got %d" % len(tutorial_ids))
        for tid in tutorial_ids:
            for s in "LR":
                side = by_id[tid][s]
                if set(side) - {"label", "next"}:
                    err("tutorial card %s side %s must have no effects" % (tid, s))

    for c in CARDS:
        cid = c.get("id", "?")
        for k in c:
            if k not in CARD_KEYS:
                err("%s: unknown key %s" % (cid, k))
        if not isinstance(cid, str) or not re.match(r"^[a-z][a-z0-9_]*$", cid):
            err("%s: id not snake_case" % cid)
        who = c.get("who")
        if who not in CHARACTERS:
            err("%s: unknown who %r" % (cid, who))
        per_who[who] = per_who.get(who, 0) + 1
        text = c.get("text", "")
        if not ascii_ok(text) or "\n" in text:
            err("%s: text not printable ASCII" % cid)
        if tlen(text) > 100:
            err("%s: text too long (%d)" % (cid, tlen(text)))
        text_bytes += len(text)
        w = c.get("weight", 2)
        if not isinstance(w, int) or not 1 <= w <= 4:
            err("%s: weight %r" % (cid, w))
        for bk in ("once", "reign_once", "chain", "priority"):
            if bk in c and not isinstance(c[bk], bool):
                err("%s: %s must be bool" % (cid, bk))
        when = c.get("when", [])
        if not isinstance(when, list) or len(when) > 3:
            err("%s: when must be list of <=3" % cid)
        for w_ in when:
            m = COND_RE.match(w_)
            if m:
                x = int(m.group(3))
                if x % 5 or not 20 <= x <= 95:
                    err("%s: bad threshold %s" % (cid, w_))
                continue
            name = w_[1:] if w_.startswith("!") else w_
            if not re.match(r"^[a-z][a-z0-9_]*$", name):
                err("%s: bad when entry %r" % (cid, w_))
            if name not in BUILTIN:
                flags_when.add(name)

        tut = cid in tutorial_ids
        side_eff = {}
        for s in "LR":
            side = c.get(s)
            if not isinstance(side, dict):
                err("%s: missing side %s" % (cid, s))
                continue
            for k in side:
                if k not in SIDE_KEYS:
                    err("%s.%s: unknown key %s" % (cid, s, k))
            lab = side.get("label")
            if not lab or not ascii_ok(lab) or tlen(lab) > 18:
                err("%s.%s: bad label %r" % (cid, s, lab))
            else:
                text_bytes += len(lab)
            eff = {}
            for m in METERS:
                if m in side:
                    v = side[m]
                    if not isinstance(v, int) or v == 0 or v % 5 or not -40 <= v <= 35:
                        err("%s.%s: bad %s=%r" % (cid, s, m, v))
                    eff[m] = v
                    if v > 0:
                        sums_up[m] += v
                    else:
                        sums_dn[m] -= v
            side_eff[s] = eff
            if "set" in side and "clear" in side:
                err("%s.%s: both set and clear" % (cid, s))
            for k, bucket in (("set", flags_set), ("clear", flags_clear)):
                if k in side:
                    f = side[k]
                    if f in BUILTIN:
                        err("%s.%s: cannot %s builtin %s" % (cid, s, k, f))
                    bucket.add(f)
            if "next" in side:
                next_targets.add(side["next"])
                if side["next"] not in by_id:
                    err("%s.%s: next %r missing" % (cid, s, side["next"]))
            if "do" in side:
                d = side["do"]
                m = DO_RE.match(d) if isinstance(d, str) else None
                if not m:
                    err("%s.%s: bad do %r" % (cid, s, d))
                else:
                    if m.group(2):
                        fates_used.add(m.group(2))
                        if m.group(2) not in fate_ids:
                            err("%s.%s: unknown fate %s" % (cid, s, m.group(2)))
                    if m.group(3) and not 1 <= int(m.group(3)) <= 15:
                        err("%s.%s: age out of range" % (cid, s))
                    if d == "chance" and "next" not in side:
                        err("%s.%s: chance requires next" % (cid, s))
            if not tut and not eff:
                err("%s.%s: non-tutorial side has no meter effect" % (cid, s))
        if not tut and len(side_eff) == 2:
            pure = all(set(c[s]) <= {"label", "o", "t", "l", "s"} for s in "LR")
            if pure:
                a, b = side_eff["L"], side_eff["R"]
                ge = all(a.get(m, 0) >= b.get(m, 0) for m in METERS)
                le = all(a.get(m, 0) <= b.get(m, 0) for m in METERS)
                if ge or le:
                    err("%s: one side strictly dominates" % cid)

    for f in flags_set | flags_clear:
        if f.startswith("!") or not re.match(r"^[a-z][a-z0-9_]*$", f):
            err("bad flag name %r" % f)
    all_flags = flags_set | flags_clear | flags_when
    for f in sorted(all_flags - flags_set):
        err("flag %s used but never set" % f)
    if len(all_flags) > 50:
        err("too many flags: %d" % len(all_flags))
    dyn_used = {f for f in all_flags if f.startswith("d_")}
    for f in FLAGS_DYNASTY:
        if not f.startswith("d_"):
            err("dynasty flag %s must start with d_" % f)
    if set(FLAGS_DYNASTY) != dyn_used:
        err("FLAGS_DYNASTY mismatch: declared-only %s, used-only %s"
            % (sorted(set(FLAGS_DYNASTY) - dyn_used), sorted(dyn_used - set(FLAGS_DYNASTY))))
    if len(FLAGS_DYNASTY) != len(set(FLAGS_DYNASTY)):
        err("duplicate FLAGS_DYNASTY entries")

    for c in CARDS:
        if c.get("chain") and c["id"] not in next_targets:
            err("chain card %s is unreachable" % c["id"])
    for fid in FATE_ORDER[8:]:
        if fid not in fates_used:
            err("special fate %s unreachable" % fid)

    def has(c, conds):
        return all(x in c.get("when", []) for x in conds)
    if not any(has(c, ["newreign", "first"]) and c.get("priority") and c.get("reign_once")
               and c["id"] not in tutorial_ids for c in CARDS):
        err("missing first-ruler coronation card")
    if not any(has(c, ["newreign", "!first"]) and c.get("priority") and c.get("reign_once")
               for c in CARDS):
        err("missing successor coronation card")

    for m in METERS:
        up, dn = sums_up[m], sums_dn[m]
        if abs(up - dn) > 0.15 * max(up, dn):
            err("meter %s unbalanced: up %d down %d" % (m, up, dn))

    # ---- stats
    print("cards: %d  fates: %d  flags: %d (dynasty %d)"
          % (len(CARDS), len(FATES), len(all_flags), len(FLAGS_DYNASTY)))
    for m, name in zip(METERS, ("ORDER", "TRIBES", "LEGION", "SPICE")):
        print("  %-6s up %4d  down %4d" % (name, sums_up[m], sums_dn[m]))
    print("per character:")
    for who in sorted(per_who):
        print("  %-14s %d" % (who, per_who[who]))
    print("total text bytes: %d" % text_bytes)
    if errors:
        print("\n%d ERRORS:" % len(errors))
        for e in errors:
            print("  " + e)
        sys.exit(1)
    print("OK")
