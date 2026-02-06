import type { LogEntry, LogSource } from './types';
import { formatTime, formatDelta, formatMidiMessage } from './types';
import { EventBus } from './eventBus';

export interface LoggerConfig {
  maxEntries: number;
  autoScroll: boolean;
  showDelta: boolean;
  filters: Set<LogSource>;
  searchTerm: string;
}

export class Logger {
  private entries: LogEntry[] = [];
  private config: LoggerConfig;
  private paused = false;
  private lastEntryTime = 0;
  private eventBus: EventBus;

  constructor(eventBus: EventBus, config: Partial<LoggerConfig> = {}) {
    this.eventBus = eventBus;
    this.config = {
      maxEntries: config.maxEntries || 10000,
      autoScroll: config.autoScroll !== false,
      showDelta: config.showDelta !== false,
      filters: config.filters || new Set(['BLE_IN', 'BLE_OUT', 'SERIAL_IN', 'UI_TO_MIDI']),
      searchTerm: config.searchTerm || '',
    };

    // Subscribe to event bus
    this.eventBus.subscribe((entry) => {
      if (!this.paused) {
        this.addEntry(entry);
      }
    });
  }

  private addEntry(entry: LogEntry): void {
    // Calculate delta time
    if (this.lastEntryTime > 0) {
      entry.tDeltaMs = entry.tRelMs - this.lastEntryTime;
    } else {
      entry.tDeltaMs = 0;
    }
    this.lastEntryTime = entry.tRelMs;

    // Add to ring buffer
    this.entries.push(entry);
    if (this.entries.length > this.config.maxEntries) {
      this.entries.shift();
    }
  }

  getEntries(): LogEntry[] {
    let filtered = this.entries;

    // Apply source filter
    filtered = filtered.filter(entry => this.config.filters.has(entry.source));

    // Apply search filter
    if (this.config.searchTerm) {
      const term = this.config.searchTerm.toLowerCase();
      filtered = filtered.filter(entry => entry.text.toLowerCase().includes(term));
    }

    return filtered;
  }

  clear(): void {
    this.entries = [];
    this.lastEntryTime = 0;
    this.eventBus.reset();
  }

  pause(): void {
    this.paused = true;
  }

  resume(): void {
    this.paused = false;
  }

  isPaused(): boolean {
    return this.paused;
  }

  updateConfig(updates: Partial<LoggerConfig>): void {
    this.config = { ...this.config, ...updates };
  }

  exportJson(): string {
    return JSON.stringify(this.entries, null, 2);
  }

  exportCsv(): string {
    const headers = ['Timestamp', 'Relative (ms)', 'Delta (ms)', 'Source', 'Type', 'Details'];
    const rows = this.entries.map(entry => {
      const details = entry.midi
        ? formatMidiMessage(entry.midi)
        : entry.serial?.line || '';
      
      return [
        formatTime(entry.tAbsMs),
        entry.tRelMs.toFixed(3),
        entry.tDeltaMs.toFixed(3),
        entry.source,
        entry.midi?.kind || 'SERIAL',
        details,
      ];
    });

    const csvRows = [headers, ...rows];
    return csvRows.map(row => row.map(cell => `"${cell}"`).join(',')).join('\n');
  }

  formatEntry(entry: LogEntry): string {
    let output = '';

    // Timestamp
    output += `<span class="timestamp">${formatTime(entry.tAbsMs)}</span>`;

    // Delta time (optional)
    if (this.config.showDelta && entry.tDeltaMs > 0) {
      output += `<span class="timestamp"> ${formatDelta(entry.tDeltaMs)}</span>`;
    }

    // Source
    const sourceLabel = this.formatSource(entry.source);
    output += `<span class="source">${sourceLabel}</span>`;

    // Message details
    if (entry.midi) {
      output += formatMidiMessage(entry.midi);
    } else if (entry.serial) {
      output += entry.serial.line;
    } else {
      output += entry.text;
    }

    return output;
  }

  private formatSource(source: LogSource): string {
    switch (source) {
      case 'UI_TO_MIDI':
        return 'UI→MIDI';
      case 'BLE_IN':
        return 'CYD←BLE';
      case 'BLE_OUT':
        return 'CYD→BLE';
      case 'SERIAL_IN':
        return 'CYD→SER';
      case 'SERIAL_OUT':
        return 'SER→CYD';
    }
  }

  getSourceClass(source: LogSource): string {
    switch (source) {
      case 'UI_TO_MIDI':
      case 'BLE_OUT':
        return 'midi-out';
      case 'BLE_IN':
        return 'midi-in';
      case 'SERIAL_IN':
      case 'SERIAL_OUT':
        return 'serial-in';
    }
  }
}
