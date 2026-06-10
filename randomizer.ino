// ============================================================
//  randomizer.ino  –  DRUM_2025_VSAMPLER
//
//  Each RAND BT press:
//    1. Cycles genre (ROCK→REGGAE→JAZZ→HIPHOP→LATIN→EUCLIDEAN)
//    2. Picks new samples from role-appropriate pools
//    3. Refreshes parameter display immediately
//
//  ── TUNE THE POOLS BELOW to match your actual samples ────────
//  Open your SOUNDS folder, listen to each file, then move its
//  index number into the correct pool.
// ============================================================

#define RAND_NUM_STEPS   16
#define RAND_NUM_VOICES  16

// ── Genre IDs ────────────────────────────────────────────────
#define RAND_MODE_EUCLIDEAN  0
#define RAND_MODE_ROCK       1
#define RAND_MODE_REGGAE     2
#define RAND_MODE_JAZZ       3
#define RAND_MODE_HIPHOP     4
#define RAND_MODE_LATIN      5
#define RAND_NUM_MODES       6

static const char* RAND_LABELS[RAND_NUM_MODES] = {
    "RAND BT ", "  ROCK  ", " REGGAE ",
    "  JAZZ  ", "HIP HOP ", " LATIN  "
};

// ════════════════════════════════════════════════════════════
//  SAMPLE POOLS  —  edit freely
//
//  Index 0–49 maps to SAMPLE00–SAMPLE49 / SYNTH1 / SYNTH4 / SYNTH6
//    32 = SYNTH1    40 = SYNTH4    43 = SYNTH6
//
//  Default groupings assume a typical drum pack ordering:
//    0– 7  Kick / bass drum variants
//    8–15  Snare / rimshot variants
//   16–23  Closed hi-hat variants
//   24–31  Open hi-hat / cymbal / ride variants
//   32     SYNTH1  (synth bass)
//   33–39  Tom / bongo / conga variants
//   40     SYNTH4  (synth pad)
//   41–42  Misc percussion
//   43     SYNTH6  (synth lead)
//   44–49  Clap / snap / shaker variants
// ════════════════════════════════════════════════════════════

static const uint8_t P_KICK[]   = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const uint8_t P_SNARE[]  = { 8, 9,10,11,12,13,14,15 };
static const uint8_t P_CHIHAT[] = {16,17,18,19,20,21,22,23 };
static const uint8_t P_OHIHAT[] = {24,25,26,27,28,29,30,31 };
static const uint8_t P_TOM[]    = {33,34,35,36,37,38,39    };
static const uint8_t P_CLAP[]   = {44,45,46,47,48,49       };
static const uint8_t P_PERC[]   = {41,42,44,45,46          };
static const uint8_t P_SYNTH[]  = {32,40,43                };

// Full pool — used for voices 12-15 for maximum variety
static const uint8_t P_ALL[] = {
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49
};

#define PSIZ(a) ((uint8_t)(sizeof(a)/sizeof(a[0])))

// ── Voice 0–7 → pool mapping ──────────────────────────────────
//   0=Kick  1=Snare  2=CHH  3=OHH  4=Tom1  5=Tom2  6=Clap  7=Perc
static const uint8_t* const V_POOL[8] = {
    P_KICK, P_SNARE, P_CHIHAT, P_OHIHAT,
    P_TOM,  P_TOM,   P_CLAP,   P_PERC
};
static const uint8_t V_POOL_SZ[8] = {
    PSIZ(P_KICK),  PSIZ(P_SNARE),  PSIZ(P_CHIHAT), PSIZ(P_OHIHAT),
    PSIZ(P_TOM),   PSIZ(P_TOM),    PSIZ(P_CLAP),   PSIZ(P_PERC)
};

// ── Euclidean density weights per voice ──────────────────────
static const uint8_t VOICE_WEIGHT[RAND_NUM_VOICES] = {
    55, 48, 78, 28, 42, 35, 46, 24,   // 0-7  drum voices
    50, 50, 50, 50, 50, 50, 50, 50    // 8-15 aux voices
};

// ── Genre bitmask patterns ────────────────────────────────────
static const uint16_t GENRE_PATTERNS[RAND_NUM_MODES][8] = {
    // EUCLIDEAN — generated dynamically
    {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
    // ROCK: Kick 1&3, Snare 2&4, CHH all 16ths, OHH beat 3
    {0x0101,0x1010,0xFFFF,0x0100,0x0000,0x0000,0x0000,0x0000},
    // REGGAE: one-drop kick, off-beat HH, clap off-beats
    {0x0100,0x1010,0x4444,0x0010,0x0000,0x0000,0x4444,0x0000},
    // JAZZ: sparse kick, ride comping, ghost snare
    {0x0401,0x1010,0x2929,0x1000,0x0000,0x0200,0x0000,0x0000},
    // HIP HOP: heavy kick, 8th HH, clap beat 4
    {0x0501,0x1010,0x5555,0x0100,0x0000,0x0000,0x1000,0x0000},
    // LATIN: son clave 3:2, conga pattern
    {0x0101,0x1449,0x5555,0x0000,0x2124,0x0000,0x0000,0x0000},
};

// ── Module state ─────────────────────────────────────────────
static uint8_t _rand_mode    = 0;
static uint8_t _rand_density = 50;
static bool    _rand_seeded  = false;

// ── Internal helpers ──────────────────────────────────────────

// ─────────────────────────────────────────────────────────────
//  _randomizeSounds()
//  Picks a new sample for every voice from its role-pool,
//  then applies it and refreshes the parameter bar display.
// ─────────────────────────────────────────────────────────────
static void _randomizeSounds() {

    // Voices 0–7: dedicated drum role pools
    for (int v = 0; v < 8; v++) {
        ROTvalue[v][0] = V_POOL[v][ random(V_POOL_SZ[v]) ];
        setSound(v);
    }

    // Voices 8–11: synth / melodic texture
    for (int v = 8; v < 12; v++) {
        ROTvalue[v][0] = P_SYNTH[ random(PSIZ(P_SYNTH)) ];
        setSound(v);
    }

    // Voices 12–15: anything goes — full pool for maximum variety
    for (int v = 12; v < RAND_NUM_VOICES; v++) {
        ROTvalue[v][0] = P_ALL[ random(PSIZ(P_ALL)) ];
        setSound(v);
    }

    // Refresh the parameter bars so SAM column shows new values
    draw8aBar();
}

static void _euclid(uint8_t* buf, int k, int n) {
    memset(buf, 0, n);
    if (k <= 0)  return;
    if (k >= n) { memset(buf, 1, n); return; }
    int acc = 0;
    for (int i = 0; i < n; i++) {
        acc += k;
        if (acc >= n) { acc -= n; buf[i] = 1; }
    }
}

static void _rotate(uint8_t* buf, int n, int r) {
    if (n <= 1) return;
    r = ((r % n) + n) % n;
    if (r == 0) return;
    uint8_t tmp[32];
    for (int i = 0; i < n; i++) tmp[(i + n - r) % n] = buf[i];
    memcpy(buf, tmp, n);
}

static void _writeToBitmask(int v, uint8_t* buf, int n) {
    pattern[v] = 0;
    for (int s = 0; s < n; s++)
        if (buf[s]) bitSet(pattern[v], s);
}

static void _euclid_voice(int v, int density) {
    uint8_t tmp[RAND_NUM_STEPS];
    float   wt = (v < 8) ? VOICE_WEIGHT[v] / 100.0f : 0.4f;
    int     k  = (int)roundf(RAND_NUM_STEPS * (density / 100.0f) * wt);
    k += (int)random(3) - 1;
    k  = constrain(k, 0, RAND_NUM_STEPS);
    _euclid(tmp, k, RAND_NUM_STEPS);
    _rotate(tmp, RAND_NUM_STEPS, (int)random(RAND_NUM_STEPS));
    _writeToBitmask(v, tmp, RAND_NUM_STEPS);
}

// ── Public API ────────────────────────────────────────────────

void rand_init() {
    if (_rand_seeded) return;
    randomSeed(esp_random());
    _rand_seeded = true;
}

const char* rand_get_label() {
    return RAND_LABELS[_rand_mode];
}

void rand_pattern() {
    _rand_mode = (_rand_mode + 1) % RAND_NUM_MODES;

    // ── Step 1: Set rhythm pattern ────────────────────────────
    if (_rand_mode == RAND_MODE_EUCLIDEAN) {
        for (int v = 0; v < RAND_NUM_VOICES; v++)
            _euclid_voice(v, _rand_density);
    } else {
        for (int v = 0; v < 8; v++)
            pattern[v] = GENRE_PATTERNS[_rand_mode][v];
        for (int v = 8; v < RAND_NUM_VOICES; v++)
            _euclid_voice(v, 20);
    }

    // ── Step 2: Randomize sound assignments ───────────────────
    _randomizeSounds();

    // Debug — shows which samples were chosen
    Serial.printf("[RAND] %s  K=%d Sn=%d HH=%d  snd0=%d snd1=%d snd2=%d\n",
                  RAND_LABELS[_rand_mode],
                  (int)ROTvalue[0][0],
                  (int)ROTvalue[1][0],
                  (int)ROTvalue[2][0],
                  pattern[0], pattern[1], pattern[2]);
}

void rand_set_density(int d) {
    _rand_density = (uint8_t)constrain(d, 0, 100);
}