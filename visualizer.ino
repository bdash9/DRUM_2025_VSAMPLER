// ============================================================
//  visualizer.ino  —  Music Visualizer  v1.0
//  Button 29 cycles through 6 states
//    0 = OFF     (normal bars shown)
//    1 = Ambient starfield    (demo — runs even when silent)
//    2 = EQ bars              (16 voice channels, fast attack / slow decay)
//    3 = Oscilloscope         (trigger-synced mixed waveform)
//    4 = Beat particles       (colour sparks fire on every trigger)
//    5 = Plasma colour wave   (audio-driven speed)
//
//  Canvas: bars row   x=0   y=100   w=480   h=80
// ============================================================

// ---- Canvas ------------------------------------------------
#define VIZ_X   0
#define VIZ_Y   (BheightPad * 2)   // 100
#define VIZ_W   480
#define VIZ_H   BheightBar          // 80

// ---- State -------------------------------------------------
int  viz_mode    = 0;
bool viz_cleared = false;

// ---- Shared audio data (Core 0 writes / Core 1 reads) ------
volatile int16_t  viz_mix_amp = 0;   // peak-hold absolute level

// Scope ring buffer — 8:1 downsampled → ~87 ms window
// Double-wide so trigger-sync always has room for VIZ_W samples
#define VIZ_SCOPE_LEN  (VIZ_W * 2)   // 960
volatile int16_t  viz_scope_buf[VIZ_SCOPE_LEN];
volatile uint16_t viz_scope_wr = 0;

// ============================================================
//  viz_push_sample()
//  Called once per output sample inside write_buffer() (Core 0)
//  Updates the peak envelope and scope ring buffer
// ============================================================
void IRAM_ATTR viz_push_sample(int16_t L, int16_t R) {
    int16_t mix = (int16_t)(((int32_t)L + R) >> 1);

    // Peak envelope: instant attack, ~170 ms half-life decay at 44100 Hz
    int16_t a = (mix < 0) ? -mix : mix;
    if (a > viz_mix_amp)  viz_mix_amp = a;
    else                  viz_mix_amp = (int16_t)((int32_t)viz_mix_amp * 32765 >> 15);

    // Scope — write every 8th sample to stretch the window to ~87 ms
    static uint8_t skip = 0;
    if (++skip >= 8) {
        skip = 0;
        viz_scope_buf[viz_scope_wr] = mix;
        viz_scope_wr = (viz_scope_wr + 1) % VIZ_SCOPE_LEN;
    }
}

// ============================================================
//  hue565()  —  8-bit hue wheel → RGB565
// ============================================================
static uint16_t hue565(uint8_t h) {
    uint8_t reg = h / 43, rem = (uint8_t)((h % 43) * 6);
    uint8_t r, g, b;
    switch (reg) {
        case 0: r=255;     g=rem;      b=0;        break;
        case 1: r=255-rem; g=255;      b=0;        break;
        case 2: r=0;       g=255;      b=rem;      break;
        case 3: r=0;       g=255-rem;  b=255;      break;
        case 4: r=rem;     g=0;        b=255;      break;
        default:r=255;     g=0;        b=255-rem;  break;
    }
    return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (b >> 3);
}

// ============================================================
//  viz_toggle()  —  called by button 29 handler
// ============================================================
void viz_toggle() {
    viz_mode    = (viz_mode + 1) % 7;   // ← was % 6, now 7 modes
    viz_cleared = false;

    if (viz_mode == 0) {
        gfx->fillRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, BLACK);
        draw8aBar();
        draw8bBar();
        drawBT(29, DARKGREY, "  VIS  ");
    } else {
        static const char* lbl[] = {
            "", " VIZ 1  ", "   EQ   ",
            " SCOPE  ", "  BEAT  ", " PLASMA ",
            " METRO  "              // ← new Mode 6
        };
        drawBT(29, ZMAGENTA, lbl[viz_mode]);
    }
}

void viz_init() {
    drawBT(29, DARKGREY, "  VISUAL  ");
}

// ============================================================
//  viz_draw()  —  call once per loop() iteration
// ============================================================
void viz_draw() {
    if (viz_mode == 0) return;

    if (!viz_cleared) {
        viz_cleared = true;
        gfx->fillRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, BLACK);
    }

switch (viz_mode) {
        case 1: viz_ambient();    break;
        case 2: viz_eq();         break;
        case 3: viz_scope();      break;
        case 4: viz_particles();  break;
        case 5: viz_plasma();     break;
        case 6: viz_metronome();  break; 
    }
}

// ============================================================
//  MODE 1  —  Ambient starfield
//  Stars drift at a slow idle speed.
//  When audio plays, boost multiplier speeds them up.
// ============================================================
#define VIZ_STAR_CNT 55

struct VizStar { int16_t x, y; int8_t vx, vy; uint8_t hue; };
static VizStar viz_star[VIZ_STAR_CNT];
static bool    viz_star_ready = false;

void viz_ambient() {
    static uint32_t prev = 0;
    uint32_t now = millis();
    if (now - prev < 35) return;   // ~28 fps
    prev = now;

    if (!viz_star_ready) {
        viz_star_ready = true;
        for (int i = 0; i < VIZ_STAR_CNT; i++) {
            viz_star[i].x   = (int16_t)random(VIZ_X, VIZ_X + VIZ_W);
            viz_star[i].y   = (int16_t)random(VIZ_Y, VIZ_Y + VIZ_H);
            viz_star[i].vx  = (int8_t)(random(0, 2) ? 1 : -1);
            viz_star[i].vy  = (int8_t)(random(0, 3) - 1);
            viz_star[i].hue = (uint8_t)random(0, 256);
        }
    }

    // Audio makes stars move faster
    int boost = 1 + (int)map(constrain((int)viz_mix_amp, 0, 28000), 0, 28000, 0, 3);

    for (int i = 0; i < VIZ_STAR_CNT; i++) {
        // Erase old 3×2 dot
        gfx->fillRect(viz_star[i].x - 1, viz_star[i].y, 3, 2, BLACK);

        // Move + slowly cycle colour
        viz_star[i].x  += viz_star[i].vx * boost;
        viz_star[i].y  += viz_star[i].vy;
        viz_star[i].hue = (uint8_t)(viz_star[i].hue + 3);

        // Wrap within canvas
        if (viz_star[i].x >= VIZ_X + VIZ_W) viz_star[i].x = VIZ_X;
        if (viz_star[i].x <  VIZ_X)          viz_star[i].x = VIZ_X + VIZ_W - 1;
        if (viz_star[i].y >= VIZ_Y + VIZ_H)  viz_star[i].y = VIZ_Y;
        if (viz_star[i].y <  VIZ_Y)           viz_star[i].y = VIZ_Y + VIZ_H - 1;

        // Draw new position
        gfx->fillRect(viz_star[i].x - 1, viz_star[i].y, 3, 2,
                      hue565(viz_star[i].hue));
    }
}

// ============================================================
//  MODE 2  —  EQ bars  (one per voice, instant-on / timed decay)
// ============================================================
static uint32_t viz_eq_last_hit[16]   = {0};
static byte     viz_eq_prev_latch[16] = {0};

void viz_eq() {
    static uint32_t prev = 0;
    uint32_t now = millis();
    if (now - prev < 40) return;   // 25 fps
    prev = now;

    const int BAR_W  = VIZ_W / 16;   // 30 px per voice
    const int AREA_H = VIZ_H - 8;    // usable height inside canvas
    const int DECAY  = 550;           // ms from peak to zero

    for (int f = 0; f < 16; f++) {

        // Detect rising edge of latch (new trigger = reset decay)
        if (latch[f] && !viz_eq_prev_latch[f]) {
            viz_eq_last_hit[f] = now;
        }
        viz_eq_prev_latch[f] = latch[f];

        int age   = (int)(now - viz_eq_last_hit[f]);
        int bar_h = max(0, AREA_H - (age * AREA_H / DECAY));

        int bx  = VIZ_X + f * BAR_W + 1;
        int bot = VIZ_Y + VIZ_H - 4;

        // Colour gradient: green (quiet) → yellow → red (loud)
        int pct = (AREA_H > 0) ? bar_h * 100 / AREA_H : 0;
        uint16_t col = (pct < 50)
            ? hue565((uint8_t)map(pct,  0, 50, 85, 42))  // green→yellow
            : hue565((uint8_t)map(pct, 50,100, 42,  0)); // yellow→red

        // Erase column then draw bar
        gfx->fillRect(bx, VIZ_Y + 3, BAR_W - 2, AREA_H, BLACK);
        if (bar_h > 0)
            gfx->fillRect(bx, bot - bar_h, BAR_W - 2, bar_h, col);

        // White peak cap when bar is at maximum
        if (bar_h >= AREA_H - 2)
            gfx->fillRect(bx, VIZ_Y + 3, BAR_W - 2, 2, 0xFFFF);
    }

    gfx->drawRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, DARKGREY);
}

// ============================================================
//  MODE 3  —  Oscilloscope  (trigger-synced rainbow waveform)
// ============================================================
void viz_scope() {
    static uint32_t prev    = 0;
    static uint8_t  hue_off = 0;
    uint32_t now = millis();
    if (now - prev < 33) return;
    prev = now;
    hue_off += 2;

    // Snapshot ring buffer
    int16_t snap[VIZ_SCOPE_LEN];
    uint16_t rd = viz_scope_wr;
    for (int i = 0; i < VIZ_SCOPE_LEN; i++)
        snap[i] = viz_scope_buf[(rd + i) % VIZ_SCOPE_LEN];

    // Find rising zero-crossing to sync display
    int sync_i = 0;
    for (int i = 1; i < VIZ_SCOPE_LEN / 2; i++) {
        if (snap[i - 1] <= 0 && snap[i] > 0) { sync_i = i; break; }
    }
    if (sync_i + VIZ_W >= VIZ_SCOPE_LEN) sync_i = 0;

    // AUTO-SCALE — find actual peak in the visible window
    // Use a minimum floor so a flat line shows when silent
    // rather than over-amplifying noise
    int16_t snap_peak = 400;   // minimum floor
    for (int i = sync_i; i < sync_i + VIZ_W; i++) {
        int16_t a = snap[i] < 0 ? -snap[i] : snap[i];
        if (a > snap_peak) snap_peak = a;
    }
    // Add 15% headroom so waveform never clips the top/bottom edge
    int scale = (int)snap_peak + ((int)snap_peak * 15 / 100);

    gfx->fillRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, BLACK);
    // Faint centre line
    gfx->drawFastHLine(VIZ_X, VIZ_Y + VIZ_H / 2, VIZ_W, 0x1082);

    for (int x = 0; x < VIZ_W - 1; x++) {
        int16_t s1 = snap[sync_i + x];
        int16_t s2 = snap[sync_i + x + 1];

        int y1 = constrain(map((int)s1, -scale, scale,
                               VIZ_Y + VIZ_H - 2, VIZ_Y + 2),
                           VIZ_Y + 1, VIZ_Y + VIZ_H - 2);
        int y2 = constrain(map((int)s2, -scale, scale,
                               VIZ_Y + VIZ_H - 2, VIZ_Y + 2),
                           VIZ_Y + 1, VIZ_Y + VIZ_H - 2);

        uint16_t col = hue565((uint8_t)(map(x, 0, VIZ_W, 0, 255) + hue_off));
        gfx->drawLine(VIZ_X + x, y1, VIZ_X + x + 1, y2, col);
    }

    gfx->drawRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, DARKGREY);
}

// ============================================================
//  MODE 4  —  Beat particles  (coloured sparks per voice)
// ============================================================
#define VIZ_PART_CNT 80

struct VizPart { int16_t x, y, vx, vy; uint8_t ttl; uint16_t col; };
static VizPart viz_part[VIZ_PART_CNT];
static bool    viz_part_ready    = false;
static byte    viz_part_prev[16] = {0};

// One distinct colour per voice
static const uint16_t VIZ_VCOL[16] = {
    0xF800, 0xFD20, 0xFFE0, 0x9FE0,
    0x07E0, 0x07FF, 0x001F, 0xF81F,
    0xFC00, 0x87E0, 0x3FE0, 0xC0FF,
    0xFFF0, 0xFF9F, 0xE81F, 0xFFFF
};

void viz_particles() {
    static uint32_t prev = 0;
    uint32_t now = millis();
    if (now - prev < 30) return;   // ~33 fps
    prev = now;

    if (!viz_part_ready) {
        viz_part_ready = true;
        memset(viz_part, 0, sizeof(viz_part));
    }

    // Spawn 8 sparks when a voice fires
    for (int f = 0; f < 16; f++) {
        if (latch[f] && !viz_part_prev[f]) {
            int spawned = 0;
            int cx = VIZ_X + (f + 1) * VIZ_W / 17;   // x spread by voice index
            int cy = VIZ_Y + VIZ_H / 2;
            for (int p = 0; p < VIZ_PART_CNT && spawned < 8; p++) {
                if (viz_part[p].ttl == 0) {
                    viz_part[p] = {
                        (int16_t)cx, (int16_t)cy,
                        (int16_t)random(-7,  8),
                        (int16_t)random(-9,  2),   // mostly upward
                        (uint8_t)random(20, 46),
                        VIZ_VCOL[f]
                    };
                    spawned++;
                }
            }
        }
        viz_part_prev[f] = latch[f];
    }

    gfx->fillRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, BLACK);

    for (int p = 0; p < VIZ_PART_CNT; p++) {
        if (!viz_part[p].ttl) continue;
        viz_part[p].ttl--;
        viz_part[p].x  += viz_part[p].vx;
        viz_part[p].y  += viz_part[p].vy;
        viz_part[p].vy += 1;   // gravity

        if (viz_part[p].x < VIZ_X || viz_part[p].x >= VIZ_X + VIZ_W ||
            viz_part[p].y < VIZ_Y || viz_part[p].y >= VIZ_Y + VIZ_H) {
            viz_part[p].ttl = 0;
            continue;
        }

        // Fade colour as particle ages
        uint8_t  fade = (uint8_t)min(255, viz_part[p].ttl * 5);
        uint16_t c    = viz_part[p].col;
        uint8_t  rr   = (uint8_t)(((c >> 11) & 0x1F) * fade >> 8);
        uint8_t  gg   = (uint8_t)(((c >>  5) & 0x3F) * fade >> 8);
        uint8_t  bb   = (uint8_t)(( c         & 0x1F) * fade >> 8);
        gfx->fillRect(viz_part[p].x, viz_part[p].y, 3, 3,
                      ((uint16_t)rr << 11) | ((uint16_t)gg << 5) | bb);
    }

    gfx->drawRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, DARKGREY);
}

// ============================================================
//  MODE 5  —  Plasma colour wave  (audio speeds up animation)
// ============================================================
void viz_plasma() {
    static uint32_t prev  = 0;
    static float    phase = 0.0f;
    uint32_t now = millis();
    if (now - prev < 55) return;   // ~18 fps
    prev = now;

    // Louder audio = faster plasma movement
    float speed = 0.10f + (float)viz_mix_amp / 32767.0f * 0.28f;
    phase += speed;

    // 4 px blocks → 120×20 = 2400 fillRect calls per frame
    // Fast on QSPI; adjust BSIZ upward if you need more CPU headroom
    const int BSIZ = 4;

    for (int y = VIZ_Y; y < VIZ_Y + VIZ_H; y += BSIZ) {
        float fy = (float)(y - VIZ_Y) / (float)VIZ_H;
        for (int x = VIZ_X; x < VIZ_X + VIZ_W; x += BSIZ) {
            float fx = (float)(x - VIZ_X) / (float)VIZ_W;
            float v  = sinf(fx * 7.0f  + phase)
                     + sinf(fy * 5.0f  + phase * 0.8f)
                     + sinf((fx + fy)   * 4.5f + phase * 1.3f);
            // v in [-3, 3] → hue 0..255
            uint8_t hue = (uint8_t)((v + 3.0f) * 42.5f);
            gfx->fillRect(x, y, BSIZ, BSIZ,
                          hue565((uint8_t)(hue + (uint8_t)(phase * 18.0f))));
        }
    }
}
// ============================================================
//  MODE 6  —  Metronome beat light
//  4 circles pulse in sequence at the current BPM
//  Works whether the sequencer is playing or stopped
// ============================================================
void viz_metronome() {
    static uint32_t last_beat  = 0;
    static int      last_bpm   = -1;
    static bool     last_lit   = false;
    static uint8_t  beat_hue   = 0;

    uint32_t now     = millis();
    uint32_t beat_ms = 60000UL / (uint32_t)constrain((int)bpm, 20, 300);

    // Detect new beat tick
    bool new_beat = (now - last_beat >= beat_ms);
    if (new_beat) {
        last_beat = now;
        beat_hue += 60;   // shift colour each beat
    }

    // Circle is lit for first 100ms of each beat
    bool circle_lit = ((now - last_beat) < 100);

    // Only redraw when lit state or BPM changes — saves CPU
    int bpm_now = (int)bpm;
    if (circle_lit == last_lit && bpm_now == last_bpm) return;

    last_lit = circle_lit;
    last_bpm = bpm_now;

    // ---- BPM text on the left --------------------------------
    char buf[8];
    sprintf(buf, "%3d", bpm_now);
    gfx->fillRect(VIZ_X, VIZ_Y, 120, VIZ_H, BLACK);   // clear left side

    gfx->setTextSize(3);
    gfx->setTextColor(0xFFFF, BLACK);
    gfx->setCursor(VIZ_X + 8, VIZ_Y + VIZ_H / 2 - 16);
    gfx->print(buf);

    gfx->setTextSize(1);
    gfx->setTextColor(0x7BEF, BLACK);
    gfx->setCursor(VIZ_X + 24, VIZ_Y + VIZ_H / 2 + 14);
    gfx->print("BPM");

    // ---- Single beat circle on the right --------------------
    int cx = VIZ_X + VIZ_W * 3 / 4;
    int cy = VIZ_Y + VIZ_H / 2;
    int r  = 28;

    if (circle_lit) {
        gfx->fillCircle(cx, cy, r, hue565(beat_hue));
        gfx->fillCircle(cx, cy, r / 2, 0xFFFF);   // white centre
    } else {
        gfx->fillCircle(cx, cy, r,     0x2104);   // dark fill
        gfx->drawCircle(cx, cy, r,     0x528A);   // dim ring
    }

    gfx->drawRect(VIZ_X, VIZ_Y, VIZ_W, VIZ_H, DARKGREY);
}

