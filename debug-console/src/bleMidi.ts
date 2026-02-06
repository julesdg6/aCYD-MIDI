import { EventBus } from './eventBus';
import { parseMidiMessage } from './types';

// BLE MIDI Service UUIDs (standard BLE MIDI spec)
const BLE_MIDI_SERVICE = '03b80e5a-ede8-4b33-a751-6ce34ec4c700';
const BLE_MIDI_CHARACTERISTIC = '7772e5db-3868-4112-a1a9-f2669d106bf3';

export class BleMidiConnection {
  private device: BluetoothDevice | null = null;
  private characteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private eventBus: EventBus;
  public connected = false;

  constructor(eventBus: EventBus) {
    this.eventBus = eventBus;
  }

  async connect(): Promise<void> {
    try {
      // Request device
      this.device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [BLE_MIDI_SERVICE] }],
      });

      if (!this.device.gatt) {
        throw new Error('GATT not available');
      }

      // Connect to GATT server
      const server = await this.device.gatt.connect();

      // Get MIDI service
      const service = await server.getPrimaryService(BLE_MIDI_SERVICE);

      // Get MIDI characteristic
      this.characteristic = await service.getCharacteristic(BLE_MIDI_CHARACTERISTIC);

      // Subscribe to notifications
      await this.characteristic.startNotifications();
      this.characteristic.addEventListener('characteristicvaluechanged', this.handleNotification.bind(this));

      // Listen for disconnection
      this.device.addEventListener('gattserverdisconnected', this.handleDisconnect.bind(this));

      this.connected = true;
      console.log('BLE MIDI connected');
    } catch (error) {
      console.error('BLE MIDI connection failed:', error);
      throw error;
    }
  }

  async disconnect(): Promise<void> {
    if (this.device?.gatt?.connected) {
      this.device.gatt.disconnect();
    }
    this.connected = false;
  }

  private handleNotification(event: Event): void {
    const target = event.target as BluetoothRemoteGATTCharacteristic;
    const value = target.value;
    if (!value) return;

    // Parse BLE MIDI packet
    // Format: [header][timestamp][midi bytes...]
    const data = new Uint8Array(value.buffer);
    
    // Skip header byte and timestamp byte, extract MIDI message
    const midiBytes: number[] = [];
    for (let i = 2; i < data.length; i++) {
      // Skip timestamp bytes (high bit set)
      if ((data[i] & 0x80) === 0 || i === 2) {
        midiBytes.push(data[i]);
      }
    }

    if (midiBytes.length > 0) {
      const midi = parseMidiMessage(midiBytes);
      
      this.eventBus.emit({
        source: 'BLE_IN',
        text: `BLE←MIDI ${this.formatMidiBytes(midiBytes)}`,
        tDeltaMs: 0, // Will be calculated by logger
        midi,
      });
    }
  }

  private handleDisconnect(): void {
    this.connected = false;
    this.characteristic = null;
    console.log('BLE MIDI disconnected');
  }

  async sendMidi(status: number, data1: number, data2?: number): Promise<void> {
    if (!this.characteristic || !this.connected) {
      throw new Error('BLE MIDI not connected');
    }

    // Build BLE MIDI packet
    // Format: [header][timestamp][status][data1][data2?]
    const header = 0x80;
    const timestamp = 0x80; // Simplified timestamp
    
    const bytes = data2 !== undefined
      ? [header, timestamp, status, data1, data2]
      : [header, timestamp, status, data1];

    await this.characteristic.writeValue(new Uint8Array(bytes));

    // Log outgoing MIDI
    const midiBytes = data2 !== undefined ? [status, data1, data2] : [status, data1];
    const midi = parseMidiMessage(midiBytes);
    
    this.eventBus.emit({
      source: 'BLE_OUT',
      text: `BLE→MIDI ${this.formatMidiBytes(midiBytes)}`,
      tDeltaMs: 0,
      midi,
    });
  }

  private formatMidiBytes(bytes: number[]): string {
    return bytes.map(b => b.toString(16).padStart(2, '0')).join(' ');
  }

  isSupported(): boolean {
    return 'bluetooth' in navigator;
  }
}
