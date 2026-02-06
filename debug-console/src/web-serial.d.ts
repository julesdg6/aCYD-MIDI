// Web Serial API type definitions
// https://wicg.github.io/serial/

interface SerialPort {
  readonly readable: ReadableStream<Uint8Array>;
  readonly writable: WritableStream<Uint8Array>;
  
  open(options: SerialOptions): Promise<void>;
  close(): Promise<void>;
  
  getInfo(): SerialPortInfo;
}

interface SerialOptions {
  baudRate: number;
  dataBits?: 7 | 8;
  stopBits?: 1 | 2;
  parity?: 'none' | 'even' | 'odd';
  bufferSize?: number;
  flowControl?: 'none' | 'hardware';
}

interface SerialPortInfo {
  usbVendorId?: number;
  usbProductId?: number;
}

interface SerialPortRequestOptions {
  filters?: SerialPortFilter[];
}

interface SerialPortFilter {
  usbVendorId?: number;
  usbProductId?: number;
}

interface Serial extends EventTarget {
  getPorts(): Promise<SerialPort[]>;
  requestPort(options?: SerialPortRequestOptions): Promise<SerialPort>;
  
  addEventListener(type: 'connect' | 'disconnect', listener: (this: this, ev: Event) => any): void;
  removeEventListener(type: 'connect' | 'disconnect', listener: (this: this, ev: Event) => any): void;
}

interface Navigator {
  readonly serial: Serial;
}
