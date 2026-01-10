#ifndef BOUNCING_BALL_MODE_H
#define BOUNCING_BALL_MODE_H

#include "common_definitions.h"
#include "ui_elements.h"
#include "midi_utils.h"

// Pong-style Ambient MIDI mode variables
struct Ball {
  float x, y;
  float vx, vy;
  uint16_t color;
  int size;
  bool active;
};

#define MAX_BALLS 4
Ball balls[MAX_BALLS];
int numActiveBalls = 1;

// Simple wall system - notes triggered by wall hits
struct Wall {
  int x, y, w, h;
  int note;
  String noteName;
  uint16_t color;
  bool active;
  unsigned long activeTime;
  int side; // 0=top, 1=right, 2=bottom, 3=left
};

#define NUM_WALLS 24  // 8 top + 8 bottom + 4 left + 4 right
Wall walls[NUM_WALLS];
int ballScale = 0;  // Scale selection
int ballKey = 0;    // Key selection
int ballOctave = 4;

// Function declarations
void initializeBouncingBallMode();
void drawBouncingBallMode();
void handleBouncingBallMode();
void initializeBalls();
void initializeWalls();
void updateBouncingBall();
void updateBalls();
void drawBalls();
void drawWalls();
void checkWallCollisions();

// Implementations
void initializeBouncingBallMode() {
  ballScale = 0;
  ballKey = 0;
  ballOctave = 4;
  numActiveBalls = 1;
  initializeBalls();
  initializeWalls();
}

void drawBouncingBallMode() {
  tft.fillScreen(THEME_BG);
  drawHeader("ZEN", "Ambient Bouncing");
  
  // Controls
  drawRoundButton(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "ADD", THEME_SUCCESS);
  drawRoundButton(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "RESET", THEME_WARNING);
  drawRoundButton(SCALE_X(110), SCALE_Y(200), BTN_MEDIUM_W, BTN_SMALL_H, "SCALE", THEME_ACCENT);
  drawRoundButton(SCALE_X(170), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "KEY-", THEME_SECONDARY);
  drawRoundButton(SCALE_X(220), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "KEY+", THEME_SECONDARY);
  drawRoundButton(SCALE_X(270), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H, "OCT", THEME_PRIMARY);
  
  // Status display
  tft.setTextColor(THEME_TEXT_DIM, THEME_BG);
  String keyName = getNoteNameFromMIDI(ballKey);
  tft.drawString(keyName + " " + scales[ballScale].name, MARGIN_SMALL, SCALE_Y(180), 1);
  tft.drawString("Oct:" + String(ballOctave), SCALE_X(150), SCALE_Y(180), 1);
  tft.drawString("Balls:" + String(numActiveBalls), SCALE_X(270), SCALE_Y(180), 1);
  
  drawWalls();
  drawBalls();
}

void initializeBalls() {
  for (int i = 0; i < MAX_BALLS; i++) {
    balls[i].x = random(SCALE_X(80), SCALE_X(240));
    balls[i].y = random(SCALE_Y(80), SCALE_Y(150));
    // Slower, more zen-like movement (scale velocity for different screens)
    float velocityScale = (displayConfig.scaleX + displayConfig.scaleY) / 2.0;
    balls[i].vx = random(-15, 15) / 10.0 * velocityScale; // -1.5 to 1.5
    balls[i].vy = random(-15, 15) / 10.0 * velocityScale;
    if (abs(balls[i].vx) < 0.5 * velocityScale) balls[i].vx = (balls[i].vx >= 0) ? 0.8 * velocityScale : -0.8 * velocityScale;
    if (abs(balls[i].vy) < 0.5 * velocityScale) balls[i].vy = (balls[i].vy >= 0) ? 0.8 * velocityScale : -0.8 * velocityScale;
    // Softer, more zen colors
    balls[i].color = random(0x2000, 0x8FFF);
    balls[i].size = random(4, 7) * displayConfig.scaleX;
    balls[i].active = (i < numActiveBalls);
  }
}

void initializeWalls() {
  int wallIndex = 0;
  
  // Top wall - 8 segments
  for (int i = 0; i < 8; i++) {
    walls[wallIndex].x = SCALE_X(50 + i * 28);
    walls[wallIndex].y = HEADER_HEIGHT + SCALE_Y(15);
    walls[wallIndex].w = SCALE_X(28);
    walls[wallIndex].h = SCALE_Y(3);
    walls[wallIndex].note = getNoteInScale(ballScale, i, ballOctave) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_PRIMARY;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 0;
    wallIndex++;
  }
  
  // Right wall - 4 segments
  for (int i = 0; i < 4; i++) {
    walls[wallIndex].x = DISPLAY_WIDTH - SCALE_X(48);
    walls[wallIndex].y = HEADER_HEIGHT + SCALE_Y(18 + i * 28);
    walls[wallIndex].w = SCALE_X(3);
    walls[wallIndex].h = SCALE_Y(28);
    walls[wallIndex].note = getNoteInScale(ballScale, i, ballOctave + 1) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_SECONDARY;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 1;
    wallIndex++;
  }
  
  // Bottom wall - 8 segments
  for (int i = 0; i < 8; i++) {
    walls[wallIndex].x = SCALE_X(50 + i * 28);
    walls[wallIndex].y = SCALE_Y(177);
    walls[wallIndex].w = SCALE_X(28);
    walls[wallIndex].h = SCALE_Y(3);
    walls[wallIndex].note = getNoteInScale(ballScale, 7 - i, ballOctave) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_ACCENT;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 2;
    wallIndex++;
  }
  
  // Left wall - 4 segments
  for (int i = 0; i < 4; i++) {
    walls[wallIndex].x = SCALE_X(50);
    walls[wallIndex].y = HEADER_HEIGHT + SCALE_Y(18 + i * 28);
    walls[wallIndex].w = SCALE_X(3);
    walls[wallIndex].h = SCALE_Y(28);
    walls[wallIndex].note = getNoteInScale(ballScale, 3 - i, ballOctave + 1) + ballKey;
    walls[wallIndex].noteName = getNoteNameFromMIDI(walls[wallIndex].note);
    walls[wallIndex].color = THEME_WARNING;
    walls[wallIndex].active = false;
    walls[wallIndex].side = 3;
    wallIndex++;
  }
}

void handleBouncingBallMode() {
  // Back button
  if (touch.justPressed && isButtonPressed(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_H)) {
    exitToMenu();
    return;
  }
  
  if (touch.justPressed) {
    // Add ball button
    if (isButtonPressed(SCALE_X(10), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      if (numActiveBalls < MAX_BALLS) {
        numActiveBalls++;
        initializeBalls();
        drawBouncingBallMode();
      }
      return;
    }
    
    // Reset button
    if (isButtonPressed(SCALE_X(60), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      numActiveBalls = 1;
      initializeBalls();
      drawBouncingBallMode();
      return;
    }
    
    // Scale button
    if (isButtonPressed(SCALE_X(110), SCALE_Y(200), BTN_MEDIUM_W, BTN_SMALL_H)) {
      ballScale = (ballScale + 1) % NUM_SCALES;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    // Key controls
    if (isButtonPressed(SCALE_X(170), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      ballKey = (ballKey - 1 + 12) % 12;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    if (isButtonPressed(SCALE_X(220), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      ballKey = (ballKey + 1) % 12;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
    
    // Octave button
    if (isButtonPressed(SCALE_X(270), SCALE_Y(200), BTN_SMALL_W, BTN_SMALL_H)) {
      ballOctave = (ballOctave == 7) ? 2 : ballOctave + 1;
      initializeWalls();
      drawBouncingBallMode();
      return;
    }
  }
  
  // Update physics and display
  updateBouncingBall();
}

void updateBouncingBall() {
  // Smooth 60 FPS animation
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 16) {
    // Clear entire play area to prevent flickering
    int playAreaX = SCALE_X(53);
    int playAreaY = HEADER_HEIGHT + SCALE_Y(18);
    int playAreaW = SCALE_X(219);
    int playAreaH = SCALE_Y(114);
    tft.fillRect(playAreaX, playAreaY, playAreaW, playAreaH, THEME_BG);
    
    updateBalls();
    checkWallCollisions();
    
    // Draw walls
    drawWalls();
    
    // Draw balls
    drawBalls();
    
    lastUpdate = millis();
  }
}

void updateBalls() {
  int minX = SCALE_X(53);
  int maxX = DISPLAY_WIDTH - SCALE_X(48);
  int minY = HEADER_HEIGHT + SCALE_Y(18);
  int maxY = SCALE_Y(177);
  
  for (int i = 0; i < numActiveBalls; i++) {
    if (!balls[i].active) continue;
    
    // Update position
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;
    
    // Bounce off walls with proper collision detection
    if (balls[i].x - balls[i].size <= minX) {
      balls[i].vx = abs(balls[i].vx);
      balls[i].x = minX + balls[i].size;
    }
    if (balls[i].x + balls[i].size >= maxX) {
      balls[i].vx = -abs(balls[i].vx);
      balls[i].x = maxX - balls[i].size;
    }
    if (balls[i].y - balls[i].size <= minY) {
      balls[i].vy = abs(balls[i].vy);
      balls[i].y = minY + balls[i].size;
    }
    if (balls[i].y + balls[i].size >= maxY) {
      balls[i].vy = -abs(balls[i].vy);
      balls[i].y = maxY - balls[i].size;
    }
  }
}

void drawBalls() {
  for (int i = 0; i < numActiveBalls; i++) {
    if (!balls[i].active) continue;
    tft.fillCircle(balls[i].x, balls[i].y, balls[i].size, balls[i].color);
    tft.drawCircle(balls[i].x, balls[i].y, balls[i].size, THEME_TEXT);
  }
}

void drawWalls() {
  for (int i = 0; i < NUM_WALLS; i++) {
    uint16_t color = walls[i].color;
    
    // Bright flash when active
    if (walls[i].active) {
      unsigned long elapsed = millis() - walls[i].activeTime;
      if (elapsed < 200) {
        color = THEME_TEXT; // Bright white flash
      } else {
        walls[i].active = false;
      }
    }
    
    // Draw wall
    tft.fillRect(walls[i].x, walls[i].y, walls[i].w, walls[i].h, color);
    
    // Add note name for longer walls
    if (walls[i].w > walls[i].h && walls[i].w > 50) {
      tft.setTextColor(THEME_BG, color);
      tft.drawCentreString(walls[i].noteName, 
                          walls[i].x + walls[i].w/2, 
                          walls[i].y - 2, 1);
    }
  }
}


void checkWallCollisions() {
  for (int b = 0; b < numActiveBalls; b++) {
    if (!balls[b].active) continue;
    
    static float lastX[MAX_BALLS], lastY[MAX_BALLS];
    static bool initialized = false;
    
    if (!initialized) {
      for (int i = 0; i < MAX_BALLS; i++) {
        lastX[i] = balls[i].x;
        lastY[i] = balls[i].y;
      }
      initialized = true;
    }
    
    // Check collision with each wall segment
    for (int w = 0; w < NUM_WALLS; w++) {
      if (walls[w].active) continue; // Skip if wall is already active
      
      bool collision = false;
      
      // Check collision based on ball position and wall bounds
      if (walls[w].side == 0) { // Top walls
        if (balls[b].y - balls[b].size <= walls[w].y + walls[w].h &&
            balls[b].x >= walls[w].x && balls[b].x <= walls[w].x + walls[w].w &&
            lastY[b] > balls[b].y) {
          collision = true;
        }
      }
      else if (walls[w].side == 1) { // Right walls
        if (balls[b].x + balls[b].size >= walls[w].x &&
            balls[b].y >= walls[w].y && balls[b].y <= walls[w].y + walls[w].h &&
            lastX[b] < balls[b].x) {
          collision = true;
        }
      }
      else if (walls[w].side == 2) { // Bottom walls
        if (balls[b].y + balls[b].size >= walls[w].y &&
            balls[b].x >= walls[w].x && balls[b].x <= walls[w].x + walls[w].w &&
            lastY[b] < balls[b].y) {
          collision = true;
        }
      }
      else if (walls[w].side == 3) { // Left walls
        if (balls[b].x - balls[b].size <= walls[w].x + walls[w].w &&
            balls[b].y >= walls[w].y && balls[b].y <= walls[w].y + walls[w].h &&
            lastX[b] > balls[b].x) {
          collision = true;
        }
      }
      
      if (collision) {
        if (deviceConnected) {
          sendMIDI(0x90, walls[w].note, random(70, 110));
          sendMIDI(0x80, walls[w].note, 0);
        }
        
        walls[w].active = true;
        walls[w].activeTime = millis();
        
        Serial.printf("Wall segment hit: %s\n", walls[w].noteName.c_str());
        break; // Only trigger one wall per ball per frame
      }
    }
    
    // Update last positions
    lastX[b] = balls[b].x;
    lastY[b] = balls[b].y;
  }
}

#endif