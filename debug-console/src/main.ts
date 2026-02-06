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

// Log controls
const searchBox = $('search-box') as HTMLInputElement;
const filterMidiIn = $('filter-midi-in') as HTMLInputElement;
const filterMidiOut = $('filter-midi-out') as HTMLInputElement;
const filterSerial = $('filter-serial') as HTMLInputElement;
const autoScrollCheck = $('auto-scroll') as HTMLInputElement;
const showDeltaCheck = $('show-delta') as HTMLInputElement;
const pauseLogBtn = $('pause-log');
const clearLogBtn = $('clear-log');
const exportJsonBtn = $('export-json');
const exportCsvBtn = $('export-csv');
const logViewer = $('log-viewer');

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

// Log controls
searchBox.addEventListener('input', () => {
  logger.updateConfig({ searchTerm: searchBox.value });
  renderLog();
});

filterMidiIn.addEventListener('change', updateFilters);
filterMidiOut.addEventListener('change', updateFilters);
filterSerial.addEventListener('change', updateFilters);

function updateFilters(): void {
  const filters = new Set<any>();
  if (filterMidiIn.checked) {
    filters.add('BLE_IN');
  }
  if (filterMidiOut.checked) {
    filters.add('BLE_OUT');
    filters.add('UI_TO_MIDI');
  }
  if (filterSerial.checked) {
    filters.add('SERIAL_IN');
    filters.add('SERIAL_OUT');
  }
  logger.updateConfig({ filters });
  renderLog();
}

autoScrollCheck.addEventListener('change', () => {
  logger.updateConfig({ autoScroll: autoScrollCheck.checked });
});

showDeltaCheck.addEventListener('change', () => {
  logger.updateConfig({ showDelta: showDeltaCheck.checked });
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

let renderScheduled = false;

function renderLog(): void {
  if (renderScheduled) return;
  renderScheduled = true;
  
  requestAnimationFrame(() => {
    const entries = logger.getEntries();
    const autoScroll = autoScrollCheck.checked;
    const wasAtBottom = logViewer.scrollHeight - logViewer.scrollTop <= logViewer.clientHeight + 50;
    
    // Simple rendering (for v1, can be optimized with virtualization later)
    logViewer.innerHTML = entries
      .map(entry => {
        const cssClass = logger.getSourceClass(entry.source);
        const formatted = logger.formatEntry(entry);
        return `<div class="log-entry ${cssClass}">${formatted}</div>`;
      })
      .join('');
    
    // Auto-scroll if enabled and was at bottom
    if (autoScroll && wasAtBottom) {
      logViewer.scrollTop = logViewer.scrollHeight;
    }
    
    renderScheduled = false;
  });
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

createKeyboard();
createKnobs();
updateConnectionStatus();

console.log('CYD Web Debug Console initialized');
