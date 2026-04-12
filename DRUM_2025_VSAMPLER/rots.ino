void select_rot(){
    if (selected_rot==11){
        counter1=master_filter; 
    } else if (selected_rot==12){
        counter1=octave;      
    } else if (selected_rot==8){
        counter1=bpm;
    } else if (selected_rot==9){
        counter1=master_vol;
    } else if (selected_rot==10){
        ztranspose=0;
        counter1=ztranspose;
    } else if (selected_rot==13){
        zmpitch=0;
        counter1=zmpitch;                
    } else if (selected_rot==14){
        counter1=sync_state; 
    } else if (selected_rot==15){    
        counter1=selected_scale;
    } else {
        counter1=ROTvalue[selected_sound][selected_rot];
    }
}

// ============================================================
void do_rot(){

    if (counter1 > max_values[selected_rot]) counter1 = max_values[selected_rot];
    if (counter1 < min_values[selected_rot]) counter1 = min_values[selected_rot];

    if (counter1 != old_counter1){
    
        if (selected_rot==11){
            master_filter = counter1;
            synthESP32_setMFilter(master_filter);
        } else if (selected_rot==12){
            octave = counter1;
        } else if (selected_rot==8){
            bpm = counter1;
            updateSeqTempo();
        } else if (selected_rot==9){
            master_vol = counter1;
            synthESP32_setMVol(master_vol);
        } else if (selected_rot==10){
            ztranspose = counter1;
            for (byte f=0; f<16; f++){
                melodic[selected_sound][f] = melodic[selected_sound][f] + ztranspose;
            }
            if (modeZ==tWrite) refreshPATTERN=true;
            ztranspose=0;
            counter1=ztranspose;
        } else if (selected_rot==13){
            zmpitch = counter1;
            for (byte k=0; k<16; k++){  
                ROTvalue[k][3] = ROTvalue[k][3] + zmpitch;  
                for (byte f=0; f<16; f++){
                    melodic[k][f] = melodic[k][f] + zmpitch;
                } 
            }
            if (modeZ==tWrite) refreshPATTERN=true;
            zmpitch=0;
            counter1=zmpitch;
        } else if (selected_rot==14){
            sync_state = counter1; 
        } else if (selected_rot==15){
            selected_scale = counter1;
        } else {       
            if (selected_rot==1) {
                if (counter1 > ROTvalue[selected_sound][2]) counter1=ROTvalue[selected_sound][2];
            }
            if (selected_rot==2) {
                if (counter1 < ROTvalue[selected_sound][1]) counter1=ROTvalue[selected_sound][1];
            }
            ROTvalue[selected_sound][selected_rot] = counter1;
            setSound(selected_sound);
            if (selected_rot<8 && !playing){
                synthESP32_TRIGGER(selected_sound);
            }
        }
        drawBar(selected_rot);
    }  
}

// ============================================================
void READ_ENCODERS(){
    bitWrite(ENC_PORT1, 0, !digitalRead(DT));
    bitWrite(ENC_PORT1, 1, !digitalRead(CLK));
    old_counter1 = counter1;
    counter1 = counter1 + read_encoder1();
    do_rot();  
}

// ============================================================
int16_t read_encoder1() {
    static int8_t enc_states[] = {0,-1,0,0,1,0,0,0,0,0,0,0,0,0,0,0};
    static uint8_t old_AB1;
    old_AB1 <<= 2;
    old_AB1 |= (ENC_PORT1 & 0x03);
    int8_t result = enc_states[(old_AB1 & 0x0f)];
    if (result > 0) return (result + (9*shiftR1));
    if (result < 0) return (result - (9*shiftR1));
    return 0;
}

// ============================================================
// ADS1115 16-bit — GAIN_ONE (±4.096V)
// At 3.3V max raw ≈ 26400
// Noise deadband = 50 counts (~6mV)
// ============================================================

#define ADS_MAX   26400   // ← ADS1115 with GAIN_ONE at 3.3V
#define ADS_NOISE 50      // ← deadband to prevent jitter

void READ_POTS(){

    // --- Read all 4 channels (blocking, called every 100ms) ---
    adc0 = constrain(ads.readADC_SingleEnded(0), 0, ADS_MAX);
    adc1 = constrain(ads.readADC_SingleEnded(1), 0, ADS_MAX);
    adc2 = constrain(ads.readADC_SingleEnded(2), 0, ADS_MAX);
    adc3 = constrain(ads.readADC_SingleEnded(3), 0, ADS_MAX);

    // --- A0: unused / spare (was doing nothing before) ---
    if (abs(adc0 - old_adc0) > ADS_NOISE){
        old_adc0 = adc0;
        // assign as needed
    }

    // --- A1: FX1 mode (0=off, 1=quarter, 2=eighth, 3=sixteenth) ---
    if (abs(adc1 - old_adc1) > ADS_NOISE){
        old_adc1 = adc1;
        int fx_mapped = map(adc1, 0, ADS_MAX, 0, 12);
        if      (fx_mapped > 6) fx1 = 3;
        else if (fx_mapped > 3) fx1 = 2;
        else if (fx_mapped > 1) fx1 = 1;
        else                    fx1 = 0;
    }

    // --- A2: Master Volume (0–127) ---
    if (abs(adc2 - old_adc2) > ADS_NOISE){
        old_adc2 = adc2;
        master_vol = map(adc2, 0, ADS_MAX, 0, 127);
        synthESP32_setMVol(master_vol);
        drawBar(9);
        Serial.printf("MasterVol: %d\n", master_vol);
    }

    // --- A3: Master Filter (0–127) ---
    if (abs(adc3 - old_adc3) > ADS_NOISE){
        old_adc3 = adc3;
        master_filter = map(adc3, 0, ADS_MAX, 0, 127);
        synthESP32_setMFilter(master_filter);
        drawBar(11);
        Serial.printf("MasterFilter: %d\n", master_filter);
    }
}