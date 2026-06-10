// ============================================================
//  randomizer.ino  v2.0  —  Musical Pattern Randomizer
//  Button 30 — each press picks a DIFFERENT style
//  Button label briefly shows which style was selected
// ============================================================

// ---- SET THESE to match your current sound set -------------
// These are pattern[] array indices (0-15), not sound numbers
#define KICK_T    0    // Kick drum track
#define SNARE_T   1    // Snare drum track
#define HATC_T    2    // Closed hi-hat track
#define HATO_T    3    // Open hi-hat track
#define CLAP_T    4    // Clap / rimshot track
// Tracks 5-15: ultra-sparse scatter only

// ---- STYLE 0: Standard Rock --------------------------------
// Kick on 1 and 3, snare on 2 and 4, straight 8th hats
static const uint8_t K_ROCK[16] = {90,0,0,5, 0,0,0,10, 85,0,0,5, 10,0,35,5};
static const uint8_t S_ROCK[16] = {0,0,0,0, 90,0,5,0, 0,0,5,0, 85,0,10,5};
static const uint8_t HC_ROCK[16]= {80,0,80,0, 80,0,80,0, 80,0,80,0, 80,0,80,0};
static const uint8_t HO_ROCK[16]= {0,0,0,25, 0,0,0,30, 0,0,0,25, 0,0,0,40};

// ---- STYLE 1: Funk -----------------------------------------
// Syncopated kick, 16th note hats, open hat pre-beat
static const uint8_t K_FUNK[16] = {85,0,25,0, 15,0,0,10, 70,0,20,0, 0,10,0,20};
static const uint8_t S_FUNK[16] = {0,0,5,0, 80,0,10,0, 5,0,0,0, 75,0,15,10};
static const uint8_t HC_FUNK[16]= {70,35,70,35, 70,35,70,35, 70,35,70,35, 70,35,70,35};
static const uint8_t HO_FUNK[16]= {0,0,0,0, 0,0,0,45, 0,0,0,0, 0,0,0,50};

// ---- STYLE 2: Hip-Hop Sparse --------------------------------
// Minimal kick pattern, snare on 2&4 only, sparse hat
static const uint8_t K_HH[16]  = {90,0,0,0, 0,0,0,0, 65,0,0,30, 0,0,20,0};
static const uint8_t S_HH[16]  = {0,0,0,0, 85,0,0,0, 0,0,0,0, 80,0,0,0};
static const uint8_t HC_HH[16] = {55,0,0,55, 0,0,55,0, 0,55,0,0, 55,0,0,55};
static const uint8_t HO_HH[16] = {0,0,0,0, 0,0,0,60, 0,0,0,0, 0,0,0,0};

// ---- STYLE 3: Offbeat / Reggae feel -------------------------
// Kick on 1 only, snare displaced, offbeat hats
static const uint8_t K_OB[16]  = {80,0,0,0, 0,0,0,0, 0,0,0,0, 80,0,0,0};
static const uint8_t S_OB[16]  = {0,0,0,0, 85,0,0,0, 0,0,0,0, 0,0,85,0};
static const uint8_t HC_OB[16] = {0,75,0,75, 0,75,0,75, 0,75,0,75, 0,75,0,75};
static const uint8_t HO_OB[16] = {0,0,0,0, 0,0,0,0, 0,0,0,70, 0,0,0,0};

// ---- STYLE 4: Four-on-Floor / Dance -------------------------
// Kick every beat, snare on 2&4, steady hats
static const uint8_t K_4F[16]  = {95,0,0,0, 95,0,0,0, 95,0,0,0, 95,0,0,0};
static const uint8_t S_4F[16]  = {0,0,0,0, 85,0,0,0, 0,0,0,0, 85,0,5,0};
static const uint8_t HC_4F[16] = {70,70,70,70, 70,70,70,70, 70,70,70,70, 70,70,70,70};
static const uint8_t HO_4F[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,70,0};

// ---- Style name shown briefly on button --------------------
static const char* STYLE_NAME[] = {
    " ROCK  ", " FUNK  ", " H-HOP ", "REGGAE ", " DANCE "
};

static int last_rand_style = -1;   // prevents same style twice in a row

static inline bool rnd_hit(int pct) { return (int)random(100) < pct; }

static uint16_t build_pat(const uint8_t prob[16]) {
    uint16_t p = 0;
    for (int s = 0; s < 16; s++)
        if (rnd_hit(prob[s])) p |= (1 << s);
    return p;
}

// ============================================================
void rand_pattern() {
    // Pick a style — never repeat the same one twice
    int style;
    do { style = random(5); } while (style == last_rand_style);
    last_rand_style = style;

    // Clear all 16 tracks
    for (int f = 0; f < 16; f++) pattern[f] = 0;

    // Select probability tables for this style
    const uint8_t *kp, *sp, *hcp, *hop;
    switch (style) {
        case 0: kp=K_ROCK; sp=S_ROCK; hcp=HC_ROCK; hop=HO_ROCK; break;
        case 1: kp=K_FUNK; sp=S_FUNK; hcp=HC_FUNK; hop=HO_FUNK; break;
        case 2: kp=K_HH;   sp=S_HH;   hcp=HC_HH;   hop=HO_HH;   break;
        case 3: kp=K_OB;   sp=S_OB;   hcp=HC_OB;   hop=HO_OB;   break;
        default:kp=K_4F;   sp=S_4F;   hcp=HC_4F;   hop=HO_4F;   break;
    }

    // Build the four main tracks
    pattern[KICK_T]  = build_pat(kp);
    pattern[SNARE_T] = build_pat(sp);
    pattern[HATC_T]  = build_pat(hcp);
    pattern[HATO_T]  = build_pat(hop);
    pattern[CLAP_T]  = build_pat(sp);   // clap loosely follows snare

    // Ultra-sparse scatter on tracks 5-15 (~0.6 hits per bar average)
    static const uint8_t SPARSE[16] = {4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4};
    for (int t = 5; t < 16; t++)
        pattern[t] = build_pat(SPARSE);

    // ---- Musical guarantees --------------------------------
    // Kick always fires on beat 1 (step 0)
    pattern[KICK_T] |= (1 << 0);

    // Snare always fires on beats 2 and 4 (steps 4 and 12)
    pattern[SNARE_T] |= (1 << 4) | (1 << 12);

    // Four-on-floor: kick on all four downbeats
    if (style == 4)
        pattern[KICK_T] |= (1<<0)|(1<<4)|(1<<8)|(1<<12);

    // Open hat never fires on same step as closed hat
    pattern[HATO_T] &= ~pattern[HATC_T];

    // ---- Refresh display -----------------------------------
    if (viz_mode == 0) { draw8aBar(); draw8bBar(); }
    drawBT(30, ZMAGENTA, STYLE_NAME[style]);   // briefly show style
    delay(200);
    drawBT(30, DARKGREY, " RAND B ");
}

void rand_init() {
    drawBT(30, DARKGREY, " RAND B ");
}