
#include <Arduino.h>
#include "Buzzer.h"
#include "pitches.h"

// =========================================================
// BUZZER CONFIGURATION
// =========================================================

const int PWM_RESOLUTION = 10;
const int MAX_PWM = (1 << PWM_RESOLUTION) - 1;

const int MAX_VOLUME = 10;

int buzzerVolume = 3;

// =========================================================
// MUSIC PLAYBACK STATE
// =========================================================

enum MusicState {
  MUSIC_STOPPED,
  MUSIC_PLAYING,
  MUSIC_PAUSED
};

MusicState musicState = MUSIC_STOPPED;

bool buzzerReady = false;

// Current song and note
int currentSong = 0;
int currentNote = 0;

// Current note timing
unsigned long noteStartMillis = 0;
unsigned long currentNoteDuration = 0;
unsigned long pausedElapsed = 0;

// Current note frequency
int currentFrequency = 0;

// Whether the current note is sounding
bool noteSounding = false;


// =========================================================
// SONG 1: JINGLE BELLS
// =========================================================

const int melody0[] = {

  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_E4, NOTE_E4,

  NOTE_E4, NOTE_G4, NOTE_C4,
  NOTE_D4, NOTE_E4,

  NOTE_F4, NOTE_F4, NOTE_F4,
  NOTE_F4, NOTE_F4,

  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4,

  NOTE_D4, NOTE_D4, NOTE_E4,
  NOTE_D4, NOTE_G4,

  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_E4, NOTE_E4,

  NOTE_E4, NOTE_G4, NOTE_C4,
  NOTE_D4, NOTE_E4,

  NOTE_F4, NOTE_F4, NOTE_F4,
  NOTE_F4, NOTE_F4,

  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4,

  NOTE_G4, NOTE_G4, NOTE_F4,
  NOTE_D4, NOTE_C4
};

const unsigned int durations0[] = {

  250, 250, 500,
  250, 250, 500,

  250, 250, 375,
  125, 1000,

  250, 250, 375,
  125, 250,

  250, 250, 125,
  125,

  250, 250, 250,
  500, 500,

  250, 250, 500,
  250, 250, 500,

  250, 250, 375,
  125, 1000,

  250, 250, 375,
  125, 250,

  250, 250, 125,
  125,

  250, 250, 250,
  250, 1000
};


// =========================================================
// SONG 2: WE WISH YOU A MERRY CHRISTMAS
// =========================================================

const int melody1[] = {

  NOTE_G4,

  NOTE_C5, NOTE_C5, NOTE_D5,
  NOTE_C5, NOTE_B4, NOTE_A4,

  NOTE_A4, NOTE_A4, NOTE_E5,
  NOTE_D5, NOTE_C5, NOTE_B4,

  NOTE_G4, NOTE_G4, NOTE_F5,
  NOTE_E5, NOTE_D5, NOTE_C5,

  NOTE_A4, NOTE_G4, NOTE_G4,
  NOTE_A4, NOTE_D5, NOTE_B4,

  NOTE_C5,

  NOTE_G4,

  NOTE_C5, NOTE_C5, NOTE_C5,
  NOTE_B4,

  NOTE_B4, NOTE_C5, NOTE_B4,
  NOTE_A4,

  NOTE_G4, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_C5,

  NOTE_G5, NOTE_G4, NOTE_G4,
  NOTE_A4, NOTE_D5, NOTE_B4,

  NOTE_C5
};

const unsigned int durations1[] = {

  300,

  300, 150, 150,
  150, 150, 300,

  300, 300, 300,
  150, 150, 300,

  300, 300, 300,
  150, 150, 300,

  300, 150, 150,
  300, 300, 300,

  900,

  300,

  300, 300, 300,
  600,

  300, 300, 300,
  600,

  300, 300, 150,
  150, 300,

  300, 150, 150,
  300, 300, 300,

  900
};


// =========================================================
// SONG 3: SILENT NIGHT
// =========================================================

const int melody2[] = {

  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,
  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,

  NOTE_D5, NOTE_D5, NOTE_B4,
  NOTE_C5, NOTE_C5, NOTE_G4,

  NOTE_A4, NOTE_A4, NOTE_C5,
  NOTE_B4, NOTE_A4,

  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,

  NOTE_A4, NOTE_A4, NOTE_C5,
  NOTE_B4, NOTE_A4,

  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,

  NOTE_D5, NOTE_D5, NOTE_F5,
  NOTE_D5, NOTE_B4, NOTE_C5,

  NOTE_E5,

  NOTE_C5, NOTE_G4, NOTE_E4,
  NOTE_G4, NOTE_F4, NOTE_D4,

  NOTE_C4
};

const unsigned int durations2[] = {

  600, 200, 400, 1200,
  600, 200, 400, 1200,

  800, 400, 1200,
  800, 400, 1200,

  800, 400, 600,
  200, 400,

  600, 200, 400, 1200,

  800, 400, 600,
  200, 400,

  600, 200, 400, 1200,

  800, 400, 600,
  200, 400, 1200,

  1200,

  400, 400, 400,
  600, 200, 400,

  1600
};


// =========================================================
// SONG MANAGEMENT
// =========================================================

struct Song {

  const char* name;

  const int* melody;

  const unsigned int* durations;

  int length;
};

// Verify matching melody and duration lengths
static_assert(
  sizeof(melody0) / sizeof(melody0[0]) ==
  sizeof(durations0) / sizeof(durations0[0]),
  "Song 1 length mismatch"
);

static_assert(
  sizeof(melody1) / sizeof(melody1[0]) ==
  sizeof(durations1) / sizeof(durations1[0]),
  "Song 2 length mismatch"
);

static_assert(
  sizeof(melody2) / sizeof(melody2[0]) ==
  sizeof(durations2) / sizeof(durations2[0]),
  "Song 3 length mismatch"
);

const Song songs[] = {

  {
    "Jingle Bells",
    melody0,
    durations0,
    sizeof(melody0) / sizeof(melody0[0])
  },

  {
    "We Wish You a Merry Christmas",
    melody1,
    durations1,
    sizeof(melody1) / sizeof(melody1[0])
  },

  {
    "Silent Night",
    melody2,
    durations2,
    sizeof(melody2) / sizeof(melody2[0])
  }
};

const int NUM_SONGS = sizeof(songs) / sizeof(songs[0]);


// =========================================================
// PWM VOLUME CONTROL
// =========================================================

unsigned int getVolumeDuty() {

  // Experimental duty-cycle levels: 0% to 50%
  const int dutyPercent[] = {
    0, 1, 2, 4, 7, 11,
    17, 24, 32, 41, 50
  };

  return (MAX_PWM * dutyPercent[buzzerVolume]) / 100;
}

// =========================================================
// SILENCE BUZZER OUTPUT
// =========================================================

void silenceBuzzer() {

  if (!buzzerReady) {
    return;
  }

  ledcWrite(BUZZER_PIN, 0);
}

// =========================================================
// GENERATE A MUSICAL NOTE
// =========================================================

void playFrequency(int frequency) {

  if (!buzzerReady) {
    return;
  }

  if (frequency <= 0 || buzzerVolume == 0) {

    silenceBuzzer();
    return;
  }

  // Configure musical frequency
  uint32_t actualFrequency = ledcChangeFrequency(
    BUZZER_PIN,
    frequency,
    PWM_RESOLUTION
  );

  if (actualFrequency == 0) {
    silenceBuzzer();
    return;
  }

  // Apply selected volume
  ledcWrite(BUZZER_PIN, getVolumeDuty());
}

// =========================================================
// INITIALIZE BUZZER
// =========================================================

void buzzerSetup() {

  buzzerReady = ledcAttach(
    BUZZER_PIN,
    1000,
    PWM_RESOLUTION
  );

  if (!buzzerReady) {

    Serial.println("Buzzer PWM initialization failed");
    return;
  }

  silenceBuzzer();

  musicState = MUSIC_STOPPED;

  Serial.println("Buzzer Initialized");
}


// =========================================================
// START CURRENT NOTE
// =========================================================

void startCurrentNote() {

  currentFrequency =
    songs[currentSong].melody[currentNote];

  currentNoteDuration =
    songs[currentSong].durations[currentNote];

  noteStartMillis = millis();

  pausedElapsed = 0;

  noteSounding = currentFrequency > 0;

  playFrequency(currentFrequency);
}

// =========================================================
// UPDATE MUSIC - NON-BLOCKING
// =========================================================

void buzzerUpdate() {

  if (musicState != MUSIC_PLAYING) {
    return;
  }

  unsigned long now = millis();

  unsigned long elapsed =
    now - noteStartMillis;

  // Play note for 85% of its duration
  unsigned long soundDuration =
    (currentNoteDuration * 85) / 100;

  // Silence during remaining 15%
  if (noteSounding && elapsed >= soundDuration) {

    silenceBuzzer();

    noteSounding = false;
  }

  // Check if the note has finished
  if (elapsed >= currentNoteDuration) {

    currentNote++;

    // Check if the entire song has finished
    if (currentNote >= songs[currentSong].length) {

      Serial.println("Song Finished");

      buzzerStop();

      // Prepare next song without playing it
      buzzerNextSong();

      return;
    }

    // Begin next musical note
    startCurrentNote();
  }
}

// =========================================================
// PLAY MUSIC
// =========================================================

void buzzerPlay() {

  if (!buzzerReady) {
    return;
  }

  currentNote = 0;

  musicState = MUSIC_PLAYING;

  Serial.print("Playing: ");
  Serial.println(songs[currentSong].name);

  startCurrentNote();
}

// =========================================================
// PAUSE MUSIC
// =========================================================

void buzzerPause() {

  if (musicState != MUSIC_PLAYING) {
    return;
  }

  pausedElapsed = min(
    millis() - noteStartMillis,
    currentNoteDuration
  );

  musicState = MUSIC_PAUSED;

  silenceBuzzer();

  noteSounding = false;

  Serial.println("Music Paused");
}

// =========================================================
// RESUME MUSIC
// =========================================================

void buzzerResume() {

  if (musicState != MUSIC_PAUSED) {
    return;
  }

  musicState = MUSIC_PLAYING;

  // Restore position within current note
  noteStartMillis = millis() - pausedElapsed;

  unsigned long soundDuration =
    (currentNoteDuration * 85) / 100;

  // Resume note only if within its sounding period
  if (pausedElapsed < soundDuration &&
      currentFrequency > 0) {

    playFrequency(currentFrequency);

    noteSounding = true;

  } else {

    silenceBuzzer();

    noteSounding = false;
  }

  Serial.println("Music Resumed");
}

// =========================================================
// STOP MUSIC
// =========================================================

void buzzerStop() {

  silenceBuzzer();

  musicState = MUSIC_STOPPED;

  currentNote = 0;

  currentFrequency = 0;
  currentNoteDuration = 0;

  pausedElapsed = 0;

  noteSounding = false;

  Serial.println("Music Stopped");
}

// =========================================================
// TOGGLE MUSIC - ST/REPT BUTTON
// =========================================================

void buzzerToggle() {

  switch (musicState) {

    case MUSIC_STOPPED:

      buzzerPlay();

      break;

    case MUSIC_PLAYING:

      buzzerPause();

      break;

    case MUSIC_PAUSED:

      buzzerResume();

      break;
  }
}

// =========================================================
// SELECT NEXT SONG
// =========================================================

void buzzerNextSong() {

  if (musicState != MUSIC_STOPPED) {
    buzzerStop();
  }

  currentSong = (currentSong + 1) % NUM_SONGS;

  currentNote = 0;

  Serial.print("Next Song: ");
  Serial.println(songs[currentSong].name);
}


// =========================================================
// INCREASE VOLUME
// =========================================================

void buzzerVolumeUp() {

  if (buzzerVolume < MAX_VOLUME) {
    buzzerVolume++;
  }

  // Apply immediately if a note is sounding
  if (buzzerReady &&
      musicState == MUSIC_PLAYING &&
      noteSounding) {

    ledcWrite(BUZZER_PIN, getVolumeDuty());
  }

  Serial.print("Music Volume: ");
  Serial.println(buzzerVolume);
}

// =========================================================
// DECREASE VOLUME
// =========================================================

void buzzerVolumeDown() {

  if (buzzerVolume > 0) {
    buzzerVolume--;
  }

  // Apply immediately if a note is sounding
  if (buzzerReady &&
      musicState == MUSIC_PLAYING &&
      noteSounding) {

    if (buzzerVolume == 0) {

      silenceBuzzer();

    } else {

      ledcWrite(BUZZER_PIN, getVolumeDuty());
    }
  }

  Serial.print("Music Volume: ");
  Serial.println(buzzerVolume);
}

