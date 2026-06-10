////////////////////////////
// DRUM SAMPLER 2025      //
////////////////////////////
// ZCARLOS 2025 / Modified by Ben

#include <Arduino.h>

//#define mylcd_type1
#define mylcd_type2
//#define ads_ok

int32_t muestra;
#define SAMPLE_RATE 44100

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#define SD_CS   10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCLK 12
#define MAX_SOUNDS 128
SPIClass sdSPI(HSPI);

#include <Adafruit_ADS1X15.h>
#ifdef mylcd_type1
  #define SDA_2 15
  #define SCL_2 16
#endif
#ifdef mylcd_type2
  #define SDA_2 17
  #define SCL_2 18
#endif

TwoWire I2C_2 = TwoWire(1);
Adafruit_ADS1115 ads;
int16_t adc0=0, adc1=0, adc2=0, adc3=0;
int16_t tmp_adc1;
int16_t old_adc0=0, old_adc1=0, old_adc2=0, old_adc3=0;
unsigned long previousMillis = 0;
const long interval = 100;
int16_t adcValue0=0, adcValue1=0, adcValue2=0, adcValue3=0;

#include <FS.h>
#include <SPIFFS.h>

#include "driver/i2s.h"
#include "synthESP32LowPassFilter_E.h"
#define MAX_16 32767
#define MIN_16 -32767

// ── Texts ────────────────────────────────────────────────────
const String trot[16] = {
    "SAM","INI","END","PIT","RVS","VOL","PAN","FIL",
    "BPM","MVO","TRP","MFI","OCT","MPI","SYN","SCA"
};
const String tbuttons1[8] = {
    "   PAD   "," RND P  "," LOAD PS "," SAVE PS ",
    "  MUTE  ","  PIANO ","  PLAY  ","  SONG  "
};
const String tbuttons2[8] = {
    "  SHIFT  ","  - 1   ","  - 10   ","  + 10   ",
    "  + 1   "," VISUAL ","RAND BT ","  SHIFT "
};

// ── LCD ──────────────────────────────────────────────────────
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE ESP32_4827A043_QSPI

#ifndef BLACK
  #ifdef ZBLACK
    #define BLACK ZBLACK
  #else
    #define BLACK 0x0000
  #endif
#endif
#ifndef BLUE
  #ifdef ZBLUE
    #define BLUE ZBLUE
  #else
    #define BLUE 0x001F
  #endif
#endif
#ifndef GREEN
  #ifdef ZGREEN
    #define GREEN ZGREEN
  #else
    #define GREEN 0x07E0
  #endif
#endif
#ifndef MAGENTA
  #ifdef ZMAGENTA
    #define MAGENTA ZMAGENTA
  #else
    #define MAGENTA 0xF81F
  #endif
#endif
#ifndef YELLOW
  #ifdef ZYELLOW
    #define YELLOW ZYELLOW
  #else
    #define YELLOW 0xFFE0
  #endif
#endif
#ifndef DARKGREY
  #define DARKGREY 0x7BEF
#endif
#ifndef LIGHTGREY
  #define LIGHTGREY 0xC618
#endif
#ifndef DARKGREEN
  #define DARKGREEN 0x03E0
#endif
#ifndef GREENYELLOW
  #define GREENYELLOW 0xAFE5
#endif
#ifndef ORANGE
  #define ORANGE 0xFD20
#endif

#define GFX_BL 1
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45 /* cs */, 47 /* sck */, 21 /* d0 */,
    48 /* d1 */, 40 /* d2 */,  39 /* d3 */);
Arduino_GFX *gfx = new Arduino_NV3041A(
    bus, GFX_NOT_DEFINED, 0, true);
#ifdef ESP32
  #undef F
  #define F(s) (s)
#endif

// ── Touch ────────────────────────────────────────────────────
#define GT911_ADDR 0x5D
#define TOUCH_SDA  8
#define TOUCH_SCL  4
#define TOUCH_INT  3
#define TOUCH_RST  38

// ── Button geometry ──────────────────────────────────────────
#define Bleft      0
#define Btop       0
#define BwidthPad  60
#define BheightPad 50
#define BwidthBar  24
#define BheightBar 80
#define BwidthFunc  60
#define BheightFunc 50
#define BwidthNorot  60
#define BheightNorot 42
#define rot0_width   144
#define rot0_height  BheightBar
#define rot1_width   BwidthBar
#define rot1_height  BheightBar/2
#define rot2_width   BwidthBar
#define rot2_height  BheightBar/2
#define WAVE_WIDTH   128
#define WAVE_HEIGHT   64
#define WAVE_ORIGIN_X   6
#define WAVE_ORIGIN_Y   (BheightPad*2)+2

// ── Forward declarations for other .ino files ─────────────────
void initSeqTimer();
void startSeqTimer();
void stopSeqTimer();
void updateSeqTempo();
extern int   viz_mode;
void viz_push_sample(int16_t L, int16_t R);
void viz_toggle();
void viz_draw();
void viz_init();
void rand_pattern();
void rand_init();
const char* rand_get_label();
extern volatile byte pendingTrigger[16];
extern volatile byte pendingPitch[16]; 

// ── Button state ─────────────────────────────────────────────
int  BPOS[48][4];
int  BCOLOR[48];

const int debounce_time = 230;
long  start_debounce;
bool  touchActivo    = false;
int   last_touched   = -1;
bool  show_last_touched = false;
long  start_showlt;
bool  flag_ss   = false;
int   count_loop = 0;
int   cox, coy, coz;

// ── modeZ defines ────────────────────────────────────────────
#define tPad    0
#define tSel    1
#define tWrite  2
#define tMute   3
#define tSolo   4
#define tClear  5
#define tLoaPS  6
#define tLoadP  7
#define tLoadS  8
#define tSavPS  9
#define tSaveP 10
#define tSaveS 11
#define tRndS  12
#define tRndP  13
#define tFirst 14
#define tLast  15
#define tMelod 16
#define tRndNo 17
#define tPiano 18
#define tSong  19
#define tMemo  20
#define tMfirs 21
#define tMlast 22
#define tRndS2 23

// ── Sample engine state ──────────────────────────────────────
uint64_t NEWENDS[50];
uint64_t NEWINIS[50];
byte     latch[16];
uint64_t samplePos[16];
uint64_t stepSize[16];

// ── Sound headers ────────────────────────────────────────────
#include "SOUNDS/zSAMPLE00.h"
#include "SOUNDS/zSAMPLE01.h"
#include "SOUNDS/zSAMPLE02.h"
#include "SOUNDS/zSAMPLE03.h"
#include "SOUNDS/zSAMPLE04.h"
#include "SOUNDS/zSAMPLE05.h"
#include "SOUNDS/zSAMPLE06.h"
#include "SOUNDS/zSAMPLE07.h"
#include "SOUNDS/zSAMPLE08.h"
#include "SOUNDS/zSAMPLE09.h"
#include "SOUNDS/zSAMPLE10.h"
#include "SOUNDS/zSAMPLE11.h"
#include "SOUNDS/zSAMPLE12.h"
#include "SOUNDS/zSAMPLE13.h"
#include "SOUNDS/zSAMPLE14.h"
#include "SOUNDS/zSAMPLE15.h"
#include "SOUNDS/zSAMPLE16.h"
#include "SOUNDS/zSAMPLE17.h"
#include "SOUNDS/zSAMPLE18.h"
#include "SOUNDS/zSAMPLE19.h"
#include "SOUNDS/zSAMPLE20.h"
#include "SOUNDS/zSAMPLE21.h"
#include "SOUNDS/zSAMPLE22.h"
#include "SOUNDS/zSAMPLE23.h"
#include "SOUNDS/zSAMPLE24.h"
#include "SOUNDS/zSAMPLE25.h"
#include "SOUNDS/zSAMPLE26.h"
#include "SOUNDS/zSAMPLE27.h"
#include "SOUNDS/zSAMPLE28.h"
#include "SOUNDS/zSAMPLE29.h"
#include "SOUNDS/zSAMPLE30.h"
#include "SOUNDS/zSAMPLE31.h"
#include "SOUNDS/SYNTH1.h"
#include "SOUNDS/zSAMPLE33.h"
#include "SOUNDS/zSAMPLE34.h"
#include "SOUNDS/zSAMPLE35.h"
#include "SOUNDS/zSAMPLE36.h"
#include "SOUNDS/zSAMPLE37.h"
#include "SOUNDS/zSAMPLE38.h"
#include "SOUNDS/zSAMPLE39.h"
#include "SOUNDS/SYNTH4.h"
#include "SOUNDS/zSAMPLE41.h"
#include "SOUNDS/zSAMPLE42.h"
#include "SOUNDS/SYNTH6.h"
#include "SOUNDS/zSAMPLE44.h"
#include "SOUNDS/zSAMPLE45.h"
#include "SOUNDS/zSAMPLE46.h"
#include "SOUNDS/zSAMPLE47.h"
#include "SOUNDS/zSAMPLE48.h"
#include "SOUNDS/zSAMPLE49.h"

const String sound_names[50] = {
    "0","1","2","3","4","5","6","7","8","9",
    "10","11","12","13","14","15","16","17","18","19",
    "20","21","22","23","24","25","26","27","28","29",
    "30","31","SYNTH 1","33","34","35","36","37","38","39",
    "SYNTH 4","SAMPLE41","42","SYNTH 6",
    "44","45","46","47","48","49"
};

const uint64_t ENDS[50] = {
    (sizeof(SAMPLE00)/sizeof(SAMPLE00[0]))-1,
    (sizeof(SAMPLE01)/sizeof(SAMPLE01[0]))-1,
    (sizeof(SAMPLE02)/sizeof(SAMPLE02[0]))-1,
    (sizeof(SAMPLE03)/sizeof(SAMPLE03[0]))-1,
    (sizeof(SAMPLE04)/sizeof(SAMPLE04[0]))-1,
    (sizeof(SAMPLE05)/sizeof(SAMPLE05[0]))-1,
    (sizeof(SAMPLE06)/sizeof(SAMPLE06[0]))-1,
    (sizeof(SAMPLE07)/sizeof(SAMPLE07[0]))-1,
    (sizeof(SAMPLE08)/sizeof(SAMPLE08[0]))-1,
    (sizeof(SAMPLE09)/sizeof(SAMPLE09[0]))-1,
    (sizeof(SAMPLE10)/sizeof(SAMPLE10[0]))-1,
    (sizeof(SAMPLE11)/sizeof(SAMPLE11[0]))-1,
    (sizeof(SAMPLE12)/sizeof(SAMPLE12[0]))-1,
    (sizeof(SAMPLE13)/sizeof(SAMPLE13[0]))-1,
    (sizeof(SAMPLE14)/sizeof(SAMPLE14[0]))-1,
    (sizeof(SAMPLE15)/sizeof(SAMPLE15[0]))-1,
    (sizeof(SAMPLE16)/sizeof(SAMPLE16[0]))-1,
    (sizeof(SAMPLE17)/sizeof(SAMPLE17[0]))-1,
    (sizeof(SAMPLE18)/sizeof(SAMPLE18[0]))-1,
    (sizeof(SAMPLE19)/sizeof(SAMPLE19[0]))-1,
    (sizeof(SAMPLE20)/sizeof(SAMPLE20[0]))-1,
    (sizeof(SAMPLE21)/sizeof(SAMPLE21[0]))-1,
    (sizeof(SAMPLE22)/sizeof(SAMPLE22[0]))-1,
    (sizeof(SAMPLE23)/sizeof(SAMPLE23[0]))-1,
    (sizeof(SAMPLE24)/sizeof(SAMPLE24[0]))-1,
    (sizeof(SAMPLE25)/sizeof(SAMPLE25[0]))-1,
    (sizeof(SAMPLE26)/sizeof(SAMPLE26[0]))-1,
    (sizeof(SAMPLE27)/sizeof(SAMPLE27[0]))-1,
    (sizeof(SAMPLE28)/sizeof(SAMPLE28[0]))-1,
    (sizeof(SAMPLE29)/sizeof(SAMPLE29[0]))-1,
    (sizeof(SAMPLE30)/sizeof(SAMPLE30[0]))-1,
    (sizeof(SAMPLE31)/sizeof(SAMPLE31[0]))-1,
    (sizeof(SYNTH1)  /sizeof(SYNTH1[0])  )-1,
    (sizeof(SAMPLE33)/sizeof(SAMPLE33[0]))-1,
    (sizeof(SAMPLE34)/sizeof(SAMPLE34[0]))-1,
    (sizeof(SAMPLE35)/sizeof(SAMPLE35[0]))-1,
    (sizeof(SAMPLE36)/sizeof(SAMPLE36[0]))-1,
    (sizeof(SAMPLE37)/sizeof(SAMPLE37[0]))-1,
    (sizeof(SAMPLE38)/sizeof(SAMPLE38[0]))-1,
    (sizeof(SAMPLE39)/sizeof(SAMPLE39[0]))-1,
    (sizeof(SYNTH4)  /sizeof(SYNTH4[0])  )-1,
    (sizeof(SAMPLE41)/sizeof(SAMPLE41[0]))-1,
    (sizeof(SAMPLE42)/sizeof(SAMPLE42[0]))-1,
    (sizeof(SYNTH6)  /sizeof(SYNTH6[0])  )-1,
    (sizeof(SAMPLE44)/sizeof(SAMPLE44[0]))-1,
    (sizeof(SAMPLE45)/sizeof(SAMPLE45[0]))-1,
    (sizeof(SAMPLE46)/sizeof(SAMPLE46[0]))-1,
    (sizeof(SAMPLE47)/sizeof(SAMPLE47[0]))-1,
    (sizeof(SAMPLE48)/sizeof(SAMPLE48[0]))-1,
    (sizeof(SAMPLE49)/sizeof(SAMPLE49[0]))-1
};

const int16_t* SAMPLES[50] = {
    SAMPLE00,SAMPLE01,SAMPLE02,SAMPLE03,SAMPLE04,
    SAMPLE05,SAMPLE06,SAMPLE07,SAMPLE08,SAMPLE09,
    SAMPLE10,SAMPLE11,SAMPLE12,SAMPLE13,SAMPLE14,
    SAMPLE15,SAMPLE16,SAMPLE17,SAMPLE18,SAMPLE19,
    SAMPLE20,SAMPLE21,SAMPLE22,SAMPLE23,SAMPLE24,
    SAMPLE25,SAMPLE26,SAMPLE27,SAMPLE28,SAMPLE29,
    SAMPLE30,SAMPLE31,SYNTH1,
    SAMPLE33,SAMPLE34,SAMPLE35,SAMPLE36,SAMPLE37,
    SAMPLE38,SAMPLE39,SYNTH4,
    SAMPLE41,SAMPLE42,SYNTH6,
    SAMPLE44,SAMPLE45,SAMPLE46,SAMPLE47,SAMPLE48,SAMPLE49
};

// ── I2S pins ─────────────────────────────────────────────────
#ifdef mylcd_type1
  #define I2S_BCK_PIN      41
  #define I2S_WS_PIN        2
  #define I2S_DATA_OUT_PIN 42
#endif
#ifdef mylcd_type2
  #define I2S_BCK_PIN       9
  #define I2S_WS_PIN        5
  #define I2S_DATA_OUT_PIN 14
#endif
#define DMA_BUF_LEN  64
#define DMA_NUM_BUF   8
static uint16_t out_buf[DMA_BUF_LEN * 2];

// ── Filters ──────────────────────────────────────────────────
LowPassFilter FILTROS[18] = {
    LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
    LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
    LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
    LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
    LowPassFilter(),LowPassFilter()
};
const int cutoff = 255;
const int reso   = 511;

static int VOL_R[16] = {10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
static int VOL_L[16] = {10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
static int PAN[16]   = {10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
static int mvol      = 10;
int master_vol    = 80;
int master_filter = 0;
int octave        = 5;

// ── Sequencer globals ────────────────────────────────────────
int  bpm = 120;
byte selected_pattern    = 0;
byte s_old_selected_pattern = 99;
volatile byte sstep      = 0;
int  s_old_sstep         = -1;
uint16_t mutes  = 0;
uint16_t solos  = 0;
uint16_t pattern[16];
byte fx1 = 0;
byte selected_memory     = 0;
byte s_old_selected_memory = 99;
int  pattern_song_counter = -1;
byte last_pattern_song   = 255;
uint8_t  melodic[16][16];
uint16_t isMelodic = B00000000<<8 | B00000000;
volatile byte firstStep  = 0;
volatile byte lastStep   = 15;
volatile byte newLastStep= 15;
byte firstMemory = 0;
byte lastMemory  = 15;
byte newLastMemory = 15;
byte selected_sound    = 0;
byte oldselected_sound = 0;
byte s_old_selected_sound = 99;
byte selected_sndSet   = 0;
byte s_old_selected_sndSet = 99;
int  ztranspose = 0;
int  zmpitch    = 0;
volatile bool load_flag  = false;
byte sync_state = 0;
volatile bool sync_flag  = false;

// ── Scales ───────────────────────────────────────────────────
uint8_t selected_scale = 0;
const uint8_t escalas[13][8] = {
    {0,0,0,0,0,0,0,0},
    {0,2,4,5,7,9,11,255},
    {0,2,3,5,7,8,10,255},
    {0,2,3,5,7,8,11,255},
    {0,2,3,5,7,9,11,255},
    {0,2,4,7,9,255,255,255},
    {0,3,5,7,10,255,255,255},
    {0,3,5,6,7,10,255,255},
    {0,2,3,5,7,9,10,255},
    {0,1,3,5,7,8,10,255},
    {0,2,4,6,7,9,11,255},
    {0,2,4,5,7,9,10,255},
    {0,1,3,5,6,8,10,255}
};
const char* nombresEscalas[] = {
    "Free","Mayor","Menor Natural","Menor Armónica",
    "Menor Melódica","Pentatónica Mayor","Pentatónica Menor",
    "Blues Menor","Dórico","Frigio","Lidio","Mixolidio","Locrio"
};

// ── Rotary encoder ───────────────────────────────────────────
#ifdef mylcd_type1
  #define CLK 9
  #define DT  14
  const byte pinBR1 = 5;
#endif
#ifdef mylcd_type2
  #define CLK 7
  #define DT  6
  const byte pinBR1 = 15;
#endif

byte  ENC_PORT1   = 0;
int   counter1    = 0;
int   old_counter1= 0;
byte  shiftR1     = false;
byte  old_shiftR1 = true;

const int max_values[16] = {49,2047,2047,127,1,127, 127,127,400,127, 1,127,20, 1,2,12};
const int min_values[16] = { 0,  0,  0,  0, 0,  0,-127,  0,  0,  0,-1,  0, 0,-1,0, 0};

int32_t ROTvalue[16][8] = {
    { 16,0,2047,60,0,80,-10,0},
    { 17,0,2047,60,0,80,-31,0},
    { 18,0,2047,60,0,80, 12,0},
    { 19,0,2047,60,0,80,  0,0},
    { 20,0,2047,60,0,80, 20,0},
    { 21,0,2047,60,0,80,  0,0},
    { 22,0,2047,60,0,80, 30,0},
    { 23,0,2047,60,0,80,  0,0},
    { 24,0,2047,60,0,80, 66,0},
    { 25,0,2047,60,0,80, 66,0},
    { 26,0,2047,60,0,80,  0,0},
    { 27,0,2047,60,0,80,-20,0},
    { 28,0,2047,60,0,80,  0,0},
    { 29,0,2047,60,0,80,-10,0},
    { 30,0,2047,60,0,80, 30,0},
    { 31,0,2047,60,0,80,-30,0}
};

byte selected_rot   = 0;
byte s_old_selected_rot = 1;
byte trigger_on[48];
byte nkey;

// ── Mode / play state ────────────────────────────────────────
byte modeZ       = 0;
int  s_old_modeZ = -1;

volatile bool playing   = false;
bool pre_playing  = false;
bool songing      = false;
bool recording    = false;
bool shifting     = false;

volatile bool clearPADSTEP    = false;
bool clearPATTERNPADS = false;
bool refreshPATTERN   = true;
volatile bool refreshPADSTEP  = false;
bool refreshMODES     = true;

#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC

// ════════════════════════════════════════════════════════════
//  S E T U P
// ════════════════════════════════════════════════════════════
void setup() {

    Serial.begin(115200);
    delay(500);
    randomSeed(analogRead(0));
    rand_init();

    // Clear all patterns
    for (int f = 0; f < 16; f++) pattern[f] = 0;
    Serial.println("Patterns cleared");

    // I2C
    Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000);

    #ifdef ads_ok
    I2C_2.begin(SDA_2, SCL_2, 400000);
    if (!ads.begin(0x48, &I2C_2)) {
        Serial.println("ADS1115 not found.");
    } else {
        Serial.println("ADS1115 init OK.");
        ads.setGain(GAIN_ONE);
    }
    #endif

    // Display
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    gfx->fillScreen(BLACK);

    #ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    #endif

    gfx->setFont(u8g2_font_5x8_mr);
    gfx->setUTF8Print(true);

    fillBPOS();
    drawScreen1_ONLY1();

    // Touch
    resetGT911();
    delay(100);

    // Synth
    synthESP32_begin();
    for (byte f = 0; f < 16; f++) setSound(f);
    synthESP32_setMVol(master_vol);
    synthESP32_setMFilter(master_filter);

    // SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }

    // SD
    sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSPI)) {
        Serial.println("SD not found, continuing without it.");
    } else {
        Serial.println("SD OK.");
    }

    // Init melodic array
    for (byte a = 0; a < 16; a++)
        for (byte b = 0; b < 16; b++)
            melodic[a][b] = 60;

    // Set voices 8-15 as melodic
    isMelodic = B11111111<<8 | B00000000;

    // Start sequencer timer
    initSeqTimer();

    #ifdef ads_ok
    pinMode(pinBR1, INPUT_PULLUP);
    pinMode(CLK,    INPUT_PULLUP);
    pinMode(DT,     INPUT_PULLUP);
    #endif

    select_rot();
    draw8aBar();
    draw8bBar();
    viz_init();   // draw button 29 label once

}   // ← setup() closes here

// ════════════════════════════════════════════════════════════
//  L O O P
// ════════════════════════════════════════════════════════════
void loop() {

    // ── Process pending triggers from sequencer ───────────────
    for (int f = 0; f < 16; f++) {
        if (pendingTrigger[f]) {
            pendingTrigger[f] = 0;
            if (bitRead(isMelodic, f)) {
                synthESP32_TRIGGER_P(f, pendingPitch[f]);
            } else {
                synthESP32_TRIGGER(f);
            }
        }
    }

    // ── Debug output every 300 ms ─────────────────────────────
        static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 500) {
        lastDebug = millis();
        extern volatile uint32_t tic_counter;
        Serial.printf("tic=%u step=%d play=%d p0=0x%04X p2=0x%04X latch0=%d\n",
                      tic_counter, (int)sstep, (int)playing,
                      pattern[0], pattern[2], (int)latch[0]);
    }

    // ── Song mode: auto-advance pattern ───────────────────────
    if (load_flag) {
        load_flag = false;
        pattern_song_counter++;
        if (pattern_song_counter > lastMemory    ||
            pattern_song_counter > newLastMemory ||
            pattern_song_counter == 16) {
            lastMemory = newLastMemory;
            pattern_song_counter = firstMemory;
        }
        load_pattern(pattern_song_counter);
        selected_pattern = pattern_song_counter;
        load_sound(pattern_song_counter);
        selected_sndSet  = pattern_song_counter;
        select_rot();
        draw8aBar();
        refreshPADSTEP = true;
    }

    #ifdef ads_ok
    shiftR1 = !digitalRead(pinBR1);
    READ_ENCODERS();
    #endif

    read_touch();
    DO_KEYPAD();
    REFRESH_KEYS();
    REFRESH_STATUS();
    showLastTouched();
    clearLastTouched();
    viz_draw();
    rand_init();   // one-shot seed guard — returns instantly after first call

    #ifdef ads_ok
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        READ_POTS();
    }
    #endif

    if (flag_ss == true) {
        count_loop++;
        if (count_loop == 2) {
            count_loop = 0;
            flag_ss    = false;
            for (byte f = 0; f < 16; f++) setSound(f);
        }
    }

}   // ← loop() closes here  ← THIS BRACE WAS MISSING 