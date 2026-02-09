import './style.css';
import { EventBus } from './eventBus';
import { MidiController } from './controller';
import { Synth303 } from './synth303';
import { formatTime, formatDelta, formatMidiMessage } from './types';

// Initialize app
const eventBus = new EventBus();
const controller = new MidiController(eventBus);
const logger = controller.getLogger();
const synth = new Synth303();

// ============================================================
// UI Elements
// ============================================================

const $ = (id: string) => document.getElementById(id)!;

// Connection buttons
const connectBleBtn = $('connect-ble') as HTMLButtonElement;
const connectSerialBtn = $('connect-serial') as HTMLButtonElement;
const bleStatus = $('ble-status');
const serialStatus = $('serial-status');
const bleLabel = $('ble-label');
const serialLabel = $('serial-label');

// Synth visualizer
const synthCanvas = $('synth-canvas') as HTMLCanvasElement;
const cutoffDisplay = $('cutoff-display');
const resonanceDisplay = $('resonance-display');
const envmodDisplay = $('envmod-display');
const decayDisplay = $('decay-display');
const accentDisplay = $('accent-display');
const volumeDisplay = $('volume-display');

// Log controls - Unified
const searchBox = $('search-box') as HTMLInputElement;
const filterMidi = $('filter-midi') as HTMLInputElement;
const filterSerial = $('filter-serial') as HTMLInputElement;
const autoScrollCheck = $('auto-scroll') as HTMLInputElement;
const showDeltaCheck = $('show-delta') as HTMLInputElement;
const pauseLogBtn = $('pause-log');
const clearLogBtn = $('clear-log');
const saveLogBtn = $('save-log');
const logViewer = $('log-viewer');

// ============================================================
// Synth Visualizer Setup
// ============================================================

function setupSynthVisualizer(): void {
  const ctx = synthCanvas.getContext('2d')!;
  const width = synthCanvas.width = synthCanvas.offsetWidth;
  const height = synthCanvas.height = synthCanvas.offsetHeight;
  
  let animationId: number;
  
  function draw() {
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, width, height);
    
    // Draw simple waveform visualization
    ctx.strokeStyle = '#00d4ff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    const centerY = height / 2;
    const time = performance.now() / 1000;
    
    for (let x = 0; x < width; x++) {
      const t = (x / width) * Math.PI * 4 + time * 2;
      const y = centerY + Math.sin(t) * (height * 0.3);
      
      if (x === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    
    ctx.stroke();
    
    // Continue animation
    animationId = requestAnimationFrame(draw);
  }
  
  draw();
}

// ============================================================
// MIDI Event Handler - Connect to Synth
// ============================================================

eventBus.subscribe((entry) => {
  // Play incoming MIDI on synth
  if (entry.midi && (entry.source === 'BLE_IN' || entry.source === 'UI_TO_MIDI')) {
    const midi = entry.midi;
    
    switch (midi.kind) {
      case 'NOTE_ON':
        if (midi.note !== undefined && midi.vel !== undefined) {
          synth.noteOn(midi.note, midi.vel);
        }
        break;
        
      case 'NOTE_OFF':
        if (midi.note !== undefined) {
          synth.noteOff(midi.note);
        }
        break;
        
      case 'CC':
        if (midi.cc !== undefined && midi.value !== undefined) {
          synth.handleCC(midi.cc, midi.value);
          updateSynthDisplay();
        }
        break;
    }
  }
});

function updateSynthDisplay(): void {
  cutoffDisplay.textContent = Math.round((synth.getParam('cutoff') as number) * 100) + '%';
  resonanceDisplay.textContent = Math.round((synth.getParam('resonance') as number) * 100) + '%';
  envmodDisplay.textContent = Math.round((synth.getParam('envMod') as number) * 100) + '%';
  decayDisplay.textContent = Math.round((synth.getParam('decay') as number) * 100) + '%';
  accentDisplay.textContent = Math.round((synth.getParam('accent') as number) * 100) + '%';
  volumeDisplay.textContent = Math.round((synth.getParam('volume') as number) * 100) + '%';
}

// ============================================================
// Event Handlers
// ============================================================

connectBleBtn.addEventListener('click', async () => {
  try {
    await controller.connectBle();
    await synth.start(); // Resume audio context on user interaction
    updateConnectionStatus();
  } catch (error) {
    alert(`BLE connection failed: ${error}`);
  }
});

connectSerialBtn.addEventListener('click', async () => {
  try {
    await controller.connectSerial();
    updateConnectionStatus();
  } catch (error) {
    alert(`Serial connection failed: ${error}`);
  }
});

// Log controls - Unified
searchBox.addEventListener('input', () => {
  renderLog();
});

filterMidi.addEventListener('change', () => {
  renderLog();
});

filterSerial.addEventListener('change', () => {
  renderLog();
});

autoScrollCheck.addEventListener('change', () => {
  // Auto-scroll is handled per render
});

showDeltaCheck.addEventListener('change', () => {
  renderLog();
});

pauseLogBtn.addEventListener('click', () => {
  if (logger.isPaused()) {
    logger.resume();
    pauseLogBtn.textContent = 'Pause';
  } else {
    logger.pause();
    pauseLogBtn.textContent = 'Resume';
  }
});

clearLogBtn.addEventListener('click', () => {
  logger.clear();
  renderLog();
});

saveLogBtn.addEventListener('click', () => {
  const content = generateLogFile();
  downloadFile('cyd-debug-log.txt', content);
});

// ============================================================
// Rendering
// ============================================================

function updateConnectionStatus(): void {
  const bleConnected = controller.isBleConnected();
  const serialConnected = controller.isSerialConnected();
  
  bleStatus.classList.toggle('connected', bleConnected);
  bleLabel.textContent = bleConnected ? 'BLE: Connected' : 'BLE: Disconnected';
  
  serialStatus.classList.toggle('connected', serialConnected);
  serialLabel.textContent = serialConnected ? 'Serial: Connected' : 'Serial: Disconnected';
  
  connectBleBtn.textContent = bleConnected ? 'Disconnect BLE' : 'Connect BLE MIDI';
  connectSerialBtn.textContent = serialConnected ? 'Disconnect Serial' : 'Connect Serial';
}

let renderScheduled = false;

function renderLog(): void {
  if (renderScheduled) return;
  renderScheduled = true;
  
  requestAnimationFrame(() => {
    const allEntries = logger.getEntries();
    const searchTerm = searchBox.value.toLowerCase();
    const showMidi = filterMidi.checked;
    const showSerial = filterSerial.checked;
    
    // Filter entries
    const entries = allEntries.filter(entry => {
      // Filter by type
      const isMidi = entry.source === 'BLE_IN' || entry.source === 'BLE_OUT' || entry.source === 'UI_TO_MIDI';
      const isSerial = entry.source === 'SERIAL_IN' || entry.source === 'SERIAL_OUT';
      
      if (isMidi && !showMidi) return false;
      if (isSerial && !showSerial) return false;
      
      // Filter by search term
      if (searchTerm && !entry.text.toLowerCase().includes(searchTerm)) return false;
      
      return true;
    });
    
    const autoScroll = autoScrollCheck.checked;
    const wasAtBottom = logViewer.scrollHeight - logViewer.scrollTop <= logViewer.clientHeight + 50;
    
    // Render entries with left/right alignment
    logViewer.innerHTML = entries
      .map(entry => {
        const isMidi = entry.source === 'BLE_IN' || entry.source === 'BLE_OUT' || entry.source === 'UI_TO_MIDI';
        const cssClass = isMidi ? 'midi' : 'serial';
        const formatted = formatLogEntry(entry);
        return `<div class="log-entry ${cssClass}"><div class="content">${formatted}</div></div>`;
      })
      .join('');
    
    // Auto-scroll if enabled and was at bottom
    if (autoScroll && wasAtBottom) {
      logViewer.scrollTop = logViewer.scrollHeight;
    }
    
    renderScheduled = false;
  });
}

function formatLogEntry(entry: any): string {
  let output = '';
  
  // Timestamp
  output += `<span class="timestamp">${formatTime(entry.tAbsMs)}</span>`;
  
  // Delta time (optional)
  if (showDeltaCheck.checked && entry.tDeltaMs > 0) {
    output += `<span class="timestamp"> ${formatDelta(entry.tDeltaMs)}</span>`;
  }
  
  // Source
  const sourceLabel = formatSource(entry.source);
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

function formatSource(source: string): string {
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
    default:
      return source;
  }
}

function generateLogFile(): string {
  const allEntries = logger.getEntries();
  
  let output = '';
  
  for (const entry of allEntries) {
    const timestamp = formatTime(entry.tAbsMs);
    const isMidi = entry.source === 'BLE_IN' || entry.source === 'BLE_OUT' || entry.source === 'UI_TO_MIDI';
    const prefix = isMidi ? 'MIDI' : 'TTY ';
    
    let message = '';
    if (entry.midi) {
      message = formatMidiMessage(entry.midi);
    } else if (entry.serial) {
      message = entry.serial.line;
    } else {
      message = entry.text;
    }
    
    output += `${timestamp} ${prefix} ${message}\n`;
  }
  
  return output;
}

// Subscribe to log updates
eventBus.subscribe(() => {
  renderLog();
});

function downloadFile(filename: string, content: string): void {
  const blob = new Blob([content], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

// ============================================================
// Initialization
// ============================================================

setupSynthVisualizer();
updateConnectionStatus();
updateSynthDisplay();

console.log('aCYD-MIDI Web Debug Console initialized');
