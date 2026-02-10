#include "midi_out_buffer.h"
#include "midi_utils.h"
#include "common_definitions.h"
#include <Arduino.h>
#include <algorithm>

// Global instance
MidiOutBuffer midiOutBuffer;

namespace {
  static constexpr TickType_t kTaskDelay = pdMS_TO_TICKS(1);
  static constexpr const char* kTaskName = "MidiOut";
  static constexpr UBaseType_t kTaskPriority = configMAX_PRIORITIES - 1;  // High priority
  static constexpr uint16_t kStackDepth = 4096;
}

MidiOutBuffer::MidiOutBuffer() 
  : writeIndex_(0), readIndex_(0), mutex_(nullptr), notesMutex_(nullptr),
    taskHandle_(nullptr), running_(false) {
}

MidiOutBuffer::~MidiOutBuffer() {
  shutdown();
}

void MidiOutBuffer::init() {
  if (running_) {
    return;
  }
  
  // Create mutexes
  mutex_ = xSemaphoreCreateMutex();
  notesMutex_ = xSemaphoreCreateMutex();
  
  if (mutex_ == nullptr || notesMutex_ == nullptr) {
    Serial.println("[MidiOutBuffer] Failed to create mutexes");
    return;
  }
  
  // Reset buffer
  writeIndex_ = 0;
  readIndex_ = 0;
  
  // Clear scheduled notes
  for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
    scheduledNotes_[i].active = false;
  }
  
  // Start output task
  running_ = true;
  BaseType_t result = xTaskCreatePinnedToCore(
    outputTask,
    kTaskName,
    kStackDepth,
    this,
    kTaskPriority,
    &taskHandle_,
    1  // Pin to core 1
  );
  
  if (result != pdPASS) {
    Serial.println("[MidiOutBuffer] Failed to create output task");
    running_ = false;
    return;
  }
  
  Serial.println("[MidiOutBuffer] Initialized");
}

void MidiOutBuffer::shutdown() {
  if (!running_) {
    return;
  }
  
  running_ = false;
  
  // Wait for task to finish
  if (taskHandle_ != nullptr) {
    vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelete(taskHandle_);
    taskHandle_ = nullptr;
  }
  
  // Delete mutexes
  if (mutex_ != nullptr) {
    vSemaphoreDelete(mutex_);
    mutex_ = nullptr;
  }
  if (notesMutex_ != nullptr) {
    vSemaphoreDelete(notesMutex_);
    notesMutex_ = nullptr;
  }
  
  Serial.println("[MidiOutBuffer] Shutdown");
}

bool MidiOutBuffer::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  MidiEvent event;
  event.type = MidiEventType::NOTE_ON;
  event.channel = channel & 0x0F;
  event.data1 = note & 0x7F;
  event.data2 = velocity & 0x7F;
  return enqueue(event);
}

bool MidiOutBuffer::noteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  MidiEvent event;
  event.type = MidiEventType::NOTE_OFF;
  event.channel = channel & 0x0F;
  event.data1 = note & 0x7F;
  event.data2 = velocity & 0x7F;
  
  // Also remove from scheduled notes if present
  removeScheduledNoteOff(channel, note);
  
  return enqueue(event);
}

bool MidiOutBuffer::controlChange(uint8_t channel, uint8_t cc, uint8_t value) {
  MidiEvent event;
  event.type = MidiEventType::CONTROL_CHANGE;
  event.channel = channel & 0x0F;
  event.data1 = cc & 0x7F;
  event.data2 = value & 0x7F;
  return enqueue(event);
}

bool MidiOutBuffer::note(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t durationTicks) {
  MidiEvent event;
  event.type = MidiEventType::NOTE_WITH_DURATION;
  event.channel = channel & 0x0F;
  event.data1 = note & 0x7F;
  event.data2 = velocity & 0x7F;
  event.duration = durationTicks;
  return enqueue(event);
}

bool MidiOutBuffer::midiClock() {
  MidiEvent event;
  event.type = MidiEventType::CLOCK;
  return enqueue(event);
}

bool MidiOutBuffer::midiStart() {
  MidiEvent event;
  event.type = MidiEventType::START;
  return enqueue(event);
}

bool MidiOutBuffer::midiContinue() {
  MidiEvent event;
  event.type = MidiEventType::CONTINUE;
  return enqueue(event);
}

bool MidiOutBuffer::midiStop() {
  MidiEvent event;
  event.type = MidiEventType::STOP;
  return enqueue(event);
}

void MidiOutBuffer::updateScheduledNotes(uint32_t currentTick) {
  if (notesMutex_ == nullptr || xSemaphoreTake(notesMutex_, 0) != pdTRUE) {
    return;
  }
  
  for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
    if (scheduledNotes_[i].active && currentTick >= scheduledNotes_[i].offTick) {
      // Send note off
      noteOff(scheduledNotes_[i].channel, scheduledNotes_[i].note, 0);
      scheduledNotes_[i].active = false;
    }
  }
  
  xSemaphoreGive(notesMutex_);
}

void MidiOutBuffer::panic() {
  Serial.println("[MidiOutBuffer] PANIC - All notes off");
  
  // Send all notes off on all 16 channels
  for (uint8_t ch = 0; ch < 16; ++ch) {
    allNotesOff(ch);
  }
  
  // Clear scheduled notes
  if (notesMutex_ != nullptr && xSemaphoreTake(notesMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
      scheduledNotes_[i].active = false;
    }
    xSemaphoreGive(notesMutex_);
  }
}

void MidiOutBuffer::allNotesOff(uint8_t channel) {
  // Send CC 123 (All Notes Off)
  controlChange(channel, 123, 0);
  // Also send CC 120 (All Sound Off) for good measure
  controlChange(channel, 120, 0);
}

size_t MidiOutBuffer::getQueuedCount() const {
  if (mutex_ == nullptr) return 0;
  
  xSemaphoreTake(mutex_, portMAX_DELAY);
  size_t count = (writeIndex_ >= readIndex_) 
    ? (writeIndex_ - readIndex_)
    : (kBufferSize - readIndex_ + writeIndex_);
  xSemaphoreGive(mutex_);
  return count;
}

size_t MidiOutBuffer::getActiveNoteCount() const {
  if (notesMutex_ == nullptr) return 0;
  
  size_t count = 0;
  xSemaphoreTake(notesMutex_, portMAX_DELAY);
  for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
    if (scheduledNotes_[i].active) {
      ++count;
    }
  }
  xSemaphoreGive(notesMutex_);
  return count;
}

bool MidiOutBuffer::isEmpty() const {
  return getQueuedCount() == 0;
}

bool MidiOutBuffer::enqueue(const MidiEvent& event) {
  if (mutex_ == nullptr || !running_) {
    return false;
  }
  
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(1)) != pdTRUE) {
    return false;
  }
  
  size_t nextWrite = (writeIndex_ + 1) % kBufferSize;
  if (nextWrite == readIndex_) {
    // Buffer full
    xSemaphoreGive(mutex_);
    Serial.println("[MidiOutBuffer] WARNING: Buffer full, dropping event");
    return false;
  }
  
  buffer_[writeIndex_] = event;
  writeIndex_ = nextWrite;
  
  xSemaphoreGive(mutex_);
  return true;
}

bool MidiOutBuffer::dequeue(MidiEvent& event) {
  if (mutex_ == nullptr || !running_) {
    return false;
  }
  
  if (xSemaphoreTake(mutex_, 0) != pdTRUE) {
    return false;
  }
  
  if (readIndex_ == writeIndex_) {
    // Buffer empty
    xSemaphoreGive(mutex_);
    return false;
  }
  
  event = buffer_[readIndex_];
  readIndex_ = (readIndex_ + 1) % kBufferSize;
  
  xSemaphoreGive(mutex_);
  return true;
}

void MidiOutBuffer::processEvent(const MidiEvent& event) {
  switch (event.type) {
    case MidiEventType::NOTE_ON:
      sendMidiMessage(0x90 | event.channel, event.data1, event.data2);
      break;
      
    case MidiEventType::NOTE_OFF:
      sendMidiMessage(0x80 | event.channel, event.data1, event.data2);
      break;
      
    case MidiEventType::CONTROL_CHANGE:
      sendMidiMessage(0xB0 | event.channel, event.data1, event.data2);
      break;
      
    case MidiEventType::NOTE_WITH_DURATION:
      // Send note on immediately
      sendMidiMessage(0x90 | event.channel, event.data1, event.data2);
      // Schedule note off (using timestamp from clock)
      addScheduledNoteOff(event.channel, event.data1, event.timestamp + event.duration);
      break;
      
    case MidiEventType::CLOCK:
      sendSystemRealtime(0xF8);
      break;
      
    case MidiEventType::START:
      sendSystemRealtime(0xFA);
      break;
      
    case MidiEventType::CONTINUE:
      sendSystemRealtime(0xFB);
      break;
      
    case MidiEventType::STOP:
      sendSystemRealtime(0xFC);
      break;
  }
}

void MidiOutBuffer::sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
  // Use existing MIDI send infrastructure
  sendMIDI(status, data1, data2);
}

void MidiOutBuffer::sendSystemRealtime(uint8_t message) {
  // System realtime messages are single byte
  // Use existing infrastructure - these map to specific functions
  if (message == 0xF8) {
    sendMIDIClock();
  } else if (message == 0xFA) {
    sendMIDIStart();
  } else if (message == 0xFC) {
    sendMIDIStop();
  }
  // Note: 0xFB (Continue) currently not in midi_utils.h, would need to add
}

bool MidiOutBuffer::addScheduledNoteOff(uint8_t channel, uint8_t note, uint32_t offTick) {
  if (notesMutex_ == nullptr || xSemaphoreTake(notesMutex_, pdMS_TO_TICKS(1)) != pdTRUE) {
    return false;
  }
  
  // Find free slot
  bool added = false;
  for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
    if (!scheduledNotes_[i].active) {
      scheduledNotes_[i].channel = channel;
      scheduledNotes_[i].note = note;
      scheduledNotes_[i].offTick = offTick;
      scheduledNotes_[i].active = true;
      added = true;
      break;
    }
  }
  
  xSemaphoreGive(notesMutex_);
  
  if (!added) {
    Serial.println("[MidiOutBuffer] WARNING: No free slots for scheduled note-off");
  }
  
  return added;
}

void MidiOutBuffer::removeScheduledNoteOff(uint8_t channel, uint8_t note) {
  if (notesMutex_ == nullptr || xSemaphoreTake(notesMutex_, pdMS_TO_TICKS(1)) != pdTRUE) {
    return;
  }
  
  for (size_t i = 0; i < kMaxScheduledNotes; ++i) {
    if (scheduledNotes_[i].active && 
        scheduledNotes_[i].channel == channel && 
        scheduledNotes_[i].note == note) {
      scheduledNotes_[i].active = false;
      break;  // Only remove first match
    }
  }
  
  xSemaphoreGive(notesMutex_);
}

void MidiOutBuffer::outputTask(void* parameter) {
  MidiOutBuffer* buffer = static_cast<MidiOutBuffer*>(parameter);
  buffer->taskLoop();
}

void MidiOutBuffer::taskLoop() {
  Serial.println("[MidiOutBuffer] Task started");
  
  while (running_) {
    MidiEvent event;
    while (dequeue(event)) {
      processEvent(event);
    }
    
    vTaskDelay(kTaskDelay);
  }
  
  Serial.println("[MidiOutBuffer] Task stopped");
}
