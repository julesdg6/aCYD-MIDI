#include "clock_runtime.h"
#include "midi_out_buffer.h"
#include "clock_manager.h"
#include <Arduino.h>
#include <algorithm>

// Global instance
ClockRuntime clockRuntime;

ClockRuntime::ClockRuntime()
  : state_(TransportState::STOPPED), currentTick_(0), 
    startPendingAtTick_(0), stopPendingAtTick_(0),
    bpm_(120), swingPercent_(0),
    startQuantize_(QuantizeMode::NEXT_BAR),
    stopQuantize_(QuantizeMode::END_OF_BAR),
    slotCount_(0), slotsMutex_(nullptr) {
}

ClockRuntime::~ClockRuntime() {
  shutdown();
}

void ClockRuntime::init() {
  Serial.println("[ClockRuntime] Initializing");
  
  // Create mutex
  slotsMutex_ = xSemaphoreCreateMutex();
  if (slotsMutex_ == nullptr) {
    Serial.println("[ClockRuntime] ERROR: Failed to create mutex");
    return;
  }
  
  // Reset state
  state_ = TransportState::STOPPED;
  currentTick_ = 0;
  startPendingAtTick_ = 0;
  stopPendingAtTick_ = 0;
  slotCount_ = 0;
  
  Serial.println("[ClockRuntime] Initialized");
}

void ClockRuntime::shutdown() {
  if (slotsMutex_ != nullptr) {
    vSemaphoreDelete(slotsMutex_);
    slotsMutex_ = nullptr;
  }
}

void ClockRuntime::requestStart() {
  if (state_ == TransportState::RUNNING || state_ == TransportState::PENDING_START) {
    return;
  }
  
  Serial.printf("[ClockRuntime] Request START (quantize: %d)\n", static_cast<int>(startQuantize_));
  
  state_ = TransportState::PENDING_START;
  startPendingAtTick_ = currentTick_;
  
  // If immediate, transition now
  if (startQuantize_ == QuantizeMode::IMMEDIATE) {
    transitionToRunning();
  }
}

void ClockRuntime::requestStop() {
  if (state_ != TransportState::RUNNING) {
    return;
  }
  
  Serial.printf("[ClockRuntime] Request STOP (quantize: %d)\n", static_cast<int>(stopQuantize_));
  
  state_ = TransportState::PENDING_STOP;
  stopPendingAtTick_ = currentTick_;
  
  // If immediate, transition now
  if (stopQuantize_ == QuantizeMode::IMMEDIATE) {
    transitionToStopped();
  }
}

void ClockRuntime::cancelStart() {
  if (state_ == TransportState::PENDING_START) {
    Serial.println("[ClockRuntime] Cancel START");
    state_ = TransportState::STOPPED;
    startPendingAtTick_ = 0;
  }
}

void ClockRuntime::forceStop() {
  if (state_ != TransportState::STOPPED) {
    Serial.println("[ClockRuntime] Force STOP");
    transitionToStopped();
  }
}

void ClockRuntime::reset() {
  Serial.println("[ClockRuntime] RESET");
  
  forceStop();
  currentTick_ = 0;
  
  // Reset all modules
  if (slotsMutex_ && xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < slotCount_; ++i) {
      if (slots_[i].module != nullptr) {
        slots_[i].module->reset();
        slots_[i].stepIndex = 0;
        slots_[i].lastStepTick = 0;
      }
    }
    xSemaphoreGive(slotsMutex_);
  }
  
  // Send panic
  midiOutBuffer.panic();
}

void ClockRuntime::requestTempo(uint16_t bpm) {
  if (bpm < kMinBPM) bpm = kMinBPM;
  if (bpm > kMaxBPM) bpm = kMaxBPM;
  
  if (bpm != bpm_) {
    Serial.printf("[ClockRuntime] Tempo change: %d -> %d BPM\n", bpm_, bpm);
    bpm_ = bpm;
    
    // Update global BPM (for backward compatibility with existing code)
    extern uint16_t sharedBPM;
    extern void setSharedBPM(uint16_t bpm);
    setSharedBPM(bpm);
  }
}

void ClockRuntime::setSwing(uint8_t percent) {
  if (percent > 50) percent = 50;
  if (swingPercent_ != percent) {
    Serial.printf("[ClockRuntime] Swing: %d%%\n", percent);
    swingPercent_ = percent;
  }
}

int ClockRuntime::registerModule(ClockedModule* module, uint8_t midiChannel) {
  if (module == nullptr) {
    return -1;
  }
  
  if (slotsMutex_ == nullptr || xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) != pdTRUE) {
    return -1;
  }
  
  if (slotCount_ >= kMaxSlots) {
    xSemaphoreGive(slotsMutex_);
    Serial.println("[ClockRuntime] ERROR: Max slots reached");
    return -1;
  }
  
  int slotId = static_cast<int>(slotCount_);
  slots_[slotCount_].module = module;
  slots_[slotCount_].midiChannel = midiChannel;
  slots_[slotCount_].mute = false;
  slots_[slotCount_].enabled = true;
  slots_[slotCount_].lastStepTick = 0;
  slots_[slotCount_].stepIndex = 0;
  slotCount_++;
  
  xSemaphoreGive(slotsMutex_);
  
  Serial.printf("[ClockRuntime] Registered module '%s' in slot %d (ch %d)\n",
                module->displayName(), slotId, midiChannel);
  
  // Initialize module
  module->init();
  
  return slotId;
}

void ClockRuntime::unregisterModule(int slotId) {
  if (slotId < 0 || slotId >= static_cast<int>(slotCount_)) {
    return;
  }
  
  if (slotsMutex_ == nullptr || xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  // Shift slots down
  for (size_t i = static_cast<size_t>(slotId); i < slotCount_ - 1; ++i) {
    slots_[i] = slots_[i + 1];
  }
  slotCount_--;
  
  xSemaphoreGive(slotsMutex_);
  
  Serial.printf("[ClockRuntime] Unregistered module from slot %d\n", slotId);
}

void ClockRuntime::setModuleMute(int slotId, bool mute) {
  if (slotId < 0 || slotId >= static_cast<int>(slotCount_)) {
    return;
  }
  
  if (slotsMutex_ && xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    slots_[slotId].mute = mute;
    xSemaphoreGive(slotsMutex_);
  }
}

void ClockRuntime::setModuleEnabled(int slotId, bool enabled) {
  if (slotId < 0 || slotId >= static_cast<int>(slotCount_)) {
    return;
  }
  
  if (slotsMutex_ && xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    slots_[slotId].enabled = enabled;
    xSemaphoreGive(slotsMutex_);
  }
}

ClockedModule* ClockRuntime::getModule(int slotId) {
  if (slotId < 0 || slotId >= static_cast<int>(slotCount_)) {
    return nullptr;
  }
  return slots_[slotId].module;
}

void ClockRuntime::processTick(uint32_t tick) {
  // Auto-increment if tick is 0 (internal clock mode)
  if (tick == 0) {
    currentTick_++;
  } else {
    currentTick_ = tick;
  }
  
  // Send MIDI clock if running or pending
  if (state_ == TransportState::RUNNING || 
      state_ == TransportState::PENDING_START ||
      state_ == TransportState::PENDING_STOP) {
    midiOutBuffer.midiClock();
  }
  
  // Check for pending start
  if (state_ == TransportState::PENDING_START && shouldStartNow(currentTick_)) {
    transitionToRunning();
  }
  
  // Check for pending stop
  if (state_ == TransportState::PENDING_STOP && shouldStopNow(currentTick_)) {
    transitionToStopped();
  }
  
  // Dispatch steps to modules if running
  if (state_ == TransportState::RUNNING) {
    dispatchStepToModules(currentTick_);
  }
  
  // Update scheduled note-offs
  midiOutBuffer.updateScheduledNotes(currentTick_);
}

void ClockRuntime::transitionToRunning() {
  Serial.printf("[ClockRuntime] Transport -> RUNNING (tick %u)\n", currentTick_);
  
  state_ = TransportState::RUNNING;
  
  // Send MIDI Start
  sendTransportMessage(state_);
  
  // Notify modules
  if (slotsMutex_ && xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < slotCount_; ++i) {
      if (slots_[i].module != nullptr && slots_[i].enabled) {
        slots_[i].module->onTransportStart();
        slots_[i].lastStepTick = currentTick_;
        slots_[i].stepIndex = 0;
      }
    }
    xSemaphoreGive(slotsMutex_);
  }
}

void ClockRuntime::transitionToStopped() {
  Serial.printf("[ClockRuntime] Transport -> STOPPED (tick %u)\n", currentTick_);
  
  TransportState oldState = state_;
  state_ = TransportState::STOPPED;
  
  // Send MIDI Stop (only if we were running)
  if (oldState == TransportState::RUNNING || oldState == TransportState::PENDING_STOP) {
    sendTransportMessage(state_);
  }
  
  // Notify modules
  if (slotsMutex_ && xSemaphoreTake(slotsMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < slotCount_; ++i) {
      if (slots_[i].module != nullptr) {
        slots_[i].module->onTransportStop();
      }
    }
    xSemaphoreGive(slotsMutex_);
  }
  
  // All notes off
  midiOutBuffer.panic();
}

bool ClockRuntime::shouldStartNow(uint32_t tick) {
  switch (startQuantize_) {
    case QuantizeMode::IMMEDIATE:
      return true;
      
    case QuantizeMode::NEXT_STEP:
      // Start on next step (assuming 1/16 note default)
      return isStepBoundary(tick, kDefaultTicksPerStep);
      
    case QuantizeMode::NEXT_BAR:
      return isBarStart(tick);
      
    case QuantizeMode::END_OF_BAR:
      // Not typical for start, but support it
      return isBarStart(tick);
  }
  
  return false;
}

bool ClockRuntime::shouldStopNow(uint32_t tick) {
  switch (stopQuantize_) {
    case QuantizeMode::IMMEDIATE:
      return true;
      
    case QuantizeMode::NEXT_STEP:
      return isStepBoundary(tick, kDefaultTicksPerStep);
      
    case QuantizeMode::NEXT_BAR:
      return isBarStart(tick);
      
    case QuantizeMode::END_OF_BAR: {
      // Stop at end of current bar (start of next bar)
      return isBarStart(tick);
    }
  }
  
  return false;
}

void ClockRuntime::dispatchStepToModules(uint32_t tick) {
  if (slotsMutex_ == nullptr || xSemaphoreTake(slotsMutex_, 0) != pdTRUE) {
    return;  // Skip if can't acquire lock immediately
  }
  
  for (size_t i = 0; i < slotCount_; ++i) {
    Slot& slot = slots_[i];
    
    if (slot.module == nullptr || !slot.enabled) {
      continue;
    }
    
    uint16_t ticksPerStep = slot.module->ticksPerStep();
    
    // Check if we're at a step boundary for this module
    if (!isStepBoundary(tick, ticksPerStep)) {
      continue;
    }
    
    // Build step context
    StepContext ctx;
    ctx.tick = tick;
    ctx.bpm_x10 = bpm_ * 10;
    ctx.ppqn = kPPQN;
    ctx.barIndex = tick / getTicksPerBar();
    ctx.tickInBar = tick % getTicksPerBar();
    ctx.stepIndex = slot.stepIndex++;
    ctx.ticksPerStep = ticksPerStep;
    ctx.stepInBar = static_cast<uint16_t>(ctx.tickInBar / ticksPerStep);
    ctx.isBarStart = isBarStart(tick);
    
    // Call module (skip MIDI output if muted)
    if (!slot.mute) {
      slot.module->onStep(ctx);
    } else if (slot.module->advanceWhileMuted()) {
      // Module advances playhead even when muted
      // (call with context but module knows it's muted)
      slot.module->onStep(ctx);
    }
    
    slot.lastStepTick = tick;
  }
  
  xSemaphoreGive(slotsMutex_);
}

void ClockRuntime::sendTransportMessage(TransportState newState) {
  switch (newState) {
    case TransportState::RUNNING:
      midiOutBuffer.midiStart();
      Serial.println("[ClockRuntime] MIDI START sent");
      break;
      
    case TransportState::STOPPED:
      midiOutBuffer.midiStop();
      Serial.println("[ClockRuntime] MIDI STOP sent");
      break;
      
    default:
      break;
  }
}

uint32_t ClockRuntime::getTicksPerBar() const {
  // For 4/4 time: 4 quarters * 24 ticks per quarter = 96 ticks per bar
  return kPPQN * timeSignature_.numerator;
}

bool ClockRuntime::isBarStart(uint32_t tick) const {
  return (tick % getTicksPerBar()) == 0;
}

bool ClockRuntime::isStepBoundary(uint32_t tick, uint16_t ticksPerStep) const {
  if (ticksPerStep == 0) return false;
  return (tick % ticksPerStep) == 0;
}

uint32_t ClockRuntime::applySwing(uint32_t tick, uint16_t ticksPerStep) const {
  // TODO: Implement swing timing
  // For now, return unmodified
  (void)ticksPerStep;
  return tick;
}
