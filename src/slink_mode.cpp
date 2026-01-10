#include "slink_mode.h"

// Global state instance
SlinkState slink_state;

// ============================================================
// Initialization
// ============================================================

void initializeSlinkMode() {
    // Initialize Trigger Wave (Wave A)
    slink_state.wave_trigger.rate_hz = 0.5f;
    slink_state.wave_trigger.sync_mode = false;
    slink_state.wave_trigger.sync_value = 1.0f;
    slink_state.wave_trigger.phase_inverted = false;
    slink_state.wave_trigger.rate_bipolar = false;
    slink_state.wave_trigger.triplet = false;
    slink_state.wave_trigger.dotted = false;
    slink_state.wave_trigger.freeze = false;
    slink_state.wave_trigger.multiply = 0.5f;
    slink_state.wave_trigger.ripple = 0.5f;
    slink_state.wave_trigger.offset = 0.0f;
    slink_state.wave_trigger.invert = 0.0f;
    slink_state.wave_trigger.gravity = 0.0f;
    slink_state.wave_trigger.scan = 0.0f;
    slink_state.wave_trigger.phase = 0.0f;
    
    // Initialize Pitch Wave (Wave B)
    slink_state.wave_pitch.rate_hz = 0.25f;
    slink_state.wave_pitch.sync_mode = false;
    slink_state.wave_pitch.sync_value = 1.0f;
    slink_state.wave_pitch.phase_inverted = false;
    slink_state.wave_pitch.rate_bipolar = false;
    slink_state.wave_pitch.triplet = false;
    slink_state.wave_pitch.dotted = false;
    slink_state.wave_pitch.freeze = false;
    slink_state.wave_pitch.multiply = 0.5f;
    slink_state.wave_pitch.ripple = 0.5f;
    slink_state.wave_pitch.offset = 0.0f;
    slink_state.wave_pitch.invert = 0.0f;
    slink_state.wave_pitch.gravity = 0.0f;
    slink_state.wave_pitch.scan = 0.0f;
    slink_state.wave_pitch.phase = 0.0f;
    
    // Initialize Bands
    for (int i = 0; i < SLINK_BANDS; i++) {
        slink_state.bands[i].enabled = true;
        slink_state.bands[i].clock_divider = 1.0f;
        slink_state.bands[i].trigger_mode = 0; // Retrigger
        slink_state.bands[i].last_trigger_value = 0.0f;
        slink_state.bands[i].armed_for_once = true;
        slink_state.bands[i].last_clock_tick = 0;
    }
    
    // Initialize Voices
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        slink_state.voices[i].active = false;
        slink_state.voices[i].note = 0;
        slink_state.voices[i].velocity = 0;
        slink_state.voices[i].channel = 1;
        slink_state.voices[i].off_time_ms = 0;
        slink_state.voices[i].band_index = 0;
    }
    
    // Initialize Trigger Engine
    slink_state.trigger_engine.threshold = 0.3f;
    slink_state.trigger_engine.vel_min = 40;
    slink_state.trigger_engine.vel_max = 120;
    slink_state.trigger_engine.forte = 0.5f;
    
    // Initialize Pitch Engine
    slink_state.pitch_engine.spread = 1.0f;
    slink_state.pitch_engine.squish = 0.5f;
    slink_state.pitch_engine.range_semitones = 48; // 4 octaves
    
    // Initialize Clock Engine
    slink_state.clock_engine.bpm = 120.0f;
    slink_state.clock_engine.swing = 0.0f;
    slink_state.clock_engine.note_len_min = 50;
    slink_state.clock_engine.note_len_max = 500;
    slink_state.clock_engine.note_len_x10 = false;
    slink_state.clock_engine.sustain_mode = false;
    slink_state.clock_engine.max_voices = 16;
    slink_state.clock_engine.last_tick_ms = 0;
    slink_state.clock_engine.swing_offset = false;
    
    // Initialize Scale Engine
    slink_state.scale_engine.root_note = 0; // C
    slink_state.scale_engine.scale_index = 0; // Major
    slink_state.scale_engine.color = 1.0f;
    slink_state.scale_engine.arp_mode = false;
    slink_state.scale_engine.num_held_notes = 0;
    for (int i = 0; i < 12; i++) {
        slink_state.scale_engine.custom_scale[i] = false;
    }
    for (int i = 0; i < 128; i++) {
        slink_state.scale_engine.held_notes[i] = 0;
    }
    
    // Initialize Mod Engine
    for (int i = 0; i < SLINK_NUM_MODULATORS; i++) {
        slink_state.mod_engine.mods[i].enabled = false;
        slink_state.mod_engine.mods[i].shape = 0; // sine
        slink_state.mod_engine.mods[i].rate_hz = 0.1f;
        slink_state.mod_engine.mods[i].sync_mode = false;
        slink_state.mod_engine.mods[i].sync_value = 1.0f;
        slink_state.mod_engine.mods[i].triplet = false;
        slink_state.mod_engine.mods[i].dotted = false;
        slink_state.mod_engine.mods[i].range = 0.5f;
        slink_state.mod_engine.mods[i].phase = 0.0f;
        slink_state.mod_engine.mods[i].output = 0.0f;
        slink_state.mod_engine.mods[i].mod_multiply = false;
        slink_state.mod_engine.mods[i].mod_ripple = false;
        slink_state.mod_engine.mods[i].mod_offset = false;
        slink_state.mod_engine.mods[i].mod_gravity = false;
        slink_state.mod_engine.mods[i].mod_scan = false;
        slink_state.mod_engine.mods[i].mod_threshold = false;
    }
    
    // Initialize UI state
    slink_state.current_tab = SLINK_TAB_MAIN;
    slink_state.main_subpage = SLINK_SUBPAGE_WAVE_A;
    
    // Initialize timing
    slink_state.last_engine_tick = millis();
    slink_state.current_time_ms = millis();
}

// ============================================================
// Main Update Loop
// ============================================================

void updateSlinkEngine() {
    uint32_t now = millis();
    slink_state.current_time_ms = now;
    
    // Calculate delta time
    float delta_time_s = (now - slink_state.last_engine_tick) / 1000.0f;
    if (delta_time_s <= 0.0f || delta_time_s > 1.0f) {
        delta_time_s = 0.001f; // Clamp to reasonable values
    }
    slink_state.last_engine_tick = now;
    
    // Update modulators
    updateModulators(delta_time_s, slink_state.clock_engine.bpm);
    
    // Update wave phases
    updateWavePhase(&slink_state.wave_trigger, delta_time_s, slink_state.clock_engine.bpm);
    updateWavePhase(&slink_state.wave_pitch, delta_time_s, slink_state.clock_engine.bpm);
    
    // Compute wave node values
    computeWaveNodes(&slink_state.wave_trigger);
    computeWaveNodes(&slink_state.wave_pitch);
    
    // Process band triggers
    processBandTriggers();
    
    // Process note offs
    processVoiceNoteOffs();
}

// ============================================================
// Wave Engine
// ============================================================

void updateWavePhase(SlinkWave* wave, float delta_time_s, float bpm) {
    if (wave->freeze) return;
    
    float rate_radians_per_sec;
    
    if (wave->sync_mode) {
        // Calculate rate from sync value and BPM
        float interval_s = getSyncInterval(wave->sync_value, wave->triplet, 
                                          wave->dotted, bpm);
        rate_radians_per_sec = (2.0f * PI) / interval_s;
    } else {
        // Use Hz rate
        rate_radians_per_sec = wave->rate_hz * 2.0f * PI;
    }
    
    // Apply phase inversion
    if (wave->phase_inverted) {
        rate_radians_per_sec = -rate_radians_per_sec;
    }
    
    // Update phase
    wave->phase += rate_radians_per_sec * delta_time_s;
    
    // Wrap phase to 0..2π
    while (wave->phase >= 2.0f * PI) wave->phase -= 2.0f * PI;
    while (wave->phase < 0.0f) wave->phase += 2.0f * PI;
}

void computeWaveNodes(SlinkWave* wave) {
    // Each band is a sum of 16 phase-shifted sines
    // This creates the characteristic Slink wave behavior
    
    for (int band = 0; band < SLINK_BANDS; band++) {
        float sum = 0.0f;
        
        // Sum 16 oscillators with different phase offsets
        for (int osc = 0; osc < SLINK_BANDS; osc++) {
            // Phase offset based on band index, oscillator index, and parameters
            float base_phase = wave->phase;
            float band_offset = (band / (float)SLINK_BANDS) * 2.0f * PI * wave->ripple;
            float osc_offset = (osc / (float)SLINK_BANDS) * 2.0f * PI * wave->multiply;
            float global_offset = wave->offset * 2.0f * PI;
            
            float phase = base_phase + band_offset + osc_offset + global_offset;
            
            // Calculate sine value
            float sine_val = sin(phase);
            
            // Apply scan parameter (changes waveshaping algorithm)
            if (wave->scan > 0.0f) {
                // Mix in different wave shapes based on scan
                float tri_val = (2.0f / PI) * asin(sine_val); // triangle approximation
                sine_val = sine_val * (1.0f - wave->scan) + tri_val * wave->scan;
            }
            
            sum += sine_val;
        }
        
        // Normalize
        sum /= SLINK_BANDS;
        
        // Apply invert (attenuverter)
        sum *= (1.0f + wave->invert);
        
        // Apply gravity (DC offset)
        sum += wave->gravity;
        
        // Clamp to -1..1
        if (sum > 1.0f) sum = 1.0f;
        if (sum < -1.0f) sum = -1.0f;
        
        // Convert to 0..1 for node value
        wave->node_values[band] = (sum + 1.0f) / 2.0f;
    }
}

// ============================================================
// Modulation Engine
// ============================================================

void updateModulators(float delta_time_s, float bpm) {
    for (int i = 0; i < SLINK_NUM_MODULATORS; i++) {
        Modulator* mod = &slink_state.mod_engine.mods[i];
        if (!mod->enabled) continue;
        
        // Calculate rate
        float rate_radians_per_sec;
        if (mod->sync_mode) {
            float interval_s = getSyncInterval(mod->sync_value, mod->triplet, 
                                              mod->dotted, bpm);
            rate_radians_per_sec = (2.0f * PI) / interval_s;
        } else {
            rate_radians_per_sec = mod->rate_hz * 2.0f * PI;
        }
        
        // Update phase
        mod->phase += rate_radians_per_sec * delta_time_s;
        while (mod->phase >= 2.0f * PI) mod->phase -= 2.0f * PI;
        
        // Calculate output based on shape
        switch (mod->shape) {
            case 0: // Sine
                mod->output = sin(mod->phase);
                break;
            case 1: // Triangle
                mod->output = (2.0f / PI) * asin(sin(mod->phase));
                break;
            case 2: // Ramp/Sawtooth
                mod->output = (mod->phase / PI) - 1.0f;
                break;
            case 3: // Square
                mod->output = (mod->phase < PI) ? 1.0f : -1.0f;
                break;
            case 4: // Random (sample & hold)
                if (mod->phase < (rate_radians_per_sec * delta_time_s)) {
                    mod->output = ((float)random(0, 2000) / 1000.0f) - 1.0f;
                }
                break;
        }
    }
}

float computeModulatedParameter(float base_value, int param_index) {
    // param_index: 0=multiply, 1=ripple, 2=offset, 3=gravity, 4=scan, 5=threshold
    float result = base_value;
    
    for (int i = 0; i < SLINK_NUM_MODULATORS; i++) {
        Modulator* mod = &slink_state.mod_engine.mods[i];
        if (!mod->enabled) continue;
        
        bool should_modulate = false;
        switch (param_index) {
            case 0: should_modulate = mod->mod_multiply; break;
            case 1: should_modulate = mod->mod_ripple; break;
            case 2: should_modulate = mod->mod_offset; break;
            case 3: should_modulate = mod->mod_gravity; break;
            case 4: should_modulate = mod->mod_scan; break;
            case 5: should_modulate = mod->mod_threshold; break;
        }
        
        if (should_modulate) {
            result += mod->output * mod->range;
        }
    }
    
    // Clamp to appropriate range
    if (param_index == 5) { // threshold
        result = constrain(result, 0.0f, 1.0f);
    } else if (param_index == 3) { // gravity
        result = constrain(result, -1.0f, 1.0f);
    } else { // others
        result = constrain(result, 0.0f, 1.0f);
    }
    
    return result;
}

// ============================================================
// Trigger Processing
// ============================================================

void processBandTriggers() {
    uint32_t now = slink_state.current_time_ms;
    
    for (int i = 0; i < SLINK_BANDS; i++) {
        if (!slink_state.bands[i].enabled) continue;
        
        float trigger_val = slink_state.wave_trigger.node_values[i];
        float pitch_val = slink_state.wave_pitch.node_values[i];
        
        // Check if we should trigger this band
        bool should_trigger = checkBandTrigger(i, trigger_val);
        
        if (should_trigger) {
            // Calculate velocity
            uint8_t velocity = calculateVelocity(trigger_val, &slink_state.trigger_engine);
            
            // Calculate pitch
            uint8_t note = calculatePitch(i, pitch_val);
            quantizeToPitch(&note, &slink_state.scale_engine);
            
            // Calculate note length
            uint32_t note_length = calculateNoteLength(trigger_val, pitch_val, 
                                                       &slink_state.clock_engine);
            
            uint32_t off_time = slink_state.clock_engine.sustain_mode ? 0 : 
                               (now + note_length);
            
            // Allocate voice and send MIDI
            int voice_idx = allocateVoice(note, velocity, 1, off_time, i);
            if (voice_idx >= 0) {
                sendMIDI(0x90, note, velocity);
            }
        }
    }
}

bool checkBandTrigger(int band_index, float trigger_value) {
    Band* band = &slink_state.bands[band_index];
    TriggerEngine* trig = &slink_state.trigger_engine;
    
    // Apply modulation to threshold
    float effective_threshold = computeModulatedParameter(trig->threshold, 5);
    
    switch (band->trigger_mode) {
        case 0: // Retrigger - triggers every time above threshold at clock rate
            {
                // Check if it's time to sample this band based on clock divider
                uint32_t interval = (uint32_t)(1000.0f / band->clock_divider);
                if ((slink_state.current_time_ms - band->last_clock_tick) >= interval) {
                    band->last_clock_tick = slink_state.current_time_ms;
                    if (trigger_value > effective_threshold) {
                        return true;
                    }
                }
            }
            break;
            
        case 1: // Once - triggers once per pass above threshold
            {
                // Check if it's time to sample this band
                uint32_t interval = (uint32_t)(1000.0f / band->clock_divider);
                if ((slink_state.current_time_ms - band->last_clock_tick) >= interval) {
                    band->last_clock_tick = slink_state.current_time_ms;
                    
                    if (trigger_value > effective_threshold && band->armed_for_once) {
                        band->armed_for_once = false;
                        return true;
                    } else if (trigger_value <= effective_threshold) {
                        band->armed_for_once = true;
                    }
                }
            }
            break;
            
        case 2: // Slink - triggers on threshold crossing (ignores clock)
            {
                bool was_below = band->last_trigger_value <= effective_threshold;
                bool is_above = trigger_value > effective_threshold;
                band->last_trigger_value = trigger_value;
                
                if (was_below && is_above) {
                    return true;
                }
            }
            break;
    }
    
    return false;
}

uint8_t calculateVelocity(float trigger_value, TriggerEngine* engine) {
    if (trigger_value <= engine->threshold) return 0;
    
    // Normalize to 0..1 above threshold
    float v_norm = (trigger_value - engine->threshold) / (1.0f - engine->threshold);
    v_norm = constrain(v_norm, 0.0f, 1.0f);
    
    // Apply forte curve
    v_norm = applyForteCurve(v_norm, engine->forte);
    
    // Map to velocity range
    uint8_t velocity = engine->vel_min + (uint8_t)(v_norm * (engine->vel_max - engine->vel_min));
    return constrain(velocity, 1, 127);
}

float applyForteCurve(float normalized, float forte) {
    // forte: 0 = soft (exponential ease-in), 0.5 = linear, 1 = hard (exponential ease-out)
    if (forte < 0.5f) {
        // Soft curve (ease-in)
        float amount = (0.5f - forte) * 2.0f; // 0..1
        float curved = pow(normalized, 1.0f + amount * 2.0f);
        return normalized * (1.0f - amount) + curved * amount;
    } else if (forte > 0.5f) {
        // Hard curve (ease-out)
        float amount = (forte - 0.5f) * 2.0f; // 0..1
        float curved = pow(normalized, 1.0f / (1.0f + amount * 2.0f));
        return normalized * (1.0f - amount) + curved * amount;
    }
    return normalized; // Linear at 0.5
}

// ============================================================
// Pitch Calculation
// ============================================================

uint8_t calculatePitch(int band_index, float pitch_value) {
    PitchEngine* pitch = &slink_state.pitch_engine;
    
    // Apply spread - how much of the range each band can access
    float band_center = band_index / (float)(SLINK_BANDS - 1); // 0..1
    float band_range = pitch->spread;
    
    // Calculate position within spread
    float position = band_center * (1.0f - band_range) + pitch_value * band_range;
    
    // Apply squish (non-linear mapping)
    if (pitch->squish < 0.5f) {
        // Bias toward lower pitches
        float amount = (0.5f - pitch->squish) * 2.0f;
        position = pow(position, 1.0f + amount);
    } else if (pitch->squish > 0.5f) {
        // Bias toward higher pitches
        float amount = (pitch->squish - 0.5f) * 2.0f;
        position = pow(position, 1.0f / (1.0f + amount));
    }
    
    // Map to MIDI note range
    int base_note = 36; // C2
    int note_offset = (int)(position * pitch->range_semitones);
    uint8_t note = base_note + note_offset;
    
    return constrain(note, 0, 127);
}

void quantizeToPitch(uint8_t* note, ScaleEngine* engine) {
    if (engine->arp_mode && engine->num_held_notes > 0) {
        // Use held notes as pitch pool
        int index = (*note) % engine->num_held_notes;
        *note = engine->held_notes[index];
        return;
    }
    
    // Use scale quantization
    int octave = (*note) / 12;
    int semitone = (*note) % 12;
    
    // Get scale intervals
    Scale* scale = &scales[engine->scale_index];
    
    // Apply root note offset
    int relative_semitone = (semitone - engine->root_note + 12) % 12;
    
    // Find nearest scale degree
    int nearest_interval = 0;
    int min_distance = 12;
    
    for (int i = 0; i < scale->numNotes; i++) {
        int interval = scale->intervals[i];
        int distance = abs(relative_semitone - interval);
        if (distance < min_distance) {
            min_distance = distance;
            nearest_interval = interval;
        }
    }
    
    // Calculate quantized note
    int quantized_semitone = (engine->root_note + nearest_interval) % 12;
    *note = octave * 12 + quantized_semitone;
}

// ============================================================
// Voice Management
// ============================================================

int allocateVoice(uint8_t note, uint8_t velocity, uint8_t channel, 
                  uint32_t off_time, uint8_t band_index) {
    // First, try to find an inactive voice
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        if (!slink_state.voices[i].active) {
            slink_state.voices[i].active = true;
            slink_state.voices[i].note = note;
            slink_state.voices[i].velocity = velocity;
            slink_state.voices[i].channel = channel;
            slink_state.voices[i].off_time_ms = off_time;
            slink_state.voices[i].band_index = band_index;
            return i;
        }
    }
    
    // No free voice, check polyphony limit
    int active_count = 0;
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        if (slink_state.voices[i].active) active_count++;
    }
    
    if (active_count >= slink_state.clock_engine.max_voices) {
        // Need to steal a voice
        int steal_idx = findVoiceToSteal();
        if (steal_idx >= 0) {
            releaseVoice(steal_idx);
            return allocateVoice(note, velocity, channel, off_time, band_index);
        }
    }
    
    return -1; // Failed to allocate
}

void releaseVoice(int voice_index) {
    if (voice_index < 0 || voice_index >= SLINK_MAX_VOICES) return;
    
    ActiveVoice* voice = &slink_state.voices[voice_index];
    if (voice->active) {
        sendMIDI(0x80, voice->note, 0);
        voice->active = false;
    }
}

int findVoiceToSteal() {
    // Steal oldest voice (lowest velocity could be alternative)
    uint32_t oldest_time = 0xFFFFFFFF;
    int oldest_idx = -1;
    
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        if (slink_state.voices[i].active) {
            if (slink_state.voices[i].off_time_ms < oldest_time) {
                oldest_time = slink_state.voices[i].off_time_ms;
                oldest_idx = i;
            }
        }
    }
    
    return oldest_idx;
}

void processVoiceNoteOffs() {
    uint32_t now = slink_state.current_time_ms;
    
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        ActiveVoice* voice = &slink_state.voices[i];
        if (voice->active && voice->off_time_ms > 0) {
            if (now >= voice->off_time_ms) {
                releaseVoice(i);
            }
        }
    }
}

// ============================================================
// Utility Functions
// ============================================================

float getSyncInterval(float sync_value, bool triplet, bool dotted, float bpm) {
    // sync_value: bars (e.g., 1.0 = 1 bar, 0.25 = 1/4 bar, 4.0 = 4 bars)
    float beats_per_bar = 4.0f;
    float seconds_per_beat = 60.0f / bpm;
    float interval = sync_value * beats_per_bar * seconds_per_beat;
    
    if (triplet && !dotted) {
        interval *= (2.0f / 3.0f);
    } else if (dotted && !triplet) {
        interval *= 1.5f;
    } else if (triplet && dotted) {
        // Golden ratio-ish behavior
        interval *= 0.618f;
    }
    
    return interval;
}

uint32_t calculateNoteLength(float trigger_val, float pitch_val, ClockEngine* engine) {
    // Combine trigger and pitch values to determine note length
    float combined = (trigger_val + pitch_val) / 2.0f;
    
    uint32_t min_len = engine->note_len_min;
    uint32_t max_len = engine->note_len_max;
    
    if (engine->note_len_x10) {
        min_len *= 10;
        max_len *= 10;
    }
    
    uint32_t length = min_len + (uint32_t)(combined * (max_len - min_len));
    return length;
}

// ============================================================
// Band Operations
// ============================================================

void toggleBand(int band_index) {
    if (band_index >= 0 && band_index < SLINK_BANDS) {
        slink_state.bands[band_index].enabled = !slink_state.bands[band_index].enabled;
    }
}

void enableRandomBand() {
    // Find disabled bands
    int disabled[SLINK_BANDS];
    int count = 0;
    for (int i = 0; i < SLINK_BANDS; i++) {
        if (!slink_state.bands[i].enabled) {
            disabled[count++] = i;
        }
    }
    
    if (count > 0) {
        int idx = disabled[random(0, count)];
        slink_state.bands[idx].enabled = true;
    }
}

void disableRandomBand() {
    // Find enabled bands
    int enabled[SLINK_BANDS];
    int count = 0;
    for (int i = 0; i < SLINK_BANDS; i++) {
        if (slink_state.bands[i].enabled) {
            enabled[count++] = i;
        }
    }
    
    if (count > 0) {
        int idx = enabled[random(0, count)];
        slink_state.bands[idx].enabled = false;
    }
}

void shuffleEnabledBands() {
    // Collect enabled band indices
    int enabled[SLINK_BANDS];
    int count = 0;
    for (int i = 0; i < SLINK_BANDS; i++) {
        if (slink_state.bands[i].enabled) {
            enabled[count++] = i;
        }
    }
    
    // Shuffle using Fisher-Yates
    for (int i = count - 1; i > 0; i--) {
        int j = random(0, i + 1);
        int temp = enabled[i];
        enabled[i] = enabled[j];
        enabled[j] = temp;
    }
    
    // Apply shuffle (actually just re-enable at different positions)
    // For simplicity, just randomize which bands are enabled
    for (int i = 0; i < SLINK_BANDS; i++) {
        slink_state.bands[i].enabled = false;
    }
    for (int i = 0; i < count; i++) {
        int new_idx = random(0, SLINK_BANDS);
        while (slink_state.bands[new_idx].enabled) {
            new_idx = (new_idx + 1) % SLINK_BANDS;
        }
        slink_state.bands[new_idx].enabled = true;
    }
}

void shiftBands() {
    // Rotate enabled band pattern by 1
    bool first = slink_state.bands[0].enabled;
    for (int i = 0; i < SLINK_BANDS - 1; i++) {
        slink_state.bands[i].enabled = slink_state.bands[i + 1].enabled;
    }
    slink_state.bands[SLINK_BANDS - 1].enabled = first;
}

void enableAllBands() {
    for (int i = 0; i < SLINK_BANDS; i++) {
        slink_state.bands[i].enabled = true;
    }
}

void clearAllBands() {
    for (int i = 0; i < SLINK_BANDS; i++) {
        slink_state.bands[i].enabled = false;
    }
}

// ============================================================
// Main Draw and Handle Functions (Stubs)
// ============================================================

void drawSlinkMode() {
    switch (slink_state.current_tab) {
        case SLINK_TAB_MAIN:
            drawMainTab();
            break;
        case SLINK_TAB_TRIGGER:
            drawTriggerTab();
            break;
        case SLINK_TAB_PITCH:
            drawPitchTab();
            break;
        case SLINK_TAB_CLOCK:
            drawClockTab();
            break;
        case SLINK_TAB_SCALE:
            drawScaleTab();
            break;
        case SLINK_TAB_MOD:
            drawModTab();
            break;
        case SLINK_TAB_SETUP:
            drawSetupTab();
            break;
    }
}

void handleSlinkMode() {
    // Update engine
    updateSlinkEngine();
    
    // Handle UI will be implemented in UI section
    // For now, just handle back button
    if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, 
                                             BACK_BUTTON_W, BACK_BUTTON_H)) {
        exitToMenu();
        return;
    }
}

// UI stubs - will be implemented next
void drawMainTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Main");
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("Slink Mode - Coming Soon", MARGIN_SMALL, HEADER_HEIGHT + SCALE_Y(20), 2);
}

void drawTriggerTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Trigger");
}

void drawPitchTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Pitch");
}

void drawClockTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Clock");
}

void drawScaleTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Scale");
}

void drawModTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Mod");
}

void drawSetupTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Setup");
}
