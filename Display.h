
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// 74HC595 shift register
#define LATCH_PIN 33
#define CLOCK_PIN 26
#define DATA_PIN 25

// Physical controls
#define POWER_LED 5
#define BUTTON_PIN 14

#define WHITE SH110X_WHITE

// =========================================================
// OLED DISPLAY
// =========================================================

void displaySetup();
void displayTest(const char* msg, int x, int y);
void displayOLED();
void clearOLED();

// =========================================================
// EXTERNAL LEDs - 74HC595
// =========================================================

void setUpLEDs();
void updateShiftRegister();
void updateLEDs();

// NEW: Update LED animation using current settings
void updateLEDPattern(int mode, int level);

// NEW: Reset LED animation to its first frame
void resetLEDPattern();

// Existing indicator LED
void powerLED(int on);

// =========================================================
// ANIMATION HELPERS
// =========================================================

void resetPatternState();

void drawFirework(int startFrame, int launchX, int peakY);
void captureParticles4();
void drawMessage4();

// =========================================================
// OLED ANIMATIONS
// =========================================================

void pattern0();
void pattern1();
void pattern2();
void pattern3();
void pattern4();
void pattern5();
void pattern6();
void pattern7();
void pattern8();
void pattern9();

// =========================================================
// PHYSICAL BUTTON
// =========================================================

void buttonSetup();

#endif
