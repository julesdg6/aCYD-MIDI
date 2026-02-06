// ============================================================
// Data Models
// ============================================================

export type LogSource =
  | "UI_TO_MIDI"
  | "BLE_IN"
  | "BLE_OUT"
  | "SERIAL_IN"
  | "SERIAL_OUT";

export type MidiKind =
  | "NOTE_ON"
  | "NOTE_OFF"
  | "CC"
  | "PC"
  | "PB"
  | "CLOCK"
  | "START"
  | "STOP"
  | "CONTINUE"
  | "SYSEX"
  | "OTHER";

export interface MidiData {
  kind: MidiKind;
  channel?: number;
  data: number[];
  note?: number;
  vel?: number;
  cc?: number;
  value?: number;
  program?: number;
  pitchBend?: number;
}

export interface SerialData {
  line: string;
}

export interface LogEntry {
  id: string;
  tAbsMs: number;      // Date.now()
  tRelMs: number;      // performance.now() - t0
  tDeltaMs: number;    // since previous entry
  source: LogSource;
  text: string;        // human-readable line
  midi?: MidiData;
  serial?: SerialData;
}

// ============================================================
// Settings
// ============================================================

export interface AppSettings {
  midiChannel: number;
  ccMapping: {
    cutoff: number;
    resonance: number;
    envMod: number;
    decay: number;
    accent: number;
    volume: number;
  };
  accentMode: "velocity" | "cc";
  accentVelocity: number;
  accentCC: number;
  accentValue: number;
  slideMode: "legato" | "cc";
  slideCC: number;
  serialBaudRate: number;
  logMaxEntries: number;
  showRawBytes: boolean;
  showDeltaTimes: boolean;
}

export const DEFAULT_SETTINGS: AppSettings = {
  midiChannel: 1,
  ccMapping: {
    cutoff: 74,
    resonance: 71,
    envMod: 75,
    decay: 76,
    accent: 77,
    volume: 7,
  },
  accentMode: "velocity",
  accentVelocity: 120,
  accentCC: 77,
  accentValue: 127,
  slideMode: "legato",
  slideCC: 65,
  serialBaudRate: 115200,
  logMaxEntries: 10000,
  showRawBytes: false,
  showDeltaTimes: true,
};

// ============================================================
// MIDI Utilities
// ============================================================

export function midiNoteToName(note: number): string {
  const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
  const octave = Math.floor(note / 12) - 1;
  const noteName = names[note % 12];
  return `${noteName}${octave}`;
}

export function parseMidiMessage(data: number[]): MidiData {
  if (data.length === 0) {
    return { kind: "OTHER", data };
  }

  const status = data[0];
  const messageType = status & 0xf0;
  const channel = (status & 0x0f) + 1;

  switch (messageType) {
    case 0x90: // Note On
      return {
        kind: data[2] > 0 ? "NOTE_ON" : "NOTE_OFF",
        channel,
        data,
        note: data[1],
        vel: data[2],
      };

    case 0x80: // Note Off
      return {
        kind: "NOTE_OFF",
        channel,
        data,
        note: data[1],
        vel: data[2],
      };

    case 0xb0: // Control Change
      return {
        kind: "CC",
        channel,
        data,
        cc: data[1],
        value: data[2],
      };

    case 0xc0: // Program Change
      return {
        kind: "PC",
        channel,
        data,
        program: data[1],
      };

    case 0xe0: // Pitch Bend
      const value14bit = data[1] | (data[2] << 7);
      return {
        kind: "PB",
        channel,
        data,
        pitchBend: value14bit - 8192,
      };

    default:
      // System messages
      if (status === 0xf8) return { kind: "CLOCK", data };
      if (status === 0xfa) return { kind: "START", data };
      if (status === 0xfc) return { kind: "STOP", data };
      if (status === 0xfb) return { kind: "CONTINUE", data };
      if (status === 0xf0) return { kind: "SYSEX", data };
      
      return { kind: "OTHER", data };
  }
}

export function formatMidiMessage(midi: MidiData): string {
  switch (midi.kind) {
    case "NOTE_ON":
      return `NOTE_ON  ch${midi.channel} note=${midi.note} (${midiNoteToName(midi.note!)}) vel=${midi.vel}`;
    case "NOTE_OFF":
      return `NOTE_OFF ch${midi.channel} note=${midi.note} (${midiNoteToName(midi.note!)}) vel=${midi.vel}`;
    case "CC":
      return `CC       ch${midi.channel} cc=${midi.cc} val=${midi.value}`;
    case "PC":
      return `PC       ch${midi.channel} program=${midi.program}`;
    case "PB":
      return `PB       ch${midi.channel} value=${midi.pitchBend}`;
    case "CLOCK":
      return "CLOCK";
    case "START":
      return "START";
    case "STOP":
      return "STOP";
    case "CONTINUE":
      return "CONTINUE";
    case "SYSEX":
      return `SYSEX    len=${midi.data.length}`;
    default:
      return `OTHER    ${midi.data.map(b => b.toString(16).padStart(2, '0')).join(' ')}`;
  }
}

export function formatTime(ms: number): string {
  const date = new Date(ms);
  const hours = date.getHours().toString().padStart(2, '0');
  const minutes = date.getMinutes().toString().padStart(2, '0');
  const seconds = date.getSeconds().toString().padStart(2, '0');
  const millis = date.getMilliseconds().toString().padStart(3, '0');
  return `${hours}:${minutes}:${seconds}.${millis}`;
}

export function formatDelta(ms: number): string {
  if (ms < 1000) {
    return `+${ms.toFixed(0)}ms`;
  } else {
    return `+${(ms / 1000).toFixed(2)}s`;
  }
}
