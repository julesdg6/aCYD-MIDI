#include "module_drum_seq_clocked.h"
#include "midi_out_buffer.h"
#include <Arduino.h>
#include <cstring>

// Register this module with the factory
REGISTER_MODULE(DrumSeqClocked, "drum_seq_clocked")

// Define static constexpr member (required for C++11/14)
constexpr uint8_t DrumSeqClocked::kDrumNotes[DrumSeqClocked::kNumTracks];

DrumSeqClocked::DrumSeqClocked() 
  : currentStep_(0), gateLength_(3), velocity_(100) {
  // Clear pattern
  memset(pattern_, 0, sizeof(pattern_));
}

DrumSeqClocked::~DrumSeqClocked() {
}

void DrumSeqClocked::init() {
  Serial.println("[DrumSeqClocked] init()");
  reset();
}

void DrumSeqClocked::reset() {
  Serial.println("[DrumSeqClocked] reset()");
  currentStep_ = 0;
}

void DrumSeqClocked::onTransportStart() {
  Serial.println("[DrumSeqClocked] onTransportStart()");
  currentStep_ = 0;
}

void DrumSeqClocked::onTransportStop() {
  Serial.println("[DrumSeqClocked] onTransportStop()");
  
  // Send all notes off for this module's tracks
  for (size_t track = 0; track < kNumTracks; ++track) {
    midiOutBuffer.noteOff(0, kDrumNotes[track], 0);
  }
}

void DrumSeqClocked::onStep(const StepContext& ctx) {
  // Advance to next step (wrapping at 16)
  size_t stepIndex = static_cast<size_t>(ctx.stepIndex % kNumSteps);
  currentStep_ = stepIndex;
  
  // Play notes for this step
  for (size_t track = 0; track < kNumTracks; ++track) {
    if (pattern_[track][stepIndex]) {
      // Use note-with-duration for automatic note-off
      midiOutBuffer.note(0, kDrumNotes[track], velocity_, gateLength_);
      
      Serial.printf("[DrumSeqClocked] Step %zu: Track %zu (note %d)\n",
                    stepIndex, track, kDrumNotes[track]);
    }
  }
}

void DrumSeqClocked::setParam(uint16_t paramId, int32_t value) {
  switch (paramId) {
    case PARAM_GATE_LEN:
      gateLength_ = static_cast<uint8_t>(value & 0xFF);
      break;
      
    case PARAM_VELOCITY:
      velocity_ = static_cast<uint8_t>(value & 0x7F);
      break;
      
    case PARAM_CHANNEL:
      // Currently fixed to channel 0 (MIDI channel 1)
      // Could be extended to support channel selection
      break;
      
    default:
      Serial.printf("[DrumSeqClocked] Unknown param: %u\n", paramId);
      break;
  }
}

int32_t DrumSeqClocked::getParam(uint16_t paramId) const {
  switch (paramId) {
    case PARAM_GATE_LEN:
      return gateLength_;
      
    case PARAM_VELOCITY:
      return velocity_;
      
    case PARAM_CHANNEL:
      return 0;  // Fixed to channel 0
      
    default:
      return 0;
  }
}

void DrumSeqClocked::serialize(uint8_t* buffer, size_t maxSize, size_t& outSize) const {
  // Simple serialization: write pattern as bytes
  size_t patternSize = sizeof(pattern_);
  if (maxSize < patternSize + 2) {
    outSize = 0;
    return;
  }
  
  buffer[0] = gateLength_;
  buffer[1] = velocity_;
  memcpy(buffer + 2, pattern_, patternSize);
  outSize = patternSize + 2;
}

bool DrumSeqClocked::deserialize(const uint8_t* buffer, size_t size) {
  size_t expectedSize = sizeof(pattern_) + 2;
  if (size < expectedSize) {
    Serial.printf("[DrumSeqClocked] Deserialize failed: size %zu < %zu\n", size, expectedSize);
    return false;
  }
  
  gateLength_ = buffer[0];
  velocity_ = buffer[1];
  memcpy(pattern_, buffer + 2, sizeof(pattern_));
  
  Serial.println("[DrumSeqClocked] Deserialized successfully");
  return true;
}

void DrumSeqClocked::toggleStep(size_t track, size_t step) {
  if (track >= kNumTracks || step >= kNumSteps) {
    return;
  }
  
  pattern_[track][step] = !pattern_[track][step];
  Serial.printf("[DrumSeqClocked] Toggle track %zu step %zu -> %d\n", 
                track, step, pattern_[track][step]);
}

bool DrumSeqClocked::getStep(size_t track, size_t step) const {
  if (track >= kNumTracks || step >= kNumSteps) {
    return false;
  }
  
  return pattern_[track][step];
}

void DrumSeqClocked::clearAll() {
  memset(pattern_, 0, sizeof(pattern_));
  Serial.println("[DrumSeqClocked] Cleared all patterns");
}
