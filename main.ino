#include <Arduino.h>
#include "Control.h"
#include "Display.h"
#include "Buzzer.h"
#include <cstring>

// =========================================================
// GLOBAL VARIABLES - SYSTEM STATE
// =========================================================

// Power and animation state
bool powerOn = false;
bool paused = false;
bool autoCycle = false;

int activePattern = -1;  // -1 = home screen / no animation

// =========================================================
// GLOBAL VARIABLES - TIMING
// =========================================================

// OLED animation timing
unsigned long previousFrameMillis = 0;
const unsigned long frameInterval = 33;  // approximately 30 FPS

// External LED timing
unsigned long previousLEDMillis = 0;
const unsigned long LED_INTERVAL = 100;

// Automatic pattern cycling
unsigned long previousCycleMillis = 0;
const unsigned long CYCLE_INTERVAL = 10000;

// Physical button debounce
const unsigned long DEBOUNCE_DELAY = 50;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastDebounceMillis = 0;

// =========================================================
// GLOBAL VARIABLES - LED SETTINGS
// =========================================================

// These will be used when we implement updateLEDs()
int ledMode = 0;
int ledLevel = 8;

const int NUM_LED_MODES = 9;
const int MAX_LED_LEVEL = 8;

// =========================================================
// FUNCTION DECLARATIONS
// =========================================================

void checkPhysicalButton(unsigned long now);
void handleIRCommand(const char* command, unsigned long now);
void togglePower(unsigned long now);
void selectPattern(int pattern, unsigned long now);
void runActivePattern();
void showHomeScreen();
void updateExternalLEDs(unsigned long now);

// =========================================================
// SETUP
// =========================================================

void setup() {
  Serial.begin(115200);

  // Initialize hardware
  displaySetup();
  buttonSetup();
  controlSetup();
  setUpLEDs();
  buzzerSetup();

  // Initialize physical button state
  lastButtonReading = digitalRead(BUTTON_PIN);
  stableButtonState = lastButtonReading;

  // Start with everything off
  powerLED(0);
  clearOLED();

  Serial.println("Christmas Ornament Initialized!");
}

// =========================================================
// MAIN LOOP
// =========================================================

void loop() {
  unsigned long now = millis();

  // TASK 1: Physical power button
  checkPhysicalButton(now);

  // TASK 2: Read IR remote
  const char* command = controlUpdate();

  if (command != NULL) {
    handleIRCommand(command, now);
  }

  // Nothing else should run while powered off
  if (!powerOn) {
    return;
  }

  // TASK 3: Automatic pattern cycling
  if (autoCycle && !paused && activePattern != -1) {

    if (now - previousCycleMillis >= CYCLE_INTERVAL) {
      
      previousCycleMillis = now;

      int nextPattern = (activePattern + 1) % 10;
      
      selectPattern(nextPattern, now);
    }
  }

  // TASK 4: Update OLED animation
  if (!paused && activePattern != -1) {

    if (now - previousFrameMillis >= frameInterval) {
      previousFrameMillis = now;

      runActivePattern();
    }
  }

  // TASK 5: Update external LEDs
  if (!paused && now - previousLEDMillis >= LED_INTERVAL) {
    previousLEDMillis = now;

    updateExternalLEDs(now);
  }

  // TASK 6: Update music independently
  buzzerUpdate();
}

// =========================================================
// PHYSICAL BUTTON - DEBOUNCED
// =========================================================

void checkPhysicalButton(unsigned long now) {

  bool reading = digitalRead(BUTTON_PIN);

  // Detect any change in the raw button reading
  if (reading != lastButtonReading) {
    lastDebounceMillis = now;
  }

  // Only accept a state change after it remains stable
  if (now - lastDebounceMillis >= DEBOUNCE_DELAY) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      // INPUT_PULLUP: LOW means pressed
      if (stableButtonState == LOW) {
        togglePower(now);
      }
    }
  }

  lastButtonReading = reading;
}

// =========================================================
// POWER CONTROL
// =========================================================

void togglePower(unsigned long now) {

  powerOn = !powerOn;

  if (powerOn) {

    Serial.println("Power ON");

    powerLED(1);

    resetLEDPattern();

    paused = false;
    activePattern = -1;

    previousFrameMillis = now;
    previousLEDMillis = now;
    previousCycleMillis = now;

    showHomeScreen();

  } else {

    Serial.println("Power OFF");

    paused = false;
    autoCycle = false;
    activePattern = -1;

    clearOLED();
    powerLED(0);

    // Turn off all 8 LEDs immediately
    resetLEDPattern();

    // Stop music immediately
    buzzerStop();
  }
}

// =========================================================
// IR REMOTE COMMAND HANDLER
// =========================================================

void handleIRCommand(const char* command, unsigned long now) {

  Serial.print("IR Command: ");
  Serial.println(command);

  // POWER always works, even when the ornament is off
  if (strcmp(command, "POWER") == 0) {
    togglePower(now);
    return;
  }

  // Ignore other commands while powered off
  if (!powerOn) {
    return;
  }

  // -------------------------------------------------------
  // NUMBER BUTTONS: 0-9
  // -------------------------------------------------------

  if (strlen(command) == 1 &&
      command[0] >= '0' &&
      command[0] <= '9') {

    int pattern = command[0] - '0';

    selectPattern(pattern, now);
    return;
  }

  // -------------------------------------------------------
  // PAUSE / RESUME
  // -------------------------------------------------------

  if (strcmp(command, "PAUSE") == 0) {

    if (activePattern != -1) {

      paused = !paused;

      // Avoid a large timing gap on resume
      if (!paused) {
        previousFrameMillis = now;
        previousLEDMillis = now;
        previousCycleMillis = now;
      }

      Serial.println(paused ? "Paused" : "Resumed");
    }

    return;
  }

  // -------------------------------------------------------
  // NEXT PATTERN
  // -------------------------------------------------------

  if (strcmp(command, "FAST FORWARD") == 0) {

    int nextPattern = (activePattern + 1) % 10;

    selectPattern(nextPattern, now);
    return;
  }

  // -------------------------------------------------------
  // PREVIOUS PATTERN
  // -------------------------------------------------------

  if (strcmp(command, "FAST BACK") == 0) {

    int previousPattern;

    if (activePattern == -1) {
      previousPattern = 9;
    } else {
      previousPattern = (activePattern + 9) % 10;
    }

    selectPattern(previousPattern, now);
    return;
  }

  // -------------------------------------------------------
  // MUSIC VOLUME
  // -------------------------------------------------------
  if (strcmp(command, "VOL+") == 0) {

  buzzerVolumeUp();

  return;
  }

  if (strcmp(command, "VOL-") == 0) {

    buzzerVolumeDown();

    return;
  }
  // -------------------------------------------------------
  // EXTERNAL LED LEVEL
  // -------------------------------------------------------

  if (strcmp(command, "UP") == 0) {

    if (ledLevel < MAX_LED_LEVEL) {
      ledLevel++;
    }

    Serial.print("LED level: ");
    Serial.println(ledLevel);

    return;
  }

  if (strcmp(command, "DOWN") == 0) {

    if (ledLevel > 0) {
      ledLevel--;
    }

    Serial.print("LED level: ");
    Serial.println(ledLevel);

    return;
  }

  // -------------------------------------------------------
  // STOP ANIMATION / HOME SCREEN
  // -------------------------------------------------------

  if (strcmp(command, "FUNC/STOP") == 0) {

    activePattern = -1;
    paused = false;
    autoCycle = false;

    resetLEDPattern();

    showHomeScreen();

    buzzerStop();

    Serial.println("Animation stopped");
    return;
  }

  // -------------------------------------------------------
  // MUSIC PLAY / PAUSE / RESUME
  // -------------------------------------------------------

  if (strcmp(command, "ST/REPT") == 0) {

    buzzerToggle();
  }

  // -------------------------------------------------------
  // EXTERNAL LED EFFECT
  // -------------------------------------------------------

  if (strcmp(command, "EQ") == 0) {

    ledMode = (ledMode + 1) % NUM_LED_MODES;

    resetLEDPattern();
    previousLEDMillis = now;

    // Show first frame immediately
    updateExternalLEDs(now);

    Serial.print("LED Mode: ");
    Serial.println(ledMode);

    return;
  }
}

// =========================================================
// PATTERN SELECTION
// =========================================================

void selectPattern(int pattern, unsigned long now) {

  if (pattern < 0 || pattern > 9) {
    return;
  }

  activePattern = pattern;
  paused = false;

  // Reset all animation frame counters
  resetPatternState();

  resetLEDPattern();

  previousFrameMillis = now;
  previousLEDMillis = now;
  previousCycleMillis = now;

  Serial.print("Selected Pattern: ");
  Serial.println(activePattern);

  // Draw the first frame immediately
  runActivePattern();
}

// =========================================================
// RUN CURRENT ANIMATION
// =========================================================

void runActivePattern() {

  switch (activePattern) {

    case 0: pattern0(); break;
    case 1: pattern1(); break;
    case 2: pattern2(); break;
    case 3: pattern3(); break;
    case 4: pattern4(); break;
    case 5: pattern5(); break;
    case 6: pattern6(); break;
    case 7: pattern7(); break;
    case 8: pattern8(); break;
    case 9: pattern9(); break;

    default:
      break;
  }
}

// =========================================================
// HOME SCREEN
// =========================================================

void showHomeScreen() {

  displayTest("Merry Christmas!", 12, 18);
  displayOLED();

  // We can make this prettier later!
}

// =========================================================
// EXTERNAL LED UPDATE
// =========================================================

void updateExternalLEDs(unsigned long now) {

  // Placeholder until we implement the LED effects
  // in Display.cpp.

  // The LED timer already runs independently
  // of the OLED animation timer.

  (void)now;

  updateLEDPattern(ledMode, ledLevel);
}
