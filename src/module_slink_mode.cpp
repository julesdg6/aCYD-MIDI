#include "module_slink_mode.h"
#include <new>

// Global state instance
SlinkState *slink_state_ptr = nullptr;

// ============================================================
// Initialization
// ============================================================

void initializeSlinkMode() {
    if (!slink_state_ptr) {
        slink_state_ptr = new (std::nothrow) SlinkState();
        if (!slink_state_ptr) {
            return;
        }
    }
    *slink_state_ptr = {};

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
    if (!slink_state_ptr) {
        return;
    }
    uint32_t now = millis();
    slink_state.current_time_ms = now;
    slink_state.clock_engine.bpm = static_cast<float>(sharedBPM);
    
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
    if (!slink_state_ptr) {
        return;
    }
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
    if (!slink_state_ptr) {
        return;
    }
    // Update engine
    updateSlinkEngine();
    
    // Handle back button
    if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, 
                                             BACK_BUTTON_W, BACK_BUTTON_H)) {
        exitToMenu();
        return;
    }
    
    if (!touch.justPressed) return;

    int hit = hitSlinkTab(touch.x, touch.y);
    if (hit >= 0) {
        slink_state.current_tab = static_cast<SlinkTab>(hit);
        requestRedraw();
        return;
    }
    
    // Handle tab-specific interactions
    switch (slink_state.current_tab) {
        case SLINK_TAB_MAIN:
            handleMainTab();
            break;
        case SLINK_TAB_TRIGGER:
            handleTriggerTab();
            break;
        case SLINK_TAB_PITCH:
            handlePitchTab();
            break;
        case SLINK_TAB_CLOCK:
            handleClockTab();
            break;
        case SLINK_TAB_SCALE:
            handleScaleTab();
            break;
        case SLINK_TAB_MOD:
            handleModTab();
            break;
        case SLINK_TAB_SETUP:
            handleSetupTab();
            break;
    }
}

// ============================================================
// UI Implementation
// ============================================================

// Put all 7 tabs on a single row - they'll be smaller but clearer
static constexpr int kSlinkTabsPerRow = 7;  // Changed from 4 to 7
static constexpr int kSlinkTabCount = SLINK_TAB_SETUP + 1;
static const char *const kSlinkTabLabels[kSlinkTabCount] = {
    "MAIN",
    "TRIG",
    "PITC",
    "CLOK",
    "SCAL",
    "MOD",
    "SETP",
};

static int getSlinkTabHeight() {
    return SCALE_Y(38);
}

static int getSlinkTabSpacing() {
    return SCALE_X(6);
}

static int getSlinkTabRows() {
    return (kSlinkTabCount + kSlinkTabsPerRow - 1) / kSlinkTabsPerRow;
}

static int getSlinkTabBarHeight() {
    int rows = getSlinkTabRows();
    return rows * getSlinkTabHeight() + (rows + 1) * getSlinkTabSpacing();
}

static void getSlinkTabRect(int index, int &x, int &y, int &w, int &h) {
    h = getSlinkTabHeight();
    int spacing = getSlinkTabSpacing();
    int cols = kSlinkTabsPerRow;
    int row = index / cols;
    int col = index % cols;
    int textWidth = DISPLAY_WIDTH - 2 * MARGIN_SMALL - (cols - 1) * spacing;
    w = textWidth / cols;
    x = MARGIN_SMALL + col * (w + spacing);
    y = HEADER_HEIGHT + spacing + row * (h + spacing);
}

static void drawSlinkTabBar() {
    for (int i = 0; i < kSlinkTabCount; i++) {
        int x, y, w, h;
        getSlinkTabRect(i, x, y, w, h);
        bool active = (i == static_cast<int>(slink_state.current_tab));
        drawRoundButton(x, y, w, h, kSlinkTabLabels[i], active ? THEME_ACCENT : THEME_SURFACE,
                        active, 2);
    }
}

int hitSlinkTab(int px, int py) {
    for (int i = 0; i < kSlinkTabCount; i++) {
        int x, y, w, h;
        getSlinkTabRect(i, x, y, w, h);
        if (px >= x && px <= x + w && py >= y && py <= y + h) {
            return i;
        }
    }
    return -1;
}

static inline int getBandToggleRowCount() {
    return 2;
}

static inline int getBandTogglePerRow() {
    return SLINK_BANDS / getBandToggleRowCount();
}

static inline int getBandToggleSpacing() {
    return SCALE_X(2);
}

static inline int getBandToggleHeight() {
    return SCALE_Y(30);
}

static void getBandToggleRect(int index, int y_start, int &x, int &y, int &w, int &h) {
    int perRow = getBandTogglePerRow();
    int row = index / perRow;
    int col = index % perRow;
    int spacing = getBandToggleSpacing();
    w = (DISPLAY_WIDTH - 2 * MARGIN_SMALL - (perRow - 1) * spacing) / perRow;
    h = getBandToggleHeight();
    x = MARGIN_SMALL + col * (w + spacing);
    y = y_start + row * (h + spacing);
}

static const char *const kMainHelperLabels[] = {"+1", "-1", "MIX", "SFT", "ALL", "CLR"};
static const uint16_t kMainHelperColors[] = {
    THEME_SUCCESS,
    THEME_ERROR,
    THEME_WARNING,
    THEME_ACCENT,
    THEME_PRIMARY,
    THEME_SECONDARY,
};
static_assert(sizeof(kMainHelperColors) / sizeof(kMainHelperColors[0]) ==
                  sizeof(kMainHelperLabels) / sizeof(kMainHelperLabels[0]),
              "Helper colors must align with labels");

static void getHelperButtonRect(int index, int y_start, int &x, int &y, int &w, int &h) {
    const int columns = 6;  // Fit all 6 buttons in one row
    const int rows = 1;
    int spacingX = SCALE_X(4);  // Reduced spacing to fit
    int spacingY = SCALE_Y(6);
    h = SCALE_Y(36);
    w = (DISPLAY_WIDTH - 2 * MARGIN_SMALL - (columns - 1) * spacingX) / columns;
    int row = index / columns;
    int col = index % columns;
    x = MARGIN_SMALL + col * (w + spacingX);
    y = y_start + row * (h + spacingY);
}

void drawMainTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Wave Engine", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    
    // Wave toggle buttons at top
    int toggleW = SCALE_X(70);
    int toggleH = SCALE_Y(32);
    int toggleY = contentY;
    drawRoundButton(MARGIN_SMALL, toggleY, toggleW, toggleH, "WAVE A",
                    slink_state.main_subpage == SLINK_SUBPAGE_WAVE_A ? THEME_WARNING : THEME_SURFACE, 
                    slink_state.main_subpage == SLINK_SUBPAGE_WAVE_A, 2);
    drawRoundButton(MARGIN_SMALL + toggleW + SCALE_X(6), toggleY, toggleW, toggleH, "WAVE B",
                    slink_state.main_subpage == SLINK_SUBPAGE_WAVE_B ? THEME_ACCENT : THEME_SURFACE,
                    slink_state.main_subpage == SLINK_SUBPAGE_WAVE_B, 2);
    
    // Draw selected wave visualization - LARGER since we only show one
    int waveY = toggleY + toggleH + SCALE_Y(10);
    int waveHeight = SCALE_Y(110);  // Much larger - was 70px
    
    if (slink_state.main_subpage == SLINK_SUBPAGE_WAVE_A) {
        drawWaveVisualization(waveY, waveHeight, &slink_state.wave_trigger, THEME_WARNING, "WAVE A (Trigger)");
    } else {
        drawWaveVisualization(waveY, waveHeight, &slink_state.wave_pitch, THEME_ACCENT, "WAVE B (Pitch)");
    }

    // Band toggles below wave
    int bandY = waveY + waveHeight + SCALE_Y(12);
    drawBandToggles(bandY);

    // Helper buttons at bottom - all on one row
    int helperY = bandY + getBandToggleRowCount() * (getBandToggleHeight() + getBandToggleSpacing()) + SCALE_Y(10);
    for (int i = 0; i < 6; i++) {
        int x, y, w, h;
        getHelperButtonRect(i, helperY, x, y, w, h);
        drawRoundButton(x, y, w, h, kMainHelperLabels[i], kMainHelperColors[i], false, 2);
    }

    // Status line at bottom
    int statusY = helperY + SCALE_Y(42);
    char statusBuf[64];
    snprintf(statusBuf, sizeof(statusBuf), "BPM:%d | Voices:%d/%d",
             sharedBPM,
             countActiveVoices(),
             slink_state.clock_engine.max_voices);
    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString(statusBuf, MARGIN_SMALL, statusY, 1);
}

void drawTriggerTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Trigger Engine", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int sliderX = MARGIN_SMALL;
    int sliderY = contentY;
    int sliderW = SCALE_X(70);
    int sliderH = DISPLAY_HEIGHT - sliderY - SCALE_Y(80);
    drawSliderControl(sliderX, sliderY, sliderW, sliderH,
                      slink_state.trigger_engine.threshold, "THRESHOLD", THEME_WARNING);

    int ctrlX = sliderX + sliderW + SCALE_X(24);
    int ctrlY = contentY;
    int ctrlW = DISPLAY_WIDTH - ctrlX - MARGIN_SMALL;
    int btnH = SCALE_Y(44);
    char buf[32];
    snprintf(buf, sizeof(buf), "MIN VELO: %d", slink_state.trigger_engine.vel_min);
    drawRoundButton(ctrlX, ctrlY, ctrlW, btnH, buf, THEME_SECONDARY, false, 2);
    snprintf(buf, sizeof(buf), "MAX VELO: %d", slink_state.trigger_engine.vel_max);
    drawRoundButton(ctrlX, ctrlY + btnH + SCALE_Y(10), ctrlW, btnH, buf, THEME_SECONDARY, false, 2);
    snprintf(buf, sizeof(buf), "FORTE: %.0f%%", slink_state.trigger_engine.forte * 100);
    drawRoundButton(ctrlX, ctrlY + 2 * (btnH + SCALE_Y(10)), ctrlW, btnH, buf, THEME_ACCENT, false, 2);
}

void drawPitchTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Pitch Engine", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int vizHeight = SCALE_Y(70);
    drawPitchGrid(contentY, vizHeight);
    drawWaveVisualization(contentY, vizHeight, &slink_state.wave_pitch, THEME_ACCENT, "PITCH WAVE");

    int btnY = contentY + vizHeight + SCALE_Y(10);
    int btnH = SCALE_Y(42);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
    char buf[32];
    snprintf(buf, sizeof(buf), "SPREAD: %.0f%%", slink_state.pitch_engine.spread * 100);
    drawRoundButton(MARGIN_SMALL, btnY, btnW, btnH, buf, THEME_PRIMARY, false, 2);
    btnY += btnH + SCALE_Y(8);
    snprintf(buf, sizeof(buf), "SQUISH: %.0f%%", slink_state.pitch_engine.squish * 100);
    drawRoundButton(MARGIN_SMALL, btnY, btnW, btnH, buf, THEME_ACCENT, false, 2);
    btnY += btnH + SCALE_Y(8);
    snprintf(buf, sizeof(buf), "RANGE: %dst", slink_state.pitch_engine.range_semitones);
    drawRoundButton(MARGIN_SMALL, btnY, btnW, btnH, buf, THEME_WARNING, false, 2);
}

void drawClockTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Clock & Length", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int mainH = SCALE_Y(52);
    drawRoundButton(MARGIN_SMALL, contentY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, mainH,
                    String(sharedBPM) + " BPM", THEME_PRIMARY, false, 4);

    int adjustY = contentY + mainH + SCALE_Y(10);
    int adjustH = SCALE_Y(42);
    int adjustW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;
    drawRoundButton(MARGIN_SMALL, adjustY, adjustW, adjustH, "-", THEME_ERROR, false, 5);
    drawRoundButton(MARGIN_SMALL + adjustW + MARGIN_SMALL, adjustY, adjustW, adjustH, "+", THEME_SUCCESS, false, 5);

    int swingY = adjustY + adjustH + SCALE_Y(12);
    drawRoundButton(MARGIN_SMALL, swingY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(40),
                    String("Swing: ") + String((int)(slink_state.clock_engine.swing * 100)) + "%", THEME_ACCENT, false, 2);

    int noteY = swingY + SCALE_Y(40) + SCALE_Y(10);
    int halfW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;
    char buf[32];
    snprintf(buf, sizeof(buf), "Min: %dms", slink_state.clock_engine.note_len_min);
    drawRoundButton(MARGIN_SMALL, noteY, halfW, SCALE_Y(38), buf, THEME_WARNING, false, 2);
    snprintf(buf, sizeof(buf), "Max: %dms", slink_state.clock_engine.note_len_max);
    drawRoundButton(MARGIN_SMALL + halfW + MARGIN_SMALL, noteY, halfW, SCALE_Y(38), buf, THEME_WARNING, false, 2);

    int toggleY = noteY + SCALE_Y(38) + SCALE_Y(8);
    drawRoundButton(MARGIN_SMALL, toggleY, halfW, SCALE_Y(38),
                    slink_state.clock_engine.note_len_x10 ? "x10: ON" : "x10: OFF", THEME_ACCENT, false, 2);
    drawRoundButton(MARGIN_SMALL + halfW + MARGIN_SMALL, toggleY, halfW, SCALE_Y(38),
                    slink_state.clock_engine.sustain_mode ? "SUST: ON" : "SUST: OFF", THEME_ACCENT, false, 2);

    int voiceY = toggleY + SCALE_Y(38) + SCALE_Y(8);
    drawRoundButton(MARGIN_SMALL, voiceY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(38),
                    "Voices: " + String(slink_state.clock_engine.max_voices), THEME_PRIMARY, false, 2);
}

void drawScaleTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Scale & Arp", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    int btnH = SCALE_Y(42);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
    char buf[64];
    drawRoundButton(MARGIN_SMALL, contentY, btnW, btnH,
                    slink_state.scale_engine.arp_mode ? "ARP MODE: ON" : "ARP MODE: OFF",
                    slink_state.scale_engine.arp_mode ? THEME_SUCCESS : THEME_SECONDARY, false, 2);

    int rowY = contentY + btnH + SCALE_Y(12);
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    if (!slink_state.scale_engine.arp_mode) {
        snprintf(buf, sizeof(buf), "Root: %s", noteNames[slink_state.scale_engine.root_note]);
        drawRoundButton(MARGIN_SMALL, rowY, btnW, btnH, buf, THEME_PRIMARY, false, 2);
        rowY += btnH + SCALE_Y(8);
        snprintf(buf, sizeof(buf), "Scale: %s", scales[slink_state.scale_engine.scale_index].name.c_str());
        drawRoundButton(MARGIN_SMALL, rowY, btnW, btnH, buf, THEME_ACCENT, false, 2);
        rowY += btnH + SCALE_Y(8);
        snprintf(buf, sizeof(buf), "Color: %d%%", static_cast<int>(slink_state.scale_engine.color * 100));
        drawRoundButton(MARGIN_SMALL, rowY, btnW, btnH, buf, THEME_WARNING, false, 2);
        rowY += btnH + SCALE_Y(12);
        drawMiniKeyboard(rowY, &slink_state.scale_engine);
    } else {
        rowY += SCALE_Y(10);
        tft.setTextColor(THEME_TEXT, THEME_BG);
        snprintf(buf, sizeof(buf), "Held notes: %d", slink_state.scale_engine.num_held_notes);
        tft.drawString(buf, MARGIN_SMALL, rowY, 2);
        rowY += SCALE_Y(25);
        tft.drawString("Use MIDI input to define", MARGIN_SMALL, rowY, 1);
        rowY += SCALE_Y(20);
        tft.drawString("your pitch pool", MARGIN_SMALL, rowY, 1);
    }
}

void drawModTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Modulators", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    int blockHeight = SCALE_Y(38);
    int halfW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;
    const char* shapes[] = {"SIN", "TRI", "SAW", "SQR", "RND"};
    char buf[32];

    for (int i = 0; i < 3; i++) {
        int y = contentY + i * (blockHeight * 2 + SCALE_Y(14));
        Modulator* mod = &slink_state.mod_engine.mods[i];
        char label = 'A' + i;
        drawRoundButton(MARGIN_SMALL, y, halfW, blockHeight, String("[") + String(label) + "]",
                        mod->enabled ? THEME_SUCCESS : THEME_SURFACE, false, 2);

        if (mod->enabled) {
            drawRoundButton(MARGIN_SMALL + halfW + MARGIN_SMALL, y, halfW, blockHeight,
                            shapes[mod->shape], THEME_ACCENT, false, 2);
            snprintf(buf, sizeof(buf), "%.1fHz", mod->rate_hz);
            drawRoundButton(MARGIN_SMALL, y + blockHeight + SCALE_Y(6), halfW, blockHeight, buf, THEME_PRIMARY, false, 2);
            snprintf(buf, sizeof(buf), "RNG: %d%%", static_cast<int>(mod->range * 100));
            drawRoundButton(MARGIN_SMALL + halfW + MARGIN_SMALL, y + blockHeight + SCALE_Y(6), halfW, blockHeight, buf, THEME_WARNING, false, 2);
        } else {
            tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
            tft.drawString("Disabled", MARGIN_SMALL + halfW + SCALE_X(4), y + blockHeight / 2 - SCALE_Y(4), 1);
        }
    }

    tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
    tft.drawString("Tap each label to adjust shape/rate/range", MARGIN_SMALL, contentY + 3 * (blockHeight * 2 + SCALE_Y(14)), 1);
}

void drawSetupTab() {
    tft.fillScreen(THEME_BG);
    drawHeader("SLINK", "Presets", 4);
    drawSlinkTabBar();

    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawString("Preset System", MARGIN_SMALL, contentY, 2);
    tft.drawString("Coming soon...", MARGIN_SMALL, contentY + SCALE_Y(25), 1);

    int btnY = contentY + SCALE_Y(60);
    int btnH = SCALE_Y(44);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;
    drawRoundButton(MARGIN_SMALL, btnY, btnW, btnH, "SAVE", THEME_SUCCESS, false, 2);
    drawRoundButton(MARGIN_SMALL, btnY + btnH + SCALE_Y(10), btnW, btnH, "LOAD", THEME_PRIMARY, false, 2);
    drawRoundButton(MARGIN_SMALL, btnY + 2 * (btnH + SCALE_Y(10)), btnW, btnH, "INIT", THEME_WARNING, false, 2);
}

// ============================================================
// Helper UI Functions
// ============================================================

void drawWaveVisualization(int y_start, int height, SlinkWave* wave, 
                          uint16_t color, const char* label) {
    // Draw label
    tft.setTextColor(color, THEME_BG);
    tft.drawString(label, MARGIN_SMALL, y_start - SCALE_Y(10), 1);
    
    // Draw 16 bands as vertical bars
    int barW = DISPLAY_WIDTH / SLINK_BANDS;
    for (int i = 0; i < SLINK_BANDS; i++) {
        int x = i * barW;
        float value = wave->node_values[i];
        int barH = (int)(value * height);
        
        // Draw bar from bottom up
        int barY = y_start + height - barH;
        
        uint16_t barColor = color;
        if (!slink_state.bands[i].enabled) {
            barColor = THEME_SURFACE; // Dimmed if band disabled
        }
        
        tft.fillRect(x + 1, barY, barW - 2, barH, barColor);
        tft.drawRect(x, y_start, barW, height, THEME_TEXT_DIM);
    }
}

void drawBandToggles(int y_pos) {
    for (int i = 0; i < SLINK_BANDS; i++) {
        int x, y, w, h;
        getBandToggleRect(i, y_pos, x, y, w, h);
        uint16_t color = slink_state.bands[i].enabled ? THEME_SUCCESS : THEME_SURFACE;
        tft.fillRect(x + 1, y, w - 2, h, color);
        tft.drawRect(x, y, w, h, THEME_TEXT_DIM);
    }
}

void drawThresholdLine(int y_pos, float threshold) {
    // Draw a horizontal line at threshold position
    int lineY = y_pos + SCALE_Y(60) - (int)(threshold * SCALE_Y(60));
    tft.drawFastHLine(0, lineY, DISPLAY_WIDTH, THEME_ERROR);
}

void drawPitchGrid(int y_start, int height) {
    // Draw octave markers (horizontal lines at C notes)
    int octaves = 4; // Show 4 octaves
    for (int i = 0; i <= octaves; i++) {
        int y = y_start + (i * height / octaves);
        tft.drawFastHLine(0, y, DISPLAY_WIDTH, THEME_TEXT_DIM);
    }
}

void drawMiniKeyboard(int y_pos, ScaleEngine* engine) {
    // Draw a simple 12-key representation
    int keyW = DISPLAY_WIDTH / 12;
    int keyH = SCALE_Y(26);
    
    for (int i = 0; i < 12; i++) {
        int x = i * keyW;
        
        // Check if this note is in the scale
        bool inScale = false;
        Scale* scale = &scales[engine->scale_index];
        for (int j = 0; j < scale->numNotes; j++) {
            if (scale->intervals[j] == i) {
                inScale = true;
                break;
            }
        }
        
        uint16_t color = inScale ? THEME_PRIMARY : THEME_SURFACE;
        if (i == engine->root_note) {
            color = THEME_WARNING; // Highlight root
        }
        
        tft.fillRect(x + 1, y_pos, keyW - 2, keyH, color);
        tft.drawRect(x, y_pos, keyW, keyH, THEME_TEXT_DIM);
    }
}

void drawSliderControl(int x, int y, int w, int h, float value, 
                      const char* label, uint16_t color) {
    // Draw vertical slider
    tft.drawRect(x, y, w, h, THEME_TEXT_DIM);
    
    int fillH = (int)(value * h);
    tft.fillRect(x + 2, y + h - fillH, w - 4, fillH, color);
    
    // Label
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawCentreString(label, x + w/2, y + h + SCALE_Y(5), 1);
}

void drawKnobControl(int x, int y, int radius, float value, 
                    const char* label, uint16_t color) {
    // Draw circular knob (simplified as arc)
    float angle = value * 270.0f - 135.0f; // -135 to +135 degrees
    
    tft.drawCircle(x, y, radius, THEME_TEXT_DIM);
    
    // Draw value indicator
    int endX = x + (int)(cos(angle * PI / 180.0f) * radius);
    int endY = y + (int)(sin(angle * PI / 180.0f) * radius);
    tft.drawLine(x, y, endX, endY, color);
    
    // Label
    tft.setTextColor(THEME_TEXT, THEME_BG);
    tft.drawCentreString(label, x, y + radius + SCALE_Y(5), 1);
}

int countActiveVoices() {
    int count = 0;
    for (int i = 0; i < SLINK_MAX_VOICES; i++) {
        if (slink_state.voices[i].active) count++;
    }
    return count;
}

// ============================================================
// UI Event Handlers
// ============================================================

void handleMainTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    
    // Handle wave toggle buttons
    int toggleW = SCALE_X(70);
    int toggleH = SCALE_Y(32);
    int toggleY = contentY;
    
    if (isButtonPressed(MARGIN_SMALL, toggleY, toggleW, toggleH)) {
        slink_state.main_subpage = SLINK_SUBPAGE_WAVE_A;
        requestRedraw();
        return;
    }
    if (isButtonPressed(MARGIN_SMALL + toggleW + SCALE_X(6), toggleY, toggleW, toggleH)) {
        slink_state.main_subpage = SLINK_SUBPAGE_WAVE_B;
        requestRedraw();
        return;
    }
    
    // Calculate positions based on new layout
    int waveY = toggleY + toggleH + SCALE_Y(10);
    int waveHeight = SCALE_Y(110);
    int bandY = waveY + waveHeight + SCALE_Y(12);

    for (int i = 0; i < SLINK_BANDS; i++) {
        int x, y, w, h;
        getBandToggleRect(i, bandY, x, y, w, h);
        if (isButtonPressed(x, y, w, h)) {
            toggleBand(i);
            requestRedraw();
            return;
        }
    }

    int helperY = bandY + getBandToggleRowCount() * (getBandToggleHeight() + getBandToggleSpacing()) + SCALE_Y(10);
    for (int i = 0; i < 6; i++) {
        int x, y, w, h;
        getHelperButtonRect(i, helperY, x, y, w, h);
        if (isButtonPressed(x, y, w, h)) {
            switch (i) {
                case 0:
                    enableRandomBand();
                    break;
                case 1:
                    disableRandomBand();
                    break;
                case 2:
                    shuffleEnabledBands();
                    break;
                case 3:
                    shiftBands();
                    break;
                case 4:
                    enableAllBands();
                    break;
                case 5:
                    clearAllBands();
                    break;
            }
            requestRedraw();
            return;
        }
    }
}

void handleTriggerTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int sliderX = MARGIN_SMALL;
    int sliderY = contentY;
    int sliderW = SCALE_X(70);
    int sliderH = DISPLAY_HEIGHT - sliderY - SCALE_Y(80);

    if (touch.isPressed && touch.x >= sliderX && touch.x <= sliderX + sliderW &&
        touch.y >= sliderY && touch.y <= sliderY + sliderH) {
        float newThreshold = 1.0f - ((touch.y - sliderY) / (float)sliderH);
        slink_state.trigger_engine.threshold = constrain(newThreshold, 0.0f, 1.0f);
        requestRedraw();
        return;
    }

    int ctrlX = sliderX + sliderW + SCALE_X(24);
    int ctrlY = contentY;
    int ctrlW = DISPLAY_WIDTH - ctrlX - MARGIN_SMALL;
    int btnH = SCALE_Y(44);

    if (isButtonPressed(ctrlX, ctrlY, ctrlW, btnH)) {
        slink_state.trigger_engine.vel_min = (slink_state.trigger_engine.vel_min + 10) % 128;
        if (slink_state.trigger_engine.vel_min > slink_state.trigger_engine.vel_max) {
            slink_state.trigger_engine.vel_min = 0;
        }
        requestRedraw();
    } else if (isButtonPressed(ctrlX, ctrlY + btnH + SCALE_Y(10), ctrlW, btnH)) {
        slink_state.trigger_engine.vel_max = (slink_state.trigger_engine.vel_max + 10) % 128;
        if (slink_state.trigger_engine.vel_max == 0) slink_state.trigger_engine.vel_max = 127;
        if (slink_state.trigger_engine.vel_max < slink_state.trigger_engine.vel_min) {
            slink_state.trigger_engine.vel_max = 127;
        }
        requestRedraw();
    } else if (isButtonPressed(ctrlX, ctrlY + 2 * (btnH + SCALE_Y(10)), ctrlW, btnH)) {
        slink_state.trigger_engine.forte += 0.1f;
        if (slink_state.trigger_engine.forte > 1.0f) slink_state.trigger_engine.forte = 0.0f;
        requestRedraw();
    }
}

void handlePitchTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int vizHeight = SCALE_Y(70);
    int btnY = contentY + vizHeight + SCALE_Y(10);
    int btnH = SCALE_Y(42);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;

    if (isButtonPressed(MARGIN_SMALL, btnY, btnW, btnH)) {
        slink_state.pitch_engine.spread += 0.1f;
        if (slink_state.pitch_engine.spread > 1.0f) slink_state.pitch_engine.spread = 0.0f;
        requestRedraw();
    } else if (isButtonPressed(MARGIN_SMALL, btnY + btnH + SCALE_Y(8), btnW, btnH)) {
        slink_state.pitch_engine.squish += 0.1f;
        if (slink_state.pitch_engine.squish > 1.0f) slink_state.pitch_engine.squish = 0.0f;
        requestRedraw();
    } else if (isButtonPressed(MARGIN_SMALL, btnY + 2 * (btnH + SCALE_Y(8)), btnW, btnH)) {
        slink_state.pitch_engine.range_semitones += 12;
        if (slink_state.pitch_engine.range_semitones > 72) slink_state.pitch_engine.range_semitones = 12;
        requestRedraw();
    }
}

void handleClockTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(8);
    int mainH = SCALE_Y(52);
    int adjustY = contentY + mainH + SCALE_Y(10);
    int adjustH = SCALE_Y(42);
    int adjustW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;

    if (isButtonPressed(MARGIN_SMALL, adjustY, adjustW, adjustH)) {
        uint16_t newBPM;
        if (sharedBPM <= 45) {
            newBPM = 40;
        } else {
            newBPM = sharedBPM - 5;
        }
        slink_state.clock_engine.bpm = static_cast<float>(newBPM);
        setSharedBPM(newBPM);
        requestRedraw();
        return;
    } else if (isButtonPressed(MARGIN_SMALL + adjustW + MARGIN_SMALL, adjustY, adjustW, adjustH)) {
        uint16_t newBPM;
        if (sharedBPM >= 235) {
            newBPM = 240;
        } else {
            newBPM = sharedBPM + 5;
        }
        slink_state.clock_engine.bpm = static_cast<float>(newBPM);
        setSharedBPM(newBPM);
        requestRedraw();
        return;
    }

    int swingY = adjustY + adjustH + SCALE_Y(12);
    if (isButtonPressed(MARGIN_SMALL, swingY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(40))) {
        slink_state.clock_engine.swing += 0.1f;
        if (slink_state.clock_engine.swing > 1.0f) {
            slink_state.clock_engine.swing = 0.0f;
        }
        requestRedraw();
        return;
    }

    int noteY = swingY + SCALE_Y(40) + SCALE_Y(10);
    int halfW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;
    if (isButtonPressed(MARGIN_SMALL, noteY, halfW, SCALE_Y(38))) {
        slink_state.clock_engine.note_len_min += 10;
        if (slink_state.clock_engine.note_len_min > 1000) {
            slink_state.clock_engine.note_len_min = 10;
        }
        // Validate range: if min > max, clamp max to min
        if (slink_state.clock_engine.note_len_min > slink_state.clock_engine.note_len_max) {
            slink_state.clock_engine.note_len_max = slink_state.clock_engine.note_len_min;
        }
        requestRedraw();
        return;
    } else if (isButtonPressed(MARGIN_SMALL + halfW + MARGIN_SMALL, noteY, halfW, SCALE_Y(38))) {
        slink_state.clock_engine.note_len_max += 50;
        if (slink_state.clock_engine.note_len_max > 2000) {
            slink_state.clock_engine.note_len_max = 50;
        }
        // Validate range: if max < min, clamp min to max
        if (slink_state.clock_engine.note_len_max < slink_state.clock_engine.note_len_min) {
            slink_state.clock_engine.note_len_min = slink_state.clock_engine.note_len_max;
        }
        requestRedraw();
        return;
    }

    int toggleY = noteY + SCALE_Y(38) + SCALE_Y(8);
    if (isButtonPressed(MARGIN_SMALL, toggleY, halfW, SCALE_Y(38))) {
        slink_state.clock_engine.note_len_x10 = !slink_state.clock_engine.note_len_x10;
        requestRedraw();
        return;
    } else if (isButtonPressed(MARGIN_SMALL + halfW + MARGIN_SMALL, toggleY, halfW, SCALE_Y(38))) {
        slink_state.clock_engine.sustain_mode = !slink_state.clock_engine.sustain_mode;
        requestRedraw();
        return;
    }

    int voiceY = toggleY + SCALE_Y(38) + SCALE_Y(8);
    if (isButtonPressed(MARGIN_SMALL, voiceY, DISPLAY_WIDTH - 2 * MARGIN_SMALL, SCALE_Y(38))) {
        slink_state.clock_engine.max_voices++;
        if (slink_state.clock_engine.max_voices > 16) {
            slink_state.clock_engine.max_voices = 1;
        }
        requestRedraw();
        return;
    }
}

void handleScaleTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    int btnH = SCALE_Y(42);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;

    if (isButtonPressed(MARGIN_SMALL, contentY, btnW, btnH)) {
        slink_state.scale_engine.arp_mode = !slink_state.scale_engine.arp_mode;
        requestRedraw();
        return;
    }

    if (!slink_state.scale_engine.arp_mode) {
        int rowY = contentY + btnH + SCALE_Y(12);
        if (isButtonPressed(MARGIN_SMALL, rowY, btnW, btnH)) {
            slink_state.scale_engine.root_note = (slink_state.scale_engine.root_note + 1) % 12;
            requestRedraw();
            return;
        }
        rowY += btnH + SCALE_Y(8);
        if (isButtonPressed(MARGIN_SMALL, rowY, btnW, btnH)) {
            slink_state.scale_engine.scale_index = (slink_state.scale_engine.scale_index + 1) % NUM_SCALES;
            requestRedraw();
            return;
        }
        rowY += btnH + SCALE_Y(8);
        if (isButtonPressed(MARGIN_SMALL, rowY, btnW, btnH)) {
            slink_state.scale_engine.color += 0.1f;
            if (slink_state.scale_engine.color > 1.0f) {
                slink_state.scale_engine.color = 0.0f;
            }
            requestRedraw();
            return;
        }
    }
}

void handleModTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    int blockHeight = SCALE_Y(38);
    int halfW = (DISPLAY_WIDTH - 3 * MARGIN_SMALL) / 2;

    for (int i = 0; i < 3; i++) {
        int baseY = contentY + i * (blockHeight * 2 + SCALE_Y(14));
        int secondY = baseY + blockHeight + SCALE_Y(6);
        Modulator* mod = &slink_state.mod_engine.mods[i];

        if (isButtonPressed(MARGIN_SMALL, baseY, halfW, blockHeight)) {
            mod->enabled = !mod->enabled;
            requestRedraw();
            return;
        }

        if (!mod->enabled) {
            continue;
        }

        if (isButtonPressed(MARGIN_SMALL + halfW + MARGIN_SMALL, baseY, halfW, blockHeight)) {
            mod->shape = (mod->shape + 1) % 5;
            requestRedraw();
            return;
        }

        if (isButtonPressed(MARGIN_SMALL, secondY, halfW, blockHeight)) {
            mod->rate_hz += 0.1f;
            if (mod->rate_hz > 10.0f) {
                mod->rate_hz = 0.1f;
            }
            requestRedraw();
            return;
        }

        if (isButtonPressed(MARGIN_SMALL + halfW + MARGIN_SMALL, secondY, halfW, blockHeight)) {
            mod->range += 0.1f;
            if (mod->range > 1.0f) {
                mod->range = 0.1f;
            }
            requestRedraw();
            return;
        }
    }
}

void handleSetupTab() {
    int contentY = HEADER_HEIGHT + getSlinkTabBarHeight() + SCALE_Y(10);
    int btnY = contentY + SCALE_Y(60);
    int btnH = SCALE_Y(44);
    int btnW = DISPLAY_WIDTH - 2 * MARGIN_SMALL;

    if (isButtonPressed(MARGIN_SMALL, btnY, btnW, btnH)) {
        // SAVE placeholder
    } else if (isButtonPressed(MARGIN_SMALL, btnY + btnH + SCALE_Y(10), btnW, btnH)) {
        // LOAD placeholder
    } else if (isButtonPressed(MARGIN_SMALL, btnY + 2 * (btnH + SCALE_Y(10)), btnW, btnH)) {
        initializeSlinkMode();
        requestRedraw();
    }
}
