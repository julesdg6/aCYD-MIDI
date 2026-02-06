import type { LogEntry } from './types';

// ============================================================
// EventBus - Central event system for all log entries
// ============================================================

type EventCallback = (entry: LogEntry) => void;

export class EventBus {
  private listeners: EventCallback[] = [];
  private t0: number;

  constructor() {
    this.t0 = performance.now();
  }

  subscribe(callback: EventCallback): () => void {
    this.listeners.push(callback);
    return () => {
      const index = this.listeners.indexOf(callback);
      if (index > -1) {
        this.listeners.splice(index, 1);
      }
    };
  }

  emit(entry: Omit<LogEntry, 'id' | 'tAbsMs' | 'tRelMs'>): void {
    const fullEntry: LogEntry = {
      id: this.generateId(),
      tAbsMs: Date.now(),
      tRelMs: performance.now() - this.t0,
      ...entry,
    };

    this.listeners.forEach(callback => callback(fullEntry));
  }

  private generateId(): string {
    return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  }

  reset(): void {
    this.t0 = performance.now();
  }
}
