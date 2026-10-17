/* World simulation: economy, construction, training, marches and the
 * rival Houses. world_tick() advances one game second. */
#include "game.h"

Game G;

/* ---------------------------------------------------------------- rng */
static uint32_t rng = 1;

uint16_t rnd16(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (uint16_t)(rng >> 8);
}

uint8_t rnd(uint8_t n) { return (uint8_t)(((uint32_t)(rnd16() & 0xFF) * n) >> 8); }

/* ------------------------------------------------------------- tables */
/*                                   hp atk rng spd cd  $   P   W  t  carry */
const UnitInfo unit_info[NUNITS] PROGMEM = {
    { 12, 3,  6, 4,  6, 30, 10, 10,  4, 20 },   /* levy      */
    { 30, 5,  6, 3,  8, 60, 40, 15,  8, 10 },   /* trooper   */
    { 12, 3, 26, 2, 11, 50, 30, 15,  7, 10 },   /* rifleman  */
    { 20, 7,  6, 6,  7, 80, 30, 30, 10, 40 },   /* sandrider */
    { 24, 6, 20, 6,  9, 120, 100, 20, 14, 30 }, /* thopter   */
};

/* spice, plasteel, seconds for level 1 */
static const uint8_t bld_base[NBUILD][3] PROGMEM = {
    { 150, 100, 20 }, { 60, 40, 10 }, { 50, 50, 10 }, { 80, 20, 12 },
    { 70, 60, 12 }, { 100, 80, 15 }, { 60, 120, 15 }, { 150, 150, 20 },
};
static const uint8_t cost_mult[10] PROGMEM = { 10, 15, 23, 34, 51, 76, 114, 171, 255, 255 };
static const uint8_t time_mult[10] PROGMEM = { 10, 14, 20, 28, 39, 55, 77, 108, 150, 200 };

const uint8_t node_xy[NNODES][2] PROGMEM = {
    { 62, 118 }, { 22, 100 }, { 102, 102 }, { 9, 70 }, { 62, 82 },
    { 117, 66 }, { 32, 50 }, { 92, 44 }, { 16, 29 }, { 108, 28 },
};
const uint8_t node_kind[NNODES] PROGMEM = {
    K_CITADEL, K_FIELD, K_FIELD, K_SIETCH, K_FIELD, K_SIETCH, K_OUTPOST, K_OUTPOST, K_CITADEL, K_CITADEL,
};
static const uint8_t node_init[NNODES][1 + NUNITS] PROGMEM = {
    { OWN_PLAYER, 0, 0, 0, 0, 0 },
    { OWN_NONE, 3, 0, 0, 0, 0 },
    { OWN_NONE, 4, 0, 0, 0, 0 },
    { OWN_TRIBES, 8, 0, 0, 6, 0 },
    { OWN_NONE, 6, 2, 0, 0, 0 },
    { OWN_TRIBES, 8, 0, 0, 6, 0 },
    { OWN_VULKAR, 6, 4, 0, 0, 0 },
    { OWN_ORDANE, 5, 2, 4, 0, 0 },
    { OWN_VULKAR, 16, 10, 4, 0, 0 },
    { OWN_ORDANE, 12, 6, 10, 0, 1 },
};

/* --------------------------------------------------------------- misc */
static uint8_t kind(uint8_t n) { return RD8(&node_kind[n]); }

static uint8_t dist(uint8_t a, uint8_t b)
{
    int16_t dx = (int16_t)RD8(&node_xy[a][0]) - RD8(&node_xy[b][0]);
    int16_t dy = (int16_t)RD8(&node_xy[a][1]) - RD8(&node_xy[b][1]);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (uint8_t)(dx > dy ? dx + dy / 2 : dy + dx / 2);   /* octagonal approximation */
}

uint16_t army_total(const uint8_t *u)
{
    uint16_t t = 0;
    for (uint8_t i = 0; i < NUNITS; i++) t += u[i];
    return t;
}

static void add_units(uint8_t *dst, const uint8_t *src)
{
    for (uint8_t i = 0; i < NUNITS; i++) {
        uint16_t v = (uint16_t)dst[i] + src[i];
        dst[i] = (uint8_t)(v > 250 ? 250 : v);
    }
}

uint16_t player_troops(void)
{
    uint16_t t = army_total(G.home);
    for (uint8_t n = 1; n < NNODES; n++)
        if (G.node[n].owner == OWN_PLAYER) t += army_total(G.node[n].g);
    for (uint8_t m = 0; m < NMARCH; m++)
        if (G.march[m].mission && G.march[m].owner == OWN_PLAYER) t += army_total(G.march[m].u);
    if (G.train_n) t++;
    return t;
}

static uint8_t owned(uint8_t k)
{
    uint8_t c = 0;
    for (uint8_t n = 1; n < NNODES; n++)
        if (G.node[n].owner == OWN_PLAYER && kind(n) == k) c++;
    return c;
}

/* ------------------------------------------------------------ economy */
uint16_t world_cap(void) { return (uint16_t)(300 + 250 * G.bld[B_WAREHOUSE]); }

int16_t world_rate(uint8_t r)
{
    int16_t v;
    if (r == R_SPICE) {
        v = (int16_t)(40 + 25 * G.bld[B_REFINERY] + 40 * owned(K_FIELD) + 15 * owned(K_OUTPOST));
        if (G.meter[M_GUILD] >= 70) v += v / 4;
        else if (G.meter[M_GUILD] < 30) v -= v / 4;
    } else if (r == R_WATER) {
        uint8_t sietches = owned(K_SIETCH);
        if (G.meter[M_TRIBES] >= 70) sietches = 2;
        v = (int16_t)(25 + 20 * G.bld[B_WINDTRAP] + 25 * sietches) - (int16_t)(player_troops() / 2);
    } else {
        v = (int16_t)(25 + 20 * G.bld[B_FOUNDRY] + 15 * owned(K_OUTPOST));
    }
    return v;
}

uint8_t build_max(uint8_t b)
{
    if (b == B_KEEP) return 7;
    uint8_t m = (uint8_t)(G.bld[B_KEEP] + 2);
    return m > 9 ? 9 : m;
}

void build_cost(uint8_t b, uint16_t *s, uint16_t *p, uint16_t *t)
{
    uint8_t l = G.bld[b];
    uint8_t cm = RD8(&cost_mult[l]);
    *s = (uint16_t)((uint16_t)RD8(&bld_base[b][0]) * cm / 10);
    *p = (uint16_t)((uint16_t)RD8(&bld_base[b][1]) * cm / 10);
    *t = (uint16_t)((uint32_t)RD8(&bld_base[b][2]) * RD8(&time_mult[l]) * (100 - 8 * G.bld[B_KEEP]) / 1000);
    if (*t < 3) *t = 3;
}

uint8_t build_start(uint8_t b)
{
    uint16_t s, p, t;
    if (G.build_id != NONE) return 1;
    if (G.bld[b] >= build_max(b)) return 2;
    build_cost(b, &s, &p, &t);
    if (G.res[R_SPICE] < s || G.res[R_STEEL] < p) return 3;
    G.res[R_SPICE] -= s;
    G.res[R_STEEL] -= p;
    G.build_id = b;
    G.build_left = t;
    return 0;
}

uint8_t unit_unlocked(uint8_t u)
{
    switch (u) {
    case U_LEVY:    return G.bld[B_BARRACKS] >= 1;
    case U_TROOPER: return G.bld[B_BARRACKS] >= 2;
    case U_RIFLE:   return G.bld[B_BARRACKS] >= 3;
    case U_RIDER:   return G.bld[B_BARRACKS] >= 2 && G.meter[M_TRIBES] >= 70;
    default:        return G.bld[B_PAD] >= 1;
    }
}

static uint8_t train_time(uint8_t u)
{
    uint8_t t = (uint8_t)((uint16_t)UI8(u, time) * (100 - 6 * G.bld[B_BARRACKS]) / 100);
    return t ? t : 1;
}

uint8_t train_start(uint8_t u, uint8_t n)
{
    if (!unit_unlocked(u)) return 1;
    if (G.train_n && G.train_type != u) return 3;
    if (G.res[R_SPICE] < (uint16_t)UI8(u, cost_s) * n || G.res[R_STEEL] < (uint16_t)UI8(u, cost_p) * n
        || G.res[R_WATER] < (uint16_t)UI8(u, cost_w) * n)
        return 2;
    G.res[R_SPICE] -= (uint16_t)UI8(u, cost_s) * n;
    G.res[R_STEEL] -= (uint16_t)UI8(u, cost_p) * n;
    G.res[R_WATER] -= (uint16_t)UI8(u, cost_w) * n;
    if (!G.train_n) G.train_left = train_time(u);
    G.train_type = u;
    G.train_n = (uint8_t)(G.train_n + n > 250 ? 250 : G.train_n + n);
    return 0;
}

/* ------------------------------------------------------------ marches */
static March *march_new(uint8_t owner, uint8_t from, uint8_t to, uint8_t mission, const uint8_t *u)
{
    for (uint8_t i = 0; i < NMARCH; i++) {
        March *m = &G.march[i];
        if (m->mission) continue;
        uint8_t slow = 9;
        for (uint8_t k = 0; k < NUNITS; k++)
            if (u[k] && UI8(k, speed) < slow) slow = UI8(k, speed);
        uint16_t t = (uint16_t)(dist(from, to) * 4 / (slow + 2));
        if (owner == OWN_PLAYER) t = (uint16_t)(t * (10 - G.bld[B_PAD]) / 10);
        if (t < 5) t = 5;
        m->mission = mission;
        m->owner = owner;
        m->from = from;
        m->to = to;
        memcpy(m->u, u, NUNITS);
        m->eta = m->total = t;
        m->loot = 0;
        return m;
    }
    return 0;
}

uint8_t march_send(uint8_t to, uint8_t mission, uint8_t pct)
{
    uint8_t u[NUNITS];
    for (uint8_t i = 0; i < NUNITS; i++) u[i] = (uint8_t)(((uint16_t)G.home[i] * pct + 99) / 100);
    if (!army_total(u)) return 1;
    if (!march_new(OWN_PLAYER, N_HOME, to, mission, u)) return 2;
    for (uint8_t i = 0; i < NUNITS; i++) G.home[i] -= u[i];
    if (G.node[to].owner == OWN_TRIBES && G.meter[M_TRIBES] >= 15) G.meter[M_TRIBES] -= 15;
    return 0;
}

uint8_t recall(uint8_t n)
{
    if (!army_total(G.node[n].g)) return 1;
    if (!march_new(OWN_PLAYER, n, N_HOME, MS_RETURN, G.node[n].g)) return 2;
    memset(G.node[n].g, 0, NUNITS);
    return 0;
}

uint8_t threat(uint16_t *eta, uint16_t *total, uint8_t *who, uint8_t *where)
{
    uint8_t found = 0;
    *eta = 0xFFFF;
    for (uint8_t i = 0; i < NMARCH; i++) {
        March *m = &G.march[i];
        if (!m->mission || m->mission == MS_RETURN || m->owner == OWN_PLAYER) continue;
        if (G.node[m->to].owner != OWN_PLAYER) continue;
        if (m->eta < *eta) {
            *eta = m->eta;
            *total = m->total;
            *who = m->owner;
            *where = m->to;
            found = 1;
        }
    }
    return found;
}

static const char s_won[] PROGMEM = "Victory! Spoils sent home";
static const char s_lost[] PROGMEM = "Our army was destroyed";
static const char s_held[] PROGMEM = "Our walls held!";
static const char s_fell[] PROGMEM = "We lost a territory";
static const char s_home[] PROGMEM = "Army returned home";
static const char s_built[] PROGMEM = "Construction complete";
static const char s_desert[] PROGMEM = "No water: troops desert!";
static const char s_worm[] PROGMEM = "A worm ate a garrison!";
static const char s_council[] PROGMEM = "A petitioner waits";
static const char s_taken[] PROGMEM = "Territory conquered!";

static void eliminate(uint8_t owner)
{
    for (uint8_t n = 0; n < NNODES; n++)
        if (G.node[n].owner == owner) G.node[n].owner = OWN_NONE;
    for (uint8_t i = 0; i < NMARCH; i++)
        if (G.march[i].owner == owner) G.march[i].mission = MS_NONE;
}

static uint8_t arrive(March *m)
{
    uint8_t to = m->to;
    Node *n = &G.node[to];
    uint8_t mission = m->mission;
    m->mission = MS_NONE;

    if (mission == MS_RETURN) {
        add_units(G.home, m->u);
        uint16_t s = G.res[R_SPICE] + m->loot;
        G.res[R_SPICE] = s > world_cap() ? world_cap() : s;
        if (m->owner == OWN_PLAYER) status(s_home, C_SAND);
        return 0;
    }
    uint8_t def_owner = n->owner;
    uint8_t *def = (to == N_HOME && def_owner == OWN_PLAYER) ? G.home : n->g;
    if (def_owner == m->owner) {
        add_units(def, m->u);
        return 0;
    }
    uint8_t k = kind(to);
    uint8_t wall = k == K_CITADEL ? (def_owner == OWN_PLAYER ? (uint8_t)(G.bld[B_WALL] + 2) : 3) : k == K_OUTPOST ? 1 : 0;
    uint8_t player = (m->owner == OWN_PLAYER || def_owner == OWN_PLAYER);
    /* citizens take up spears when the citadel itself is attacked */
    uint8_t militia = 0;
    if (def == G.home) {
        militia = (uint8_t)(6 + 5 * G.bld[B_KEEP]);
        uint8_t one[NUNITS] = { militia, 0, 0, 0, 0 };
        add_units(G.home, one);
    }
    uint8_t won = battle(m->u, m->owner, def, def_owner, to, wall, player);
    if (militia) G.home[U_LEVY] = G.home[U_LEVY] > militia ? (uint8_t)(G.home[U_LEVY] - militia) : 0;

    if (!won) {
        if (m->owner == OWN_PLAYER) status(s_lost, C_RED);
        else if (def_owner == OWN_PLAYER) status(s_held, C_GREEN);
        return player;
    }
    if (mission == MS_CONQUER || to != N_HOME || m->owner == OWN_PLAYER) {
        if (mission == MS_CONQUER) {
            if (def_owner == OWN_PLAYER) status(s_fell, C_RED);
            n->owner = m->owner;
            memcpy(n->g, m->u, NUNITS);
            if (to == N_HOME) G.result = 2;
            if (m->owner == OWN_PLAYER) {
                status(s_taken, C_GREEN);
                if (to == N_VULKAR) eliminate(OWN_VULKAR);
                if (to == N_ORDANE) eliminate(OWN_ORDANE);
                n->owner = OWN_PLAYER;
                if (G.node[N_VULKAR].owner != OWN_VULKAR && G.node[N_ORDANE].owner != OWN_ORDANE)
                    G.result = 1;
            }
            return player;
        }
        /* raid: carry spoils back */
        uint16_t carry = 0;
        for (uint8_t i = 0; i < NUNITS; i++) carry += (uint16_t)m->u[i] * UI8(i, carry);
        if (m->owner == OWN_PLAYER) {
            uint16_t loot = (uint16_t)(60 + rnd(80) + (k == K_CITADEL ? 250 : k == K_OUTPOST ? 100 : 0));
            March *r = march_new(OWN_PLAYER, to, N_HOME, MS_RETURN, m->u);
            if (r) r->loot = loot < carry ? loot : carry;
            status(s_won, C_GREEN);
        } else {
            add_units(G.node[m->owner == OWN_VULKAR ? N_VULKAR : N_ORDANE].g, m->u);
        }
        return player;
    }
    /* an AI raid broke into the citadel: plunder, then go home */
    uint16_t carry = 0;
    for (uint8_t i = 0; i < NUNITS; i++) carry += (uint16_t)m->u[i] * UI8(i, carry);
    for (uint8_t r = 0; r < NRES; r++) G.res[r] = G.res[r] > carry ? G.res[r] - carry : 0;
    status(s_fell, C_RED);
    uint8_t src = m->owner == OWN_VULKAR ? N_VULKAR : m->owner == OWN_ORDANE ? N_ORDANE : 3;
    add_units(G.node[src].g, m->u);
    return 1;
}

/* ------------------------------------------------------------------ AI */
void ai_attack(uint8_t r, uint8_t forced)
{
    uint8_t owner = (uint8_t)(OWN_VULKAR + r), src = r ? N_ORDANE : N_VULKAR;
    if (G.node[src].owner != owner) return;
    uint8_t day = (uint8_t)(G.secs / DAY_SECS);
    uint8_t best = NONE;
    uint16_t best_score = 0xFFFF;
    for (uint8_t n = 0; n < NNODES; n++) {
        uint8_t o = G.node[n].owner;
        if (o == owner || o == OWN_TRIBES) continue;
        if (r == 0 && o != OWN_PLAYER && !forced && n != 1 && n != 4) continue;
        uint16_t sc = (uint16_t)(army_total(n == N_HOME ? G.home : G.node[n].g) * 4 + dist(src, n) / 2);
        if (n == N_HOME && !forced && day < 15) sc += 60;
        if (o == OWN_PLAYER && r == 0) sc /= 2;
        if (sc < best_score) { best_score = sc; best = n; }
    }
    G.ai_t[r] = (uint16_t)(210 - (day * 3 > 100 ? 100 : day * 3) + rnd(60));
    if (best == NONE) return;
    uint8_t u[NUNITS], pct = (uint8_t)((day < 10 ? 30 : 45) + rnd(day < 10 ? 16 : 31));
    for (uint8_t i = 0; i < NUNITS; i++) u[i] = (uint8_t)((uint16_t)G.node[src].g[i] * pct / 100);
    if (army_total(u) < 4) return;
    uint8_t mission = (best == N_HOME && day < 12) ? MS_RAID : MS_CONQUER;
    if (march_new(owner, src, best, mission, u))
        for (uint8_t i = 0; i < NUNITS; i++) G.node[src].g[i] -= u[i];
}

static void ai_grow(void)
{
    uint8_t day = (uint8_t)(G.secs / DAY_SECS);
    uint8_t k = ++G.ai_grow;
    uint8_t *v = G.node[N_VULKAR].g, *o = G.node[N_ORDANE].g;
    uint8_t add[2][NUNITS] = {
        { (uint8_t)(day >= 10 || (k & 1)), (uint8_t)(k % 4 == 0), (uint8_t)(k % 6 == 0), 0, 0 },
        { (uint8_t)(k % 3 == 0), (uint8_t)(k % 5 == 0), (uint8_t)(k % 3 == 1), 0, (uint8_t)(day >= 12 && k % 10 == 0) },
    };
    if (day >= 20) { add[0][0]++; add[1][2]++; }
    if (G.node[N_VULKAR].owner == OWN_VULKAR) add_units(v, add[0]);
    if (G.node[N_ORDANE].owner == OWN_ORDANE) add_units(o, add[1]);
}

void node_lose_random(void)
{
    for (uint8_t tries = 0; tries < 20; tries++) {
        uint8_t n = (uint8_t)(1 + rnd(NNODES - 1));
        if (G.node[n].owner == OWN_PLAYER) {
            G.node[n].owner = OWN_NONE;
            memset(G.node[n].g, 0, NUNITS);
            G.node[n].g[U_LEVY] = 3;
            status(s_fell, C_RED);
            return;
        }
    }
}

void node_gain_field(void)
{
    for (uint8_t n = 1; n < NNODES; n++)
        if (G.node[n].owner == OWN_NONE && kind(n) == K_FIELD) {
            G.node[n].owner = OWN_PLAYER;
            memset(G.node[n].g, 0, NUNITS);
            status(s_taken, C_GREEN);
            return;
        }
}

/* --------------------------------------------------------------- tick */
uint8_t world_tick(void)
{
    uint8_t redraw = 0;
    G.secs++;

    uint16_t cap = world_cap();
    for (uint8_t r = 0; r < NRES; r++) {
        int16_t rt = world_rate(r);
        uint16_t a = (uint16_t)(G.acc[r] + (rt < 0 ? -rt : rt));
        uint16_t d = a / 60;
        G.acc[r] = (uint8_t)(a % 60);
        if (rt >= 0) {
            G.res[r] = (uint16_t)(G.res[r] + d > cap ? cap : G.res[r] + d);
        } else {
            G.res[r] = G.res[r] > d ? (uint16_t)(G.res[r] - d) : 0;
            if (!G.res[r] && r == R_WATER && G.secs % 10 == 0) {
                uint8_t big = 0;
                for (uint8_t i = 1; i < NUNITS; i++) if (G.home[i] > G.home[big]) big = i;
                if (G.home[big]) { G.home[big]--; status(s_desert, C_RED); }
            }
        }
    }

    if (G.build_id != NONE && !--G.build_left) {
        G.bld[G.build_id]++;
        G.build_id = NONE;
        status(s_built, C_GREEN);
        redraw = 1;
    }
    if (G.train_n && !--G.train_left) {
        if (G.home[G.train_type] < 250) G.home[G.train_type]++;
        if (--G.train_n) G.train_left = train_time(G.train_type);
        redraw = 1;
    }

    for (uint8_t i = 0; i < NMARCH; i++) {
        March *m = &G.march[i];
        if (m->mission && !--m->eta) redraw |= (uint8_t)(arrive(m) | 1);
    }

    if (G.secs % 20 == 0) ai_grow();
    if (G.secs > 90)
        for (uint8_t r = 0; r < 2; r++)
            if (!--G.ai_t[r]) ai_attack(r, 0);

    /* the Maker hunts rhythmic garrisons on the open sand */
    if (G.secs % 120 == 60 && G.meter[M_TRIBES] < 70) {
        uint8_t n = (uint8_t)(1 + rnd(NNODES - 1));
        if (G.node[n].owner == OWN_PLAYER && kind(n) == K_FIELD && army_total(G.node[n].g) && rnd(3) == 0) {
            for (uint8_t i = 0; i < NUNITS; i++) G.node[n].g[i] /= 2;
            status(s_worm, C_ORANGE);
        }
    }

    if (!G.council_t || !--G.council_t) {
        if (G.pending < 3) {
            G.pending++;
            status(s_council, C_BLUE);
            redraw = 1;
        }
        G.council_t = (uint16_t)(60 + rnd(40));
    }

    if (G.node[N_HOME].owner != OWN_PLAYER) G.result = 2;
    if (G.secs % 30 == 0) save_game();
    return redraw;
}

void world_new(void)
{
    uint32_t keep = rng;
    memset(&G, 0, sizeof G);
    rng = keep ? keep : 1;
    G.res[R_SPICE] = 300;
    G.res[R_WATER] = 200;
    G.res[R_STEEL] = 200;
    G.bld[B_KEEP] = 1;
    G.bld[B_REFINERY] = 1;
    G.bld[B_WINDTRAP] = 1;
    G.bld[B_WAREHOUSE] = 1;
    G.bld[B_BARRACKS] = 1;
    G.build_id = NONE;
    G.home[U_LEVY] = 8;
    G.home[U_TROOPER] = 3;
    for (uint8_t n = 0; n < NNODES; n++) {
        G.node[n].owner = RD8(&node_init[n][0]);
        for (uint8_t i = 0; i < NUNITS; i++) G.node[n].g[i] = RD8(&node_init[n][1 + i]);
    }
    for (uint8_t m = 0; m < 4; m++) G.meter[m] = 50;
    memset(G.recent, NONE, sizeof G.recent);
    G.charges = 1;
    G.pending = 1;
    G.council_t = 60;
    G.ai_t[0] = 240;
    G.ai_t[1] = 330;
    G.next_card = NONE;
    G.first = 1;
    G.house = rnd(8);
    G.name = rnd(16);
}

/* --------------------------------------------------------------- save */
#define EE_SAVE 32
#define SAVE_MAGIC 0xB2

static uint8_t checksum(void)
{
    const uint8_t *p = (const uint8_t *)&G;
    uint8_t s = 0x5A;
    for (uint16_t i = 0; i < sizeof G - 1; i++) s = (uint8_t)((s << 1 | s >> 7) ^ p[i]);
    return s;
}

void save_game(void)
{
    G.magic = G.result ? 0 : SAVE_MAGIC;
    G.sum = checksum();
    const uint8_t *p = (const uint8_t *)&G;
    for (uint16_t i = 0; i < sizeof G; i++) hal_ee_write((uint16_t)(EE_SAVE + i), p[i]);
}

uint8_t load_game(void)
{
    uint8_t *p = (uint8_t *)&G;
    uint32_t keep = rng;
    for (uint16_t i = 0; i < sizeof G; i++) p[i] = hal_ee_read((uint16_t)(EE_SAVE + i));
    if (G.magic != SAVE_MAGIC || G.sum != checksum()) return 0;
    rng = keep ^ G.secs;
    if (!rng) rng = 1;
    return 1;
}

void world_seed(uint32_t s) { rng = s ? s : 1; }
