import { EventBus } from './eventBus';

export class SerialConnection {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<string> | null = null;
  private eventBus: EventBus;
  private baudRate: number;
  public connected = false;

  constructor(eventBus: EventBus, baudRate = 115200) {
    this.eventBus = eventBus;
    this.baudRate = baudRate;
  }

  async connect(): Promise<void> {
    try {
      // Request serial port
      this.port = await navigator.serial.requestPort();

      // Open port
      await this.port.open({ baudRate: this.baudRate });

      this.connected = true;

      // Start reading
      this.startReading();

      console.log('Serial connected');
    } catch (error) {
      console.error('Serial connection failed:', error);
      throw error;
    }
  }

  async disconnect(): Promise<void> {
    if (this.reader) {
      await this.reader.cancel();
      this.reader = null;
    }

    if (this.port) {
      await this.port.close();
      this.port = null;
    }

    this.connected = false;
  }

  private async startReading(): Promise<void> {
    if (!this.port || !this.port.readable) return;

    const textDecoder = new TextDecoderStream();
    const readableStreamClosed = this.port.readable.pipeTo(textDecoder.writable as any);
    this.reader = textDecoder.readable.getReader();

    let buffer = '';

    try {
      while (true) {
        const { value, done } = await this.reader.read();
        if (done) {
          break;
        }

        buffer += value;
        
        // Process complete lines
        const lines = buffer.split('\n');
        buffer = lines.pop() || ''; // Keep incomplete line in buffer

        for (const line of lines) {
          const trimmed = line.trim();
          if (trimmed) {
            this.eventBus.emit({
              source: 'SERIAL_IN',
              text: `SERIAL  ${trimmed}`,
              tDeltaMs: 0,
              serial: { line: trimmed },
            });
          }
        }
      }
    } catch (error) {
      console.error('Serial read error:', error);
    } finally {
      this.reader.releaseLock();
      await readableStreamClosed.catch(() => { /* ignore */ });
    }
  }

  async sendCommand(command: string): Promise<void> {
    if (!this.port || !this.port.writable) {
      throw new Error('Serial not connected');
    }

    const writer = this.port.writable.getWriter();
    const encoder = new TextEncoder();
    await writer.write(encoder.encode(command + '\n'));
    writer.releaseLock();

    // Log outgoing command
    this.eventBus.emit({
      source: 'SERIAL_OUT',
      text: `SERIAL→ ${command}`,
      tDeltaMs: 0,
      serial: { line: command },
    });
  }

  isSupported(): boolean {
    return 'serial' in navigator;
  }

  setBaudRate(baudRate: number): void {
    this.baudRate = baudRate;
  }
}
