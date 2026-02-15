#include "clocked_module.h"
#include <Arduino.h>
#include <cstring>

ModuleFactory& ModuleFactory::instance() {
  static ModuleFactory factory;
  return factory;
}

void ModuleFactory::registerModule(const char* typeId, CreateFn createFn) {
  if (count_ >= kMaxModuleTypes) {
    Serial.printf("[ModuleFactory] ERROR: Cannot register '%s' - max types reached\n", typeId);
    return;
  }
  
  // Check for duplicate
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(entries_[i].typeId, typeId) == 0) {
      Serial.printf("[ModuleFactory] WARNING: Module '%s' already registered\n", typeId);
      return;
    }
  }
  
  entries_[count_].typeId = typeId;
  entries_[count_].createFn = createFn;
  count_++;
  
  Serial.printf("[ModuleFactory] Registered module type: %s\n", typeId);
}

ClockedModule* ModuleFactory::create(const char* typeId) {
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(entries_[i].typeId, typeId) == 0) {
      ClockedModule* module = entries_[i].createFn();
      Serial.printf("[ModuleFactory] Created module: %s\n", typeId);
      return module;
    }
  }
  
  Serial.printf("[ModuleFactory] ERROR: Unknown module type: %s\n", typeId);
  return nullptr;
}

const char** ModuleFactory::getTypeIds(size_t& outCount) const {
  outCount = count_;
  static const char* typeIds[kMaxModuleTypes];
  for (size_t i = 0; i < count_; ++i) {
    typeIds[i] = entries_[i].typeId;
  }
  return typeIds;
}
