#ifndef PHYSICS_DROP_MODE_H
#define PHYSICS_DROP_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

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
DropBall dropBalls[MAX_DROP_BALLS];
Platform platforms[MAX_PLATFORMS];
int numActiveDropBalls = 0;
int numPlatforms = 0;
int dropScale = 0;
int dropKey = 0;
int dropOctave = 4;
bool platformMode = false; // false = drop mode, true = platform edit mode

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

// Implementations
void initializePhysicsDropMode() {
  numActiveDropBalls = 0;
  numPlatforms = 0;
  dropScale = 0;
  dropKey = 0;
  dropOctave = 4;
  platformMode = false;
  
  // Initialize all balls as inactive
  for (int i = 0; i < MAX_DROP_BALLS; i++) {
    dropBalls[i].active = false;
  }
  
  // Create some default platforms (with scaled positions)
  platforms[0] = {(float)SCALE_X(80), (float)SCALE_Y(160), (float)SCALE_X(60), (float)SCALE_Y(8), 0.2, THEME_PRIMARY, false, 60, "C4", 0};
  platforms[1] = {(float)SCALE_X(180), (float)SCALE_Y(140), (float)SCALE_X(50), (float)SCALE_Y(8), -0.3, THEME_SECONDARY, false, 64, "E4", 0};
  platforms[2] = {(float)SCALE_X(120), (float)SCALE_Y(120), (float)SCALE_X(40), (float)SCALE_Y(8), 0.1, THEME_ACCENT, false, 67, "G4", 0};
  numPlatforms = 3;
}

void drawPhysicsDropMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("DROP", platformMode ? "Platform Edit" : "Tap to Drop");
  
  // Controls
  drawRoundButton(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, platformMode ? "DROP" : "EDIT", THEME_WARNING);
  drawRoundButton(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "CLEAR", THEME_ERROR);
  drawRoundButton(SCALE_X(110), SCALE_Y(200), BTN_MEDIUM_W, BTN_SMALL_H, "SCALE", THEME_ACCENT);
  drawRoundButton(SCALE_X(170), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "KEY-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(220), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "KEY+", THEME_SECONDARY);
  drawRoundButton(SCALE_X(270), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "OCT", THEME_PRIMARY);
  
  // Status display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String keyName = getNoteNameFromMIDI(dropKey);
  tft.drawString(keyName + " " + scales[dropScale].name, MARGIN_SMALL, SCALE_Y(180), 1);
  tft.drawString("Oct:" + String(dropOctave), SCALE_X(150), SCALE_Y(180), 1);
  tft.drawString("Balls:" + String(numActiveDropBalls), SCALE_X(220), SCALE_Y(180), 1);
  
  drawPlatforms();
  drawDropBalls();
}

void drawDropBalls() {
  for (int i = 0; i < MAX_DROP_BALLS; i++) {
    if (!dropBalls[i].active) continue;
    
    // Fade out old balls
    unsigned long age = millis() - dropBalls[i].spawnTime;
    if (age > 5000) {
      dropBalls[i].active = false;
      numActiveDropBalls--;
      continue;
    }
    
    tft.fillCircle(dropBalls[i].x, dropBalls[i].y, dropBalls[i].size, dropBalls[i].color);
    tft.drawCircle(dropBalls[i].x, dropBalls[i].y, dropBalls[i].size, THEME_TEXT);
  }
}

void drawPlatforms() {
  for (int i = 0; i < numPlatforms; i++) {
    uint16_t color = platforms[i].color;
    
    // Flash when hit
    if (platforms[i].active) {
      unsigned long elapsed = millis() - platforms[i].activeTime;
      if (elapsed < 200) {
        color = THEME_TEXT;
      } else {
        platforms[i].active = false;
      }
    }
    
    // Draw angled rectangle (simplified as normal rectangle for now)
    tft.fillRect(platforms[i].x, platforms[i].y, platforms[i].w, platforms[i].h, color);
    tft.drawRect(platforms[i].x, platforms[i].y, platforms[i].w, platforms[i].h, THEME_TEXT);
    
    // Show note name
    tft.setTextColor(THEME_BG, color);
    tft.drawCentreString(platforms[i].noteName, 
                        platforms[i].x + platforms[i].w/2, 
                        platforms[i].y + platforms[i].h/2 - 4, 1);
  }
}

void handlePhysicsDropMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    // Mode toggle
    if (isButtonPressed(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      platformMode = !platformMode;
      drawPhysicsDropMode();
      return;
    }
    
    // Clear button
    if (isButtonPressed(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      for (int i = 0; i < MAX_DROP_BALLS; i++) {
        dropBalls[i].active = false;
      }
      numActiveDropBalls = 0;
      numPlatforms = 0;
      drawPhysicsDropMode();
      return;
    }
    
    // Scale button
    if (isButtonPressed(SCALE_X(110), SCALE_Y(200), BTN_MEDIUM_W, BTN_SMALL_H)) {
      dropScale = (dropScale + 1) % NUM_SCALES;
      drawPhysicsDropMode();
      return;
    }
    
    // Key controls
    if (isButtonPressed(SCALE_X(170), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      dropKey = (dropKey - 1 + 12) % 12;
      drawPhysicsDropMode();
      return;
    }
    
    if (isButtonPressed(SCALE_X(220), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      dropKey = (dropKey + 1) % 12;
      drawPhysicsDropMode();
      return;
    }
    
    // Octave button
    if (isButtonPressed(SCALE_X(270), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      dropOctave = (dropOctave == 7) ? 2 : dropOctave + 1;
      drawPhysicsDropMode();
      return;
    }
    
    // Touch in play area
    if (touch.y >= 60 && touch.y <= 175) {
      if (platformMode) {
        // Add platform
        addPlatform(touch.x, touch.y);
      } else {
        // Drop ball
        spawnDropBall(touch.x, touch.y);
      }
      return;
    }
  }
  
  // Update physics
  updatePhysics();
}

void spawnDropBall(int x, int y) {
  if (numActiveDropBalls >= MAX_DROP_BALLS) return;
  
  // Find inactive ball slot
  for (int i = 0; i < MAX_DROP_BALLS; i++) {
    if (!dropBalls[i].active) {
      dropBalls[i].x = x;
      dropBalls[i].y = y;
      dropBalls[i].vx = random(-10, 11) / 10.0; // -1 to 1
      dropBalls[i].vy = 0;
      dropBalls[i].color = random(0x2000, 0x8FFF);
      dropBalls[i].size = random(3, 6);
      dropBalls[i].active = true;
      dropBalls[i].spawnTime = millis();
      dropBalls[i].note = getNoteInScale(dropScale, random(8), dropOctave) + dropKey;
      dropBalls[i].noteName = getNoteNameFromMIDI(dropBalls[i].note);
      numActiveDropBalls++;
      break;
    }
  }
}

void addPlatform(int x, int y) {
  if (numPlatforms >= MAX_PLATFORMS) return;
  
  platforms[numPlatforms].x = x - 25;
  platforms[numPlatforms].y = y - 4;
  platforms[numPlatforms].w = 50;
  platforms[numPlatforms].h = 8;
  platforms[numPlatforms].angle = random(-5, 6) / 10.0; // -0.5 to 0.5 radians
  platforms[numPlatforms].color = random(0x2000, 0xFFFF);
  platforms[numPlatforms].active = false;
  platforms[numPlatforms].note = getNoteInScale(dropScale, numPlatforms % 8, dropOctave) + dropKey;
  platforms[numPlatforms].noteName = getNoteNameFromMIDI(platforms[numPlatforms].note);
  platforms[numPlatforms].activeTime = 0;
  numPlatforms++;
  
  drawPhysicsDropMode();
}

void updatePhysics() {
  static unsigned long lastUpdate = 0;
  static float lastX[MAX_DROP_BALLS], lastY[MAX_DROP_BALLS];
  static bool initialized = false;
  
  if (millis() - lastUpdate < 50) return; // 20 FPS to reduce flickering
  
  // Initialize last positions
  if (!initialized) {
    for (int i = 0; i < MAX_DROP_BALLS; i++) {
      lastX[i] = dropBalls[i].x;
      lastY[i] = dropBalls[i].y;
    }
    initialized = true;
  }
  
  // Clear previous ball positions only
  for (int i = 0; i < MAX_DROP_BALLS; i++) {
    if (dropBalls[i].active) {
      tft.fillCircle(lastX[i], lastY[i], dropBalls[i].size + 1, THEME_BG);
    }
  }
  
  for (int i = 0; i < MAX_DROP_BALLS; i++) {
    if (!dropBalls[i].active) continue;
    
    // Store last position
    lastX[i] = dropBalls[i].x;
    lastY[i] = dropBalls[i].y;
    
    // Apply gravity
    dropBalls[i].vy += dropBalls[i].gravity;
    
    // Apply friction
    dropBalls[i].vx *= dropBalls[i].friction;
    
    // Update position
    dropBalls[i].x += dropBalls[i].vx;
    dropBalls[i].y += dropBalls[i].vy;
    
    // Boundary collisions
    if (dropBalls[i].x - dropBalls[i].size <= 10) {
      dropBalls[i].x = 10 + dropBalls[i].size;
      dropBalls[i].vx = -dropBalls[i].vx * dropBalls[i].bounce;
    }
    if (dropBalls[i].x + dropBalls[i].size >= 310) {
      dropBalls[i].x = 310 - dropBalls[i].size;
      dropBalls[i].vx = -dropBalls[i].vx * dropBalls[i].bounce;
    }
    if (dropBalls[i].y + dropBalls[i].size >= 175) {
      dropBalls[i].y = 175 - dropBalls[i].size;
      dropBalls[i].vy = -dropBalls[i].vy * dropBalls[i].bounce;
      
      // Ground hit - play note
      if (deviceConnected && abs(dropBalls[i].vy) > 1) {
        sendMIDI(0x90, dropBalls[i].note, random(60, 100));
        sendMIDI(0x80, dropBalls[i].note, 0);
      }
    }
  }
  
  checkPlatformCollisions();
  
  // Redraw platforms (they don't move so less flickering)
  drawPlatforms();
  // Draw balls at new positions
  drawDropBalls();
  
  lastUpdate = millis();
}

void checkPlatformCollisions() {
  for (int b = 0; b < MAX_DROP_BALLS; b++) {
    if (!dropBalls[b].active) continue;
    
    for (int p = 0; p < numPlatforms; p++) {
      // Simple rectangle collision
      if (dropBalls[b].x + dropBalls[b].size >= platforms[p].x &&
          dropBalls[b].x - dropBalls[b].size <= platforms[p].x + platforms[p].w &&
          dropBalls[b].y + dropBalls[b].size >= platforms[p].y &&
          dropBalls[b].y - dropBalls[b].size <= platforms[p].y + platforms[p].h &&
          dropBalls[b].vy > 0) { // Only if falling down
        
        // Bounce off platform
        dropBalls[b].y = platforms[p].y - dropBalls[b].size;
        dropBalls[b].vy = -abs(dropBalls[b].vy) * dropBalls[b].bounce;
        dropBalls[b].vx += platforms[p].angle * 1.5; // Platform angle affects bounce
        
        // Play platform note
        if (deviceConnected && !platforms[p].active) {
          sendMIDI(0x90, platforms[p].note, random(70, 110));
          sendMIDI(0x80, platforms[p].note, 0);
          
          platforms[p].active = true;
          platforms[p].activeTime = millis();
        }
        
        break;
      }
    }
  }
}

#endif