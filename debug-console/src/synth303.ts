/**
 * TB-303 Bass Synthesizer
 * A simplified Web Audio API implementation inspired by the Roland TB-303
 * 
 * Features:
 * - Sawtooth and square oscillators
 * - Resonant lowpass filter with envelope modulation
 * - Accent and slide
 * - Monophonic with note priority
 */

export interface Synth303Params {
  waveform: 'saw' | 'square';
  cutoff: number;        // 0-1
  resonance: number;     // 0-1
  envMod: number;        // 0-1
  decay: number;         // 0-1
  accent: number;        // 0-1
  volume: number;        // 0-1
}

export class Synth303 {
  private audioContext: AudioContext;
  private masterGain: GainNode;
  
  // Oscillator and filter
  private osc: OscillatorNode | null = null;
  private oscGain: GainNode;
  private filter: BiquadFilterNode;
  private filterEnv: GainNode;
  
  // Current state
  private currentNote: number | null = null;
  private slideMode = false;
  private accentMode = false;
  
  // Parameters
  private params: Synth303Params = {
    waveform: 'saw',
    cutoff: 0.5,
    resonance: 0.7,
    envMod: 0.5,
    decay: 0.3,
    accent: 0.5,
    volume: 0.5,
  };
  
  constructor() {
    this.audioContext = new AudioContext();
    
    // Create audio graph
    this.masterGain = this.audioContext.createGain();
    this.masterGain.gain.value = this.params.volume;
    this.masterGain.connect(this.audioContext.destination);
    
    // Oscillator gain (for envelope)
    this.oscGain = this.audioContext.createGain();
    this.oscGain.gain.value = 0;
    
    // Filter
    this.filter = this.audioContext.createBiquadFilter();
    this.filter.type = 'lowpass';
    this.filter.Q.value = 1; // Will be modulated by resonance
    
    // Filter envelope modulation
    this.filterEnv = this.audioContext.createGain();
    this.filterEnv.gain.value = 0;
    
    // Connect graph: osc -> oscGain -> filter -> masterGain -> destination
    this.oscGain.connect(this.filter);
    this.filter.connect(this.masterGain);
    
    // Update filter from params
    this.updateFilter();
  }
  
  /**
   * Start or resume the audio context (required after user interaction)
   */
  async start(): Promise<void> {
    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }
  }
  
  /**
   * Handle MIDI Note On
   */
  noteOn(note: number, velocity: number = 100): void {
    const now = this.audioContext.currentTime;
    const frequency = this.midiNoteToFrequency(note);
    const normalizedVel = velocity / 127;
    const accentBoost = this.accentMode ? this.params.accent : 0;
    const effectiveVel = Math.min(1, normalizedVel + accentBoost * 0.5);
    
    // If we're already playing a note, handle slide
    if (this.currentNote !== null && this.osc) {
      if (this.slideMode) {
        // Slide to new frequency
        this.osc.frequency.cancelScheduledValues(now);
        this.osc.frequency.linearRampToValueAtTime(frequency, now + 0.1);
        this.currentNote = note;
        return;
      } else {
        // Stop previous note
        this.noteOff(this.currentNote);
      }
    }
    
    // Create new oscillator
    this.osc = this.audioContext.createOscillator();
    this.osc.type = this.params.waveform === 'square' ? 'square' : 'sawtooth';
    this.osc.frequency.value = frequency;
    this.osc.connect(this.oscGain);
    this.osc.start(now);
    
    // Amplitude envelope (simple AR)
    this.oscGain.gain.cancelScheduledValues(now);
    this.oscGain.gain.setValueAtTime(0, now);
    this.oscGain.gain.linearRampToValueAtTime(effectiveVel * 0.6, now + 0.001);
    
    // Filter envelope
    const baseCutoff = 50 + this.params.cutoff * 1950; // 50 - 2000 Hz
    const envAmount = this.params.envMod * 3000; // Up to +3000 Hz
    const accentEnvBoost = this.accentMode ? this.params.accent * 2000 : 0;
    const peakCutoff = Math.min(20000, baseCutoff + envAmount + accentEnvBoost);
    const decayTime = 0.05 + this.params.decay * 1.5; // 50ms to 1.55s
    
    this.filter.frequency.cancelScheduledValues(now);
    this.filter.frequency.setValueAtTime(peakCutoff, now);
    this.filter.frequency.exponentialRampToValueAtTime(
      Math.max(baseCutoff, 50),
      now + decayTime
    );
    
    this.currentNote = note;
  }
  
  /**
   * Handle MIDI Note Off
   */
  noteOff(note: number): void {
    if (this.currentNote !== note) {
      return;
    }
    
    const now = this.audioContext.currentTime;
    
    // Quick release
    this.oscGain.gain.cancelScheduledValues(now);
    this.oscGain.gain.linearRampToValueAtTime(0, now + 0.05);
    
    // Stop oscillator after release
    if (this.osc) {
      this.osc.stop(now + 0.06);
      this.osc = null;
    }
    
    this.currentNote = null;
  }
  
  /**
   * All notes off (panic)
   */
  allNotesOff(): void {
    const now = this.audioContext.currentTime;
    
    this.oscGain.gain.cancelScheduledValues(now);
    this.oscGain.gain.setValueAtTime(0, now);
    
    if (this.osc) {
      this.osc.stop(now + 0.01);
      this.osc = null;
    }
    
    this.currentNote = null;
  }
  
  /**
   * Handle MIDI CC
   */
  handleCC(cc: number, value: number): void {
    const normalized = value / 127;
    
    switch (cc) {
      case 7:  // Volume
        this.setParam('volume', normalized);
        break;
      case 71: // Resonance
        this.setParam('resonance', normalized);
        break;
      case 74: // Cutoff
        this.setParam('cutoff', normalized);
        break;
      case 75: // Env Mod
        this.setParam('envMod', normalized);
        break;
      case 76: // Decay
        this.setParam('decay', normalized);
        break;
      case 77: // Accent
        this.setParam('accent', normalized);
        break;
      case 65: // Slide (portamento)
        this.slideMode = value >= 64;
        break;
    }
  }
  
  /**
   * Set synth parameter
   */
  setParam(param: keyof Synth303Params, value: number | string): void {
    if (param === 'waveform') {
      this.params.waveform = value as 'saw' | 'square';
    } else {
      this.params[param] = Math.max(0, Math.min(1, value as number)) as never;
    }
    
    // Update audio parameters
    if (param === 'volume') {
      this.masterGain.gain.setValueAtTime(
        this.params.volume,
        this.audioContext.currentTime
      );
    } else if (param === 'cutoff' || param === 'resonance') {
      this.updateFilter();
    }
  }
  
  /**
   * Get current parameter value
   */
  getParam(param: keyof Synth303Params): number | string {
    return this.params[param];
  }
  
  /**
   * Toggle accent mode
   */
  setAccent(enabled: boolean): void {
    this.accentMode = enabled;
  }
  
  /**
   * Toggle slide mode
   */
  setSlide(enabled: boolean): void {
    this.slideMode = enabled;
  }
  
  /**
   * Update filter parameters
   */
  private updateFilter(): void {
    // Resonance: Q from 1 to 20
    const q = 1 + this.params.resonance * 19;
    this.filter.Q.setValueAtTime(q, this.audioContext.currentTime);
  }
  
  /**
   * Convert MIDI note to frequency
   */
  private midiNoteToFrequency(note: number): number {
    return 440 * Math.pow(2, (note - 69) / 12);
  }
  
  /**
   * Get audio context (for cleanup)
   */
  getContext(): AudioContext {
    return this.audioContext;
  }
}
