// sequencer.ino
#include "esp_timer.h"

volatile uint32_t tic_counter        = 0;
volatile uint32_t seq_tick           = 0;
volatile byte     pendingTrigger[16] = {0};
volatile byte     pendingPitch[16]   = {0};

static esp_timer_handle_t seq_timer_handle = NULL;

void IRAM_ATTR tic();

// Timer callback — fires at PPQN_24 rate (24x per beat)
static void seq_timer_cb(void* arg) {
    if (!playing) return;

    // FX1 repeat modes
    if (!(seq_tick % 12) && fx1==1) pendingTrigger[selected_sound] = 1;
    if (!(seq_tick % 6)  && fx1==2) pendingTrigger[selected_sound] = 1;
    if (!(seq_tick % 3)  && fx1==3) pendingTrigger[selected_sound] = 1;

    // Advance step every 6 ticks = 16th note
    if (!(seq_tick % 6)) tic();

    // Clear step highlight
    if ((seq_tick % 6) == 4) clearPADSTEP = true;

    seq_tick++;
    if (seq_tick >= 24) seq_tick = 0;
}

void initSeqTimer() {
    esp_timer_create_args_t args = {};
    args.callback               = seq_timer_cb;
    args.arg                    = NULL;
    args.dispatch_method        = ESP_TIMER_TASK;
    args.name                   = "seq";
    args.skip_unhandled_events  = true;
    esp_err_t err = esp_timer_create(&args, &seq_timer_handle);
    Serial.printf("initSeqTimer: %s\n", err==ESP_OK ? "OK" : "FAILED");
}

void startSeqTimer() {
    if (seq_timer_handle == NULL) return;
    uint64_t period_us = 60000000ULL / ((uint64_t)bpm * 24);
    seq_tick = 0;
    esp_err_t err = esp_timer_start_periodic(seq_timer_handle, period_us);
    Serial.printf("startSeqTimer %lluus: %s\n", period_us, err==ESP_OK ? "OK" : "FAILED");
}

void stopSeqTimer() {
    if (seq_timer_handle != NULL) {
        esp_timer_stop(seq_timer_handle);
    }
}

void updateSeqTempo() {
    if (seq_timer_handle == NULL || !playing) return;
    esp_timer_stop(seq_timer_handle);
    uint64_t period_us = 60000000ULL / ((uint64_t)bpm * 24);
    esp_timer_start_periodic(seq_timer_handle, period_us);
}

void IRAM_ATTR tic() {
    tic_counter++;

    if (sstep == firstStep) sync_flag = true;

    for (int f = 0; f < 16; f++) {
        if (!bitRead(mutes, f)) {
            if (solos == 0 || (solos > 0 && bitRead(solos, f))) {
                if (bitRead(pattern[f], sstep)) {
                    latch[f]          = 0;
                    pendingTrigger[f] = 1;
                    pendingPitch[f]   = bitRead(isMelodic, f) ? melodic[f][sstep] : 0;
                }
            }
        }
    }

    sstep++;
    if (sstep==(lastStep+1) || sstep==(newLastStep+1) || sstep==16) {
        lastStep = newLastStep;
        sstep    = firstStep;
        if (songing) load_flag = true;
    }
    refreshPADSTEP = true;
}