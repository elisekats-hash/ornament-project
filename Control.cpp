
#include <Arduino.h>
#include <IRremote.hpp>
#include "Control.h"

// =========================================================
// IR REMOTE - GLOBAL VARIABLES
// =========================================================

// Last successfully decoded command
const char* lastCommand = NULL;

// Timing for held-button repeats
unsigned long lastRepeatMillis = 0;

// Delay before allowing held buttons to repeat
const unsigned long REPEAT_INTERVAL = 180;

// =========================================================
// HELPER: CHECK IF BUTTON SUPPORTS HOLDING
// =========================================================

bool isRepeatableCommand(const char* command) {

  if (command == NULL) {
    return false;
  }

  return strcmp(command, "VOL+") == 0 ||
         strcmp(command, "VOL-") == 0 ||
         strcmp(command, "UP") == 0 ||
         strcmp(command, "DOWN") == 0;
}

// =========================================================
// HELPER: DECODE RAW IR DATA
// =========================================================

const char* decodeCommand(uint32_t rawData) {

  switch (rawData) {

    case 0xBA45FF00: return "POWER";

    case 0xB847FF00: return "FUNC/STOP";

    case 0xB946FF00: return "VOL+";

    case 0xBB44FF00: return "FAST BACK";

    case 0xBF40FF00: return "PAUSE";

    case 0xBC43FF00: return "FAST FORWARD";

    case 0xF807FF00: return "DOWN";

    case 0xEA15FF00: return "VOL-";

    case 0xF609FF00: return "UP";

    case 0xE619FF00: return "EQ";

    case 0xF20DFF00: return "ST/REPT";

    case 0xE916FF00: return "0";

    case 0xF30CFF00: return "1";

    case 0xE718FF00: return "2";

    case 0xA15EFF00: return "3";

    case 0xF708FF00: return "4";

    case 0xE31CFF00: return "5";

    case 0xA55AFF00: return "6";

    case 0xBD42FF00: return "7";

    case 0xAD52FF00: return "8";

    case 0xB54AFF00: return "9";

    default:
      return NULL;
  }
}

// =========================================================
// INITIALIZE IR RECEIVER
// =========================================================

void controlSetup() {

  IrReceiver.begin(SIGNAL_LINE, ENABLE_LED_FEEDBACK);

  lastCommand = NULL;
  lastRepeatMillis = 0;

  Serial.println("IR Receiver Initialized");
}

// =========================================================
// UPDATE IR RECEIVER
// =========================================================

const char* controlUpdate() {

  // No completed IR packet available
  if (!IrReceiver.decode()) {
    return NULL;
  }

  unsigned long now = millis();

  // Capture decoded data before resuming receiver
  uint32_t rawData = IrReceiver.decodedIRData.decodedRawData;

  bool isRepeat =
    (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);

  // Resume receiver as soon as possible
  IrReceiver.resume();

  // -------------------------------------------------------
  // CASE 1: HELD-BUTTON REPEAT PACKET
  // -------------------------------------------------------

  if (isRepeat) {

    // Ignore repeats from single-action buttons
    if (!isRepeatableCommand(lastCommand)) {
      return NULL;
    }

    // Only allow repeats at a controlled interval
    if (now - lastRepeatMillis < REPEAT_INTERVAL) {
      return NULL;
    }

    lastRepeatMillis = now;

    return lastCommand;
  }

  // -------------------------------------------------------
  // CASE 2: NEW BUTTON PRESS
  // -------------------------------------------------------

  const char* command = decodeCommand(rawData);

  if (command == NULL) {
    lastCommand = NULL;
    return NULL;
  }

  // Save command for possible future repeat packets
  lastCommand = command;

  lastRepeatMillis = now;

  return command;
}
