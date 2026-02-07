import './style.css';
import { EventBus } from './eventBus';
import { MidiController } from './controller';

// Initialize app
const eventBus = new EventBus();
const controller = new MidiController(eventBus);
const logger = controller.getLogger();

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

// Keyboard
const keyboard = $('keyboard');
const octaveDisplay = $('octave-display');
const octDownBtn = $('oct-down');
const octUpBtn = $('oct-up');

// Toggles
const gateToggle = $('gate-toggle');
const accentToggle = $('accent-toggle');
const slideToggle = $('slide-toggle');

// Controls
const panicBtn = $('panic');
const testBurstBtn = $('test-burst');

// Log controls - Serial
const searchBoxSerial = $('search-box-serial') as HTMLInputElement;
const autoScrollCheckSerial = $('auto-scroll-serial') as HTMLInputElement;
const showDeltaCheckSerial = $('show-delta-serial') as HTMLInputElement;
const pauseLogBtnSerial = $('pause-log-serial');
const clearLogBtnSerial = $('clear-log-serial');
const logViewerSerial = $('log-viewer-serial');

// Log controls - MIDI
const searchBoxMidi = $('search-box-midi') as HTMLInputElement;
const filterMidiIn = $('filter-midi-in') as HTMLInputElement;
const filterMidiOut = $('filter-midi-out') as HTMLInputElement;
const autoScrollCheckMidi = $('auto-scroll-midi') as HTMLInputElement;
const showDeltaCheckMidi = $('show-delta-midi') as HTMLInputElement;
const pauseLogBtnMidi = $('pause-log-midi');
const clearLogBtnMidi = $('clear-log-midi');
const exportJsonBtn = $('export-json');
const exportCsvBtn = $('export-csv');
const logViewerMidi = $('log-viewer-midi');

// Knobs container
const knobsGrid = $('knobs-grid');

// ============================================================
// Keyboard Setup
// ============================================================

const whiteKeys = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
const blackKeyPositions = [1, 2, 4, 5, 6]; // After C, D, F, G, A

function createKeyboard(): void {
  keyboard.innerHTML = '';
  
  // Create one octave of keys
  for (let i = 0; i < 7; i++) {
    const note = i * 2 + (i >= 3 ? -1 : 0); // C=0, D=2, E=4, F=5, G=7, A=9, B=11
    
    // White key
    const whiteKey = document.createElement('div');
    whiteKey.className = 'key';
    whiteKey.dataset.note = note.toString();
    keyboard.appendChild(whiteKey);
    
    // Black key (if applicable)
    if (blackKeyPositions.includes(i)) {
      const blackKey = document.createElement('div');
      blackKey.className = 'key black';
      blackKey.dataset.note = (note + 1).toString();
      keyboard.appendChild(blackKey);
    }
  }
  
  // Add event listeners
  keyboard.querySelectorAll('.key').forEach(key => {
    const element = key as HTMLElement;
    
    element.addEventListener('mousedown', () => {
      const note = parseInt(element.dataset.note || '0');
      const absoluteNote = note + (controller.getOctave() * 12) + 24; // C0 = MIDI 24
      controller.sendNoteOn(absoluteNote);
      element.classList.add('active');
    });
    
    element.addEventListener('mouseup', () => {
      const note = parseInt(element.dataset.note || '0');
      const absoluteNote = note + (controller.getOctave() * 12) + 24;
      if (!controller.isGateEnabled()) {
        controller.sendNoteOff(absoluteNote);
      }
      element.classList.remove('active');
    });
    
    element.addEventListener('mouseleave', () => {
      element.classList.remove('active');
    });
  });
}

// ============================================================
// Knobs Setup
// ============================================================

const knobConfigs = [
  { name: 'cutoff', label: 'Cutoff', cc: 74 },
  { name: 'resonance', label: 'Resonance', cc: 71 },
  { name: 'envMod', label: 'Env Mod', cc: 75 },
  { name: 'decay', label: 'Decay', cc: 76 },
  { name: 'accent', label: 'Accent', cc: 77 },
  { name: 'volume', label: 'Volume', cc: 7 },
];

function createKnobs(): void {
  knobsGrid.innerHTML = '';
  
  knobConfigs.forEach(config => {
    const container = document.createElement('div');
    container.className = 'knob-container';
    
    const knob = document.createElement('div');
    knob.className = 'knob';
    knob.dataset.name = config.name;
    
    const label = document.createElement('div');
    label.className = 'knob-label';
    label.textContent = config.label;
    
    const value = document.createElement('div');
    value.className = 'knob-value';
    value.id = `knob-value-${config.name}`;
    value.textContent = '64';
    
    container.appendChild(knob);
    container.appendChild(label);
    container.appendChild(value);
    knobsGrid.appendChild(container);
    
    // Knob interaction
    let isDragging = false;
    let startY = 0;
    let startValue = 64;
    
    knob.addEventListener('mousedown', (e) => {
      isDragging = true;
      startY = e.clientY;
      startValue = controller.getKnobValue(config.name);
      e.preventDefault();
    });
    
    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      
      const deltaY = startY - e.clientY; // Inverted
      const newValue = Math.max(0, Math.min(127, startValue + Math.floor(deltaY / 2)));
      
      controller.setKnobValue(config.name, newValue);
      value.textContent = newValue.toString();
      
      // Update knob rotation
      const rotation = (newValue / 127) * 270 - 135; // -135 to +135 degrees
      knob.style.transform = `rotate(${rotation}deg)`;
      
      // Send CC (with rate limiting)
      controller.sendCC(config.cc, newValue);
    });
    
    document.addEventListener('mouseup', () => {
      isDragging = false;
    });
  });
}

// ============================================================
// Event Handlers
// ============================================================

connectBleBtn.addEventListener('click', async () => {
  try {
    await controller.connectBle();
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

octDownBtn.addEventListener('click', () => {
  controller.setOctave(controller.getOctave() - 1);
  octaveDisplay.textContent = controller.getOctave().toString();
});

octUpBtn.addEventListener('click', () => {
  controller.setOctave(controller.getOctave() + 1);
  octaveDisplay.textContent = controller.getOctave().toString();
});

gateToggle.addEventListener('click', () => {
  controller.toggleGate();
  gateToggle.classList.toggle('active');
});

accentToggle.addEventListener('click', () => {
  controller.toggleAccent();
  accentToggle.classList.toggle('active');
});

slideToggle.addEventListener('click', () => {
  controller.toggleSlide();
  slideToggle.classList.toggle('active');
});

panicBtn.addEventListener('click', () => {
  controller.allNotesOff();
});

testBurstBtn.addEventListener('click', () => {
  controller.sendTestBurst();
});

// Log controls - Serial
searchBoxSerial.addEventListener('input', () => {
  renderSerialLog();
});

autoScrollCheckSerial.addEventListener('change', () => {
  // Auto-scroll is handled per render
});

showDeltaCheckSerial.addEventListener('change', () => {
  renderSerialLog();
});

pauseLogBtnSerial.addEventListener('click', () => {
  if (logger.isPaused()) {
    logger.resume();
    pauseLogBtnSerial.textContent = 'Pause';
    pauseLogBtnMidi.textContent = 'Pause';
  } else {
    logger.pause();
    pauseLogBtnSerial.textContent = 'Resume';
    pauseLogBtnMidi.textContent = 'Resume';
  }
});

clearLogBtnSerial.addEventListener('click', () => {
  logger.clear();
  renderSerialLog();
  renderMidiLog();
});

// Log controls - MIDI
searchBoxMidi.addEventListener('input', () => {
  renderMidiLog();
});

filterMidiIn.addEventListener('change', () => {
  renderMidiLog();
});

filterMidiOut.addEventListener('change', () => {
  renderMidiLog();
});

autoScrollCheckMidi.addEventListener('change', () => {
  // Auto-scroll is handled per render
});

showDeltaCheckMidi.addEventListener('change', () => {
  renderMidiLog();
});

pauseLogBtnMidi.addEventListener('click', () => {
  if (logger.isPaused()) {
    logger.resume();
    pauseLogBtnSerial.textContent = 'Pause';
    pauseLogBtnMidi.textContent = 'Pause';
  } else {
    logger.pause();
    pauseLogBtnSerial.textContent = 'Resume';
    pauseLogBtnMidi.textContent = 'Resume';
  }
});

clearLogBtnMidi.addEventListener('click', () => {
  logger.clear();
  renderSerialLog();
  renderMidiLog();
});

exportJsonBtn.addEventListener('click', () => {
  downloadFile('cyd-debug-log.json', logger.exportJson());
});

exportCsvBtn.addEventListener('click', () => {
  downloadFile('cyd-debug-log.csv', logger.exportCsv());
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

let renderSerialScheduled = false;
let renderMidiScheduled = false;

function renderSerialLog(): void {
  if (renderSerialScheduled) return;
  renderSerialScheduled = true;
  
  requestAnimationFrame(() => {
    const allEntries = logger.getEntries();
    const searchTerm = searchBoxSerial.value.toLowerCase();
    const showDelta = showDeltaCheckSerial.checked;
    
    // Filter for serial entries only
    const entries = allEntries.filter(entry => {
      const isSerial = entry.source === 'SERIAL_IN' || entry.source === 'SERIAL_OUT';
      if (!isSerial) return false;
      if (searchTerm && !entry.text.toLowerCase().includes(searchTerm)) return false;
      return true;
    });
    
    const autoScroll = autoScrollCheckSerial.checked;
    const wasAtBottom = logViewerSerial.scrollHeight - logViewerSerial.scrollTop <= logViewerSerial.clientHeight + 50;
    
    // Render entries
    logViewerSerial.innerHTML = entries
      .map(entry => {
        const cssClass = logger.getSourceClass(entry.source);
        const formatted = logger.formatEntry(entry);
        return `<div class="log-entry ${cssClass}">${formatted}</div>`;
      })
      .join('');
    
    // Auto-scroll if enabled and was at bottom
    if (autoScroll && wasAtBottom) {
      logViewerSerial.scrollTop = logViewerSerial.scrollHeight;
    }
    
    renderSerialScheduled = false;
  });
}

function renderMidiLog(): void {
  if (renderMidiScheduled) return;
  renderMidiScheduled = true;
  
  requestAnimationFrame(() => {
    const allEntries = logger.getEntries();
    const searchTerm = searchBoxMidi.value.toLowerCase();
    const showDelta = showDeltaCheckMidi.checked;
    
    // Filter for MIDI entries only
    const entries = allEntries.filter(entry => {
      // Check filters
      if (entry.source === 'BLE_IN' && !filterMidiIn.checked) return false;
      if ((entry.source === 'BLE_OUT' || entry.source === 'UI_TO_MIDI') && !filterMidiOut.checked) return false;
      
      const isMidi = entry.source === 'BLE_IN' || entry.source === 'BLE_OUT' || entry.source === 'UI_TO_MIDI';
      if (!isMidi) return false;
      if (searchTerm && !entry.text.toLowerCase().includes(searchTerm)) return false;
      return true;
    });
    
    const autoScroll = autoScrollCheckMidi.checked;
    const wasAtBottom = logViewerMidi.scrollHeight - logViewerMidi.scrollTop <= logViewerMidi.clientHeight + 50;
    
    // Render entries
    logViewerMidi.innerHTML = entries
      .map(entry => {
        const cssClass = logger.getSourceClass(entry.source);
        const formatted = logger.formatEntry(entry);
        return `<div class="log-entry ${cssClass}">${formatted}</div>`;
      })
      .join('');
    
    // Auto-scroll if enabled and was at bottom
    if (autoScroll && wasAtBottom) {
      logViewerMidi.scrollTop = logViewerMidi.scrollHeight;
    }
    
    renderMidiScheduled = false;
  });
}

// Subscribe to log updates
eventBus.subscribe(() => {
  renderSerialLog();
  renderMidiLog();
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

createKeyboard();
createKnobs();
updateConnectionStatus();

console.log('aCYD-MIDI Web Debug Console initialized');
