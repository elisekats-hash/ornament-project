
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// Hardware configuration
#define BUZZER_PIN 32

void buzzerSetup();

void buzzerUpdate();

void buzzerPlay();

void buzzerPause();

void buzzerResume();

void buzzerStop();

void buzzerToggle();

void buzzerVolumeUp();

void buzzerVolumeDown();

void buzzerNextSong();

#endif
