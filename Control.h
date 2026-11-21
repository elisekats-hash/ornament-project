#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

// IR receiver GPIO
#define SIGNAL_LINE 27

// Initialize IR receiver
void controlSetup();

// Returns a command when a valid button event occurs.
// Returns NULL when there is no new event.
const char* controlUpdate();

#endif
