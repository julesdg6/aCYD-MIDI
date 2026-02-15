#ifndef MIDI_OUT_BUFFER_H
#define MIDI_OUT_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/**
 * MIDI Output Buffer - Thread-safe buffered MIDI output
 * 
 * Provides a lock-free ring buffer for MIDI events with a dedicated
 * FreeRTOS task for transmission. Ensures tight timing by decoupling
 * MIDI generation from transmission.
 * 
 * Features:
 * - Thread-safe enqueueing from any context (ISR-safe)
 * - Dedicated output task for minimal jitter
 * - Scheduled note-offs with tick-based timing
 * - Panic/all-notes-off on stop/reset
 * - Integration with BLE, Hardware MIDI, WiFi transports
 */

// MIDI event types
enum class MidiEventType : uint8_t {
  NOTE_ON = 0,
  NOTE_OFF,
  CONTROL_CHANGE,
  CLOCK,
  START,
  CONTINUE,
  STOP,
  NOTE_WITH_DURATION  // Special: note-on with auto note-off
};

// MIDI event structure (fixed size for ring buffer)
struct MidiEvent {
  MidiEventType type;
  uint8_t channel;  // 0-15 for channel events
  uint8_t data1;    // note/cc number
  uint8_t data2;    // velocity/cc value
  uint16_t duration; // For NOTE_WITH_DURATION: duration in ticks
  uint32_t timestamp; // Tick count when event was created
  
  MidiEvent() : type(MidiEventType::NOTE_ON), channel(0), data1(0), 
                data2(0), duration(0), timestamp(0) {}
};

// Scheduled note-off entry
struct ScheduledNoteOff {
  uint8_t channel;
  uint8_t note;
  uint32_t offTick;  // Tick count when note should turn off
  bool active;
  
  ScheduledNoteOff() : channel(0), note(0), offTick(0), active(false) {}
};

class MidiOutBuffer {
public:
  static constexpr size_t kBufferSize = 256;
  static constexpr size_t kMaxScheduledNotes = 64;
  
  MidiOutBuffer();
  ~MidiOutBuffer();
  
  // Initialize and start output task
  void init();
  
  // Shutdown and cleanup
  void shutdown();
  
  // Immediate MIDI events (enqueue for sending)
  bool noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  bool noteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0);
  bool controlChange(uint8_t channel, uint8_t cc, uint8_t value);
  
  // Note with automatic note-off after duration (in ticks)
  bool note(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t durationTicks);
  
  // Transport messages
  bool midiClock();
  bool midiStart();
  bool midiContinue();
  bool midiStop();
  
  // Update scheduled note-offs (call from clock update with current tick)
  void updateScheduledNotes(uint32_t currentTick);
  
  // Panic: send all notes off on all channels
  void panic();
  
  // Send all notes off for specific channel
  void allNotesOff(uint8_t channel);
  
  // Get buffer statistics
  size_t getQueuedCount() const;
  size_t getActiveNoteCount() const;
  bool isEmpty() const;
  
private:
  // Ring buffer for events
  MidiEvent buffer_[kBufferSize];
  volatile size_t writeIndex_;
  volatile size_t readIndex_;
  mutable SemaphoreHandle_t mutex_;
  
  // Scheduled note-offs
  ScheduledNoteOff scheduledNotes_[kMaxScheduledNotes];
  SemaphoreHandle_t notesMutex_;
  
  // Output task
  TaskHandle_t taskHandle_;
  volatile bool running_;
  
  // Internal methods
  bool enqueue(const MidiEvent& event);
  bool dequeue(MidiEvent& event);
  void processEvent(const MidiEvent& event);
  void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
  void sendSystemRealtime(uint8_t message);
  bool addScheduledNoteOff(uint8_t channel, uint8_t note, uint32_t offTick);
  void removeScheduledNoteOff(uint8_t channel, uint8_t note);
  
  // Task function
  static void outputTask(void* parameter);
  void taskLoop();
};

// Global instance (defined in midi_out_buffer.cpp)
extern MidiOutBuffer midiOutBuffer;

#endif // MIDI_OUT_BUFFER_H
