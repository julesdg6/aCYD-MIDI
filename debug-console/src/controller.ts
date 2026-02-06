import { EventBus } from './eventBus';
import { BleMidiConnection } from './bleMidi';
import { SerialConnection } from './serial';
import { Logger } from './logger';
import type { AppSettings } from './types';
import { DEFAULT_SETTINGS } from './types';

export class MidiController {
  private bleMidi: BleMidiConnection;
  private serial: SerialConnection;
  private logger: Logger;
  private eventBus: EventBus;
  private settings: AppSettings;
  
  // State
  private currentOctave = 3;
  private gateEnabled = false;
  private accentEnabled = false;
  private slideEnabled = false;
  private activeNotes = new Set<number>();
  private knobValues = new Map<string, number>();

  constructor(eventBus: EventBus) {
    this.eventBus = eventBus;
    this.settings = { ...DEFAULT_SETTINGS };
    this.bleMidi = new BleMidiConnection(eventBus);
    this.serial = new SerialConnection(eventBus, this.settings.serialBaudRate);
    this.logger = new Logger(eventBus, {
      maxEntries: this.settings.logMaxEntries,
      showDelta: this.settings.showDeltaTimes,
    });

    // Initialize knob values
    Object.entries(this.settings.ccMapping).forEach(([name, cc]) => {
      this.knobValues.set(name, 64); // Default mid value
    });
  }

  // Connection management
  async connectBle(): Promise<void> {
    await this.bleMidi.connect();
  }

  async disconnectBle(): Promise<void> {
    await this.bleMidi.disconnect();
  }

  async connectSerial(): Promise<void> {
    await this.serial.connect();
  }

  async disconnectSerial(): Promise<void> {
    await this.serial.disconnect();
  }

  isBleConnected(): boolean {
    return this.bleMidi.connected;
  }

  isSerialConnected(): boolean {
    return this.serial.connected;
  }

  // MIDI operations
  async sendNoteOn(note: number, velocity?: number): Promise<void> {
    const vel = velocity ?? (this.accentEnabled && this.settings.accentMode === 'velocity'
      ? this.settings.accentVelocity
      : 100);
    
    const status = 0x90 | (this.settings.midiChannel - 1);
    await this.bleMidi.sendMidi(status, note, vel);
    
    this.activeNotes.add(note);

    // Log UI action
    this.eventBus.emit({
      source: 'UI_TO_MIDI',
      text: `UI→MIDI NOTE_ON note=${note} vel=${vel}`,
      tDeltaMs: 0,
      midi: {
        kind: 'NOTE_ON',
        channel: this.settings.midiChannel,
        data: [status, note, vel],
        note,
        vel,
      },
    });
  }

  async sendNoteOff(note: number): Promise<void> {
    const status = 0x80 | (this.settings.midiChannel - 1);
    await this.bleMidi.sendMidi(status, note, 0);
    
    this.activeNotes.delete(note);

    // Log UI action
    this.eventBus.emit({
      source: 'UI_TO_MIDI',
      text: `UI→MIDI NOTE_OFF note=${note}`,
      tDeltaMs: 0,
      midi: {
        kind: 'NOTE_OFF',
        channel: this.settings.midiChannel,
        data: [status, note, 0],
        note,
        vel: 0,
      },
    });
  }

  async sendCC(cc: number, value: number): Promise<void> {
    const status = 0xb0 | (this.settings.midiChannel - 1);
    await this.bleMidi.sendMidi(status, cc, value);

    // Log UI action
    this.eventBus.emit({
      source: 'UI_TO_MIDI',
      text: `UI→MIDI CC cc=${cc} val=${value}`,
      tDeltaMs: 0,
      midi: {
        kind: 'CC',
        channel: this.settings.midiChannel,
        data: [status, cc, value],
        cc,
        value,
      },
    });
  }

  async allNotesOff(): Promise<void> {
    // Send All Notes Off (CC 123)
    const status = 0xb0 | (this.settings.midiChannel - 1);
    await this.bleMidi.sendMidi(status, 123, 0);
    
    // Send All Sound Off (CC 120)
    await this.bleMidi.sendMidi(status, 120, 0);

    // Also send note offs for all active notes
    for (const note of this.activeNotes) {
      await this.sendNoteOff(note);
    }

    this.activeNotes.clear();
  }

  async sendTestBurst(): Promise<void> {
    // Send a deterministic sequence for testing
    const testSequence = [
      // Note sequence
      { type: 'note', note: 60, vel: 100 },
      { type: 'delay', ms: 100 },
      { type: 'note', note: 64, vel: 100 },
      { type: 'delay', ms: 100 },
      { type: 'note', note: 67, vel: 100 },
      { type: 'delay', ms: 100 },
      // CC sweep
      { type: 'cc', cc: 74, value: 0 },
      { type: 'delay', ms: 50 },
      { type: 'cc', cc: 74, value: 64 },
      { type: 'delay', ms: 50 },
      { type: 'cc', cc: 74, value: 127 },
      { type: 'delay', ms: 100 },
      // Note offs
      { type: 'noteOff', note: 60 },
      { type: 'noteOff', note: 64 },
      { type: 'noteOff', note: 67 },
    ];

    for (const item of testSequence) {
      if (item.type === 'note') {
        await this.sendNoteOn((item as any).note, (item as any).vel);
      } else if (item.type === 'noteOff') {
        await this.sendNoteOff((item as any).note);
      } else if (item.type === 'cc') {
        await this.sendCC((item as any).cc, (item as any).value);
      } else if (item.type === 'delay') {
        await new Promise(resolve => setTimeout(resolve, (item as any).ms));
      }
    }
  }

  // State management
  setOctave(octave: number): void {
    this.currentOctave = Math.max(0, Math.min(8, octave));
  }

  getOctave(): number {
    return this.currentOctave;
  }

  toggleGate(): void {
    this.gateEnabled = !this.gateEnabled;
  }

  isGateEnabled(): boolean {
    return this.gateEnabled;
  }

  toggleAccent(): void {
    this.accentEnabled = !this.accentEnabled;
  }

  isAccentEnabled(): boolean {
    return this.accentEnabled;
  }

  toggleSlide(): void {
    this.slideEnabled = !this.slideEnabled;
  }

  isSlideEnabled(): boolean {
    return this.slideEnabled;
  }

  setKnobValue(name: string, value: number): void {
    this.knobValues.set(name, value);
  }

  getKnobValue(name: string): number {
    return this.knobValues.get(name) || 64;
  }

  // Logger access
  getLogger(): Logger {
    return this.logger;
  }

  // Settings
  getSettings(): AppSettings {
    return { ...this.settings };
  }

  updateSettings(updates: Partial<AppSettings>): void {
    this.settings = { ...this.settings, ...updates };
    
    // Update serial baud rate if changed
    if (updates.serialBaudRate !== undefined) {
      this.serial.setBaudRate(updates.serialBaudRate);
    }

    // Update logger config if needed
    if (updates.logMaxEntries !== undefined || updates.showDeltaTimes !== undefined) {
      this.logger.updateConfig({
        maxEntries: this.settings.logMaxEntries,
        showDelta: this.settings.showDeltaTimes,
      });
    }
  }
}
