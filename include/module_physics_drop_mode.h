#ifndef MODULE_PHYSICS_DROP_MODE_H
#define MODULE_PHYSICS_DROP_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"
#include <new>

// Physics Drop mode variables
struct DropBall {
  float x, y;
  float vx, vy;
  float gravity = 0.15;
  float bounce = 0.6;
  float friction = 0.98;
  uint16_t color;
  int size;
  bool active;
  unsigned long spawnTime;
  int note;
  String noteName;
};

struct Platform {
  float x, y;
  float w, h;
  float angle; // In radians
  uint16_t color;
  bool active;
  int note;
  String noteName;
  unsigned long activeTime;
};

#define MAX_DROP_BALLS 8
#define MAX_PLATFORMS 6

// Function declarations
void initializePhysicsDropMode();
void drawPhysicsDropMode();
void handlePhysicsDropMode();
void drawDropBalls();
void drawPlatforms();
void updatePhysics();
void spawnDropBall(int x, int y);
void addPlatform(int x, int y);
void checkPlatformCollisions();

#endif // MODULE_PHYSICS_DROP_MODE_H
