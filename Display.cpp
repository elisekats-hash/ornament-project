#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Display.h>
#include <math.h>
#include <stdlib.h>

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int frame0 = 0;
int frame1 = 0;
int frame2 = 0;
int frame3 = 0;
int frame4 = 0;
int frame5 = 0;
int frame6 = 0;
int frame7 = 0;
int frame8 = 0;
int frame9 = 0;

bool particlesCaptured4 = false;

const int HOLD4 = 40;      // frames text stays static
const int EXPLODE4 = 35;   // frames the explosion lasts
const int PAUSE4 = 15;     // blank pause before repeat
const int CYCLE4 = HOLD4 + EXPLODE4 + PAUSE4;

const int MAX_PARTICLES4 = 80;

struct Particle4 {
  int x, y;       // original position
  float vx, vy;   // velocity
};

Particle4 particles4[MAX_PARTICLES4];
int numParticles4 = 0;

void drawMessage4() {
  display.setTextSize(1);
  display.setTextColor(WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  const char* line1 = "Merry";
  const char* line2 = "Christmas";

  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  int line1X = (SCREEN_WIDTH - w) / 2;
  int line1Y = SCREEN_HEIGHT / 2 - h - 2;

  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  int line2X = (SCREEN_WIDTH - w) / 2;
  int line2Y = SCREEN_HEIGHT / 2 + 2;

  display.setCursor(line1X, line1Y);
  display.print(line1);

  display.setCursor(line2X, line2Y);
  display.print(line2);
}

void captureParticles4() {
  numParticles4 = 0;
  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;

  // Sample every 2nd pixel to keep particle count manageable
  for (int y = 0; y < SCREEN_HEIGHT && numParticles4 < MAX_PARTICLES4; y += 2) {
    for (int x = 0; x < SCREEN_WIDTH && numParticles4 < MAX_PARTICLES4; x += 2) {
      if (display.getPixel(x, y)) {
        float dx = x - cx;
        float dy = y - cy;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < 0.1) dist = 0.1; // avoid divide-by-zero at dead center

        float speed = 0.8 + (rand() % 100) / 100.0; // 0.8–1.8 px/frame, slight variety
        particles4[numParticles4].x = x;
        particles4[numParticles4].y = y;
        particles4[numParticles4].vx = (dx / dist) * speed;
        particles4[numParticles4].vy = (dy / dist) * speed;
        numParticles4++;
      }
    }
  }
}


void displaySetup(){
  display.begin(0x3C, true); // address + reset
  display.clearDisplay();  
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void displayTest(const char* msg, int x, int y){
  display.clearDisplay();
  display.setCursor(x, y);
  display.print(msg);
}

void displayOLED(){
  display.display();
}

void clearOLED(){
  display.clearDisplay();
  display.display();
}


// =========================================================
// SHIFT REGISTER + EXTERNAL LED CONTROL
// =========================================================

// Byte representing the eight LED states
// bit 0 = Q0, bit 7 = Q7
byte leds = 0;

// LED animation frame counter
unsigned int ledFrame = 0;

// =========================================================
// INITIALIZE LEDs
// =========================================================

void setUpLEDs() {

  pinMode(POWER_LED, OUTPUT);

  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  digitalWrite(POWER_LED, LOW);
  digitalWrite(LATCH_PIN, HIGH);

  // Start with all LEDs off
  leds = 0;
  updateShiftRegister();
}

// =========================================================
// WRITE BYTE TO SHIFT REGISTER
// =========================================================

void updateShiftRegister() {

  digitalWrite(LATCH_PIN, LOW);

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leds);

  digitalWrite(LATCH_PIN, HIGH);
}

// =========================================================
// WRITE CURRENT LED STATES
// =========================================================

void updateLEDs() {

  updateShiftRegister();
}

// =========================================================
// RESET LED ANIMATION
// =========================================================

void resetLEDPattern() {

  ledFrame = 0;
  leds = 0;

  updateShiftRegister();
}

// =========================================================
// GENERATE LED ANIMATION FRAME
// =========================================================


void updateLEDPattern(int mode, int level) {

  level = constrain(level, 0, 8);

  // All LEDs OFF
  if (mode == 0 || level == 0) {
    leds = 0;
    updateShiftRegister();
    return;
  }

  // Mask controlling number of participating LEDs
  byte mask = (1U << level) - 1U;

  switch (mode) {

    // =====================================================
    // MODE 1: CLOCKWISE CHASE
    // =====================================================

    case 1: {

      int position = ledFrame % level;

      leds = (1U << position);

      break;
    }

    // =====================================================
    // MODE 2: ALTERNATING RED AND GREEN
    // =====================================================

    case 2: {

      byte redMask   = 0b00100010;
      byte greenMask = 0b01000100;

      leds = (ledFrame % 2 == 0) ? redMask : greenMask;

      break;
    }

    // =====================================================
    // MODE 3: FILL AND EMPTY
    // =====================================================

    case 3: {

      int phase = ledFrame % (2 * level);
      int count;

      if (phase <= level) {
        count = phase;
      } else {
        count = 2 * level - phase;
      }

      leds = (1U << count) - 1U;

      break;
    }

    // =====================================================
    // MODE 4: ALL LEDs ON
    // =====================================================

    case 4: {

      leds = 0b11111111;

      break;
    }

    // =====================================================
    // MODE 5: BACK-AND-FORTH CHASE
    // =====================================================

    case 5: {

      if (level == 1) {
        leds = 0b00000001;
        break;
      }

      int cycle = 2 * level - 2;
      int position = ledFrame % cycle;

      if (position >= level) {
        position = cycle - position;
      }

      leds = (1U << position);

      break;
    }

    // =====================================================
    // MODE 6: COLOUR ROTATION
    // =====================================================

    case 6: {

      const byte colours[] = {
        0b00010001,  // White
        0b00100010,  // Red
        0b01000100,  // Green
        0b10001000   // Yellow
      };

      // Hold each colour for 300 ms
      leds = colours[(ledFrame / 3) % 4];

      break;
    }

    // =====================================================
    // MODE 7: ALTERNATING GROUPS
    // =====================================================

    case 7: {

      // White + Green, then Red + Yellow

      if ((ledFrame / 3) % 2 == 0) {
        leds = 0b01010101;
      } else {
        leds = 0b10101010;
      }

      break;
    }

    // =====================================================
    // MODE 8: RANDOM TWINKLE
    // =====================================================

    case 8: {

      leds = 0;

      for (int i = 0; i < level; i++) {

        // 30% probability of each LED being ON
        if (random(0, 100) < 30) {
          leds |= (1U << i);
        }
      }

      break;
    }

    default:

      leds = 0;
      break;
  }

  // Apply LED level to all patterns
  leds &= mask;

  // Send new LED states to shift register
  updateShiftRegister();

  // Advance animation frame
  ledFrame++;
}



// Tune these to change the look/feel
const int NUM_PARTICLES = 10;
const int RISE_DURATION = 20;    // frames for the rocket to climb
const int EXPLODE_DURATION = 25; // frames the burst lasts
const int CYCLE_LENGTH = RISE_DURATION + EXPLODE_DURATION + 15; // +pause before repeat

void drawFirework(int startFrame, int launchX, int peakY) {
  int localFrame = (frame5 - startFrame) % CYCLE_LENGTH;
  if (localFrame < 0) localFrame += CYCLE_LENGTH; // handle negative modulo

  if (localFrame < RISE_DURATION) {
    // --- Rising rocket ---
    int y = SCREEN_HEIGHT - (localFrame * (SCREEN_HEIGHT - peakY) / RISE_DURATION);
    display.drawPixel(launchX, y, WHITE);

  } else if (localFrame < RISE_DURATION + EXPLODE_DURATION) {
    // --- Explosion ---
    int burstFrame = localFrame - RISE_DURATION;
    float radius = burstFrame * 1.3; // how fast the burst expands

    for (int i = 0; i < NUM_PARTICLES; i++) {
      float angle = (2 * PI * i) / NUM_PARTICLES;

      // gravity pulls particles down slightly over time
      int px = launchX + (int)(cos(angle) * radius);
      int py = peakY + (int)(sin(angle) * radius) + (burstFrame * burstFrame) / 12;

      if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
        display.drawPixel(px, py, WHITE);
      }
    }
  }
  // else: dark pause before next cycle, draw nothing
}

//==============================================================================
//                                PATTERNS
//===============================================================================

// falling snowflakes
void pattern0(){

  display.clearDisplay();

  int yOffset = frame0 % SCREEN_HEIGHT;

  // snowflake 1
  display.drawPixel(0, (1+ yOffset) % SCREEN_HEIGHT, WHITE);
  // snowflake 2
  display.drawPixel(40, (7+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(40, (6+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(40, (5+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(40, (4+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(40, (3+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(41, (5+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(42, (5+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(39, (5+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(38, (5+ yOffset) % SCREEN_HEIGHT,WHITE);

  // snowflakes
  display.drawPixel(100, (10+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(100, (8+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(100, (9+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(100, (12+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(100, (11+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(98, (10+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(99, (10+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(101, (10+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(102, (10+ yOffset) % SCREEN_HEIGHT,WHITE);
  

  display.drawPixel(23, (48+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(113, (56+ yOffset) % SCREEN_HEIGHT,WHITE);
  display.drawPixel(50, (50+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(12, (13+ yOffset) % SCREEN_HEIGHT, WHITE);

  //snowflake
  display.drawPixel(89, (35+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(95, (5 + yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(67, (32+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(109, (20+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(27, (56 + yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(43, (60+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(23, (19+ yOffset) % SCREEN_HEIGHT, WHITE);
  display.drawPixel(45, (45+ yOffset) % SCREEN_HEIGHT, WHITE);

  displayOLED();

  frame0++;
}

// waves
void pattern1(){
  display.clearDisplay();

  float phase = frame1 * 0.15;

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    int y1 = (SCREEN_HEIGHT / 2) + (int)(sin((x * 0.2) + phase) * 10);
    int y2 = (SCREEN_HEIGHT / 2) + (int)(sin((x * 0.15) + phase * 1.5) * 6);
    display.drawPixel(x, y1, WHITE);
    display.drawPixel(x, y2, WHITE);
  }

  displayOLED();

  frame1++;
}

// Christmas Tree
void pattern2() {

  display.clearDisplay();

  int cx = SCREEN_WIDTH / 2;

  // Christmas tree
  display.drawTriangle(cx, 12, cx - 15, 31,
                       cx + 15, 31, WHITE);

  display.drawTriangle(cx, 22, cx - 21, 43,
                       cx + 21, 43, WHITE);

  display.drawTriangle(cx, 33, cx - 28, 55,
                       cx + 28, 55, WHITE);

  // Tree trunk
  display.drawRect(cx - 4, 55, 8, 8, WHITE);

  // Twinkling star
  int starSize = 3 + (frame2 / 6) % 2;

  display.drawLine(cx - starSize, 7,
                   cx + starSize, 7, WHITE);

  display.drawLine(cx, 7 - starSize,
                   cx, 7 + starSize, WHITE);

  display.drawLine(cx - 2, 5, cx + 2, 9, WHITE);
  display.drawLine(cx + 2, 5, cx - 2, 9, WHITE);

  // Animated tree decorations
  const int ornamentX[] = {64, 56, 71, 49, 64, 76, 59};
  const int ornamentY[] = {22, 32, 35, 45, 43, 48, 51};

  for (int i = 0; i < 7; i++) {

    if ((frame2 / 8 + i) % 3 != 0) {
      display.fillCircle(
        ornamentX[i], ornamentY[i], 1, WHITE
      );
    }
  }

  // Christmas presents
  display.drawRect(16, 50, 13, 11, WHITE);
  display.drawFastVLine(22, 50, 11, WHITE);
  display.drawFastHLine(16, 54, 13, WHITE);

  display.drawRect(99, 48, 16, 13, WHITE);
  display.drawFastVLine(107, 48, 13, WHITE);
  display.drawFastHLine(99, 53, 16, WHITE);

  // Falling snow on either side
  for (int i = 0; i < 12; i++) {

    int x = (i * 31 + 7) % SCREEN_WIDTH;
    int y = (i * 17 + frame2 / 2) % SCREEN_HEIGHT;

    if (x < 35 || x > 93) {
      display.drawPixel(x, y, WHITE);
    }
  }

  displayOLED();

  frame2++;
}

// Snowman
void pattern3() {

  display.clearDisplay();

  int cx = SCREEN_WIDTH / 2;

  // Body and head
  display.drawCircle(cx, 48, 14, WHITE);
  display.drawCircle(cx, 25, 10, WHITE);

  // Top hat
  display.fillRect(cx - 7, 5, 14, 10, WHITE);
  display.drawFastHLine(cx - 13, 15, 27, WHITE);

  // Eyes
  display.fillCircle(cx - 4, 22, 1, WHITE);
  display.fillCircle(cx + 4, 22, 1, WHITE);

  // Carrot nose
  display.drawTriangle(
    cx + 1, 25,
    cx + 9, 27,
    cx + 1, 28,
    WHITE
  );

  // Smile
  display.drawPixel(cx - 4, 29, WHITE);
  display.drawPixel(cx - 2, 31, WHITE);
  display.drawPixel(cx,     32, WHITE);
  display.drawPixel(cx + 2, 31, WHITE);
  display.drawPixel(cx + 4, 29, WHITE);

  // Buttons
  for (int i = 0; i < 3; i++) {
    display.fillCircle(cx, 42 + i * 6, 1, WHITE);
  }

  // Scarf
  display.fillRect(cx - 9, 34, 18, 4, WHITE);

  int scarfOffset = (int)(sin(frame3 * 0.15) * 4);

  display.drawLine(cx + 7, 36,
                   cx + 10 + scarfOffset, 48, WHITE);

  display.drawLine(cx + 8, 36,
                   cx + 11 + scarfOffset, 48, WHITE);

  // Left arm
  display.drawLine(cx - 12, 42, cx - 28, 28, WHITE);
  display.drawLine(cx - 28, 28, cx - 33, 27, WHITE);
  display.drawLine(cx - 28, 28, cx - 27, 22, WHITE);

  // Animated right arm
  int armX = cx + 30;
  int armY = 28 + (int)(sin(frame3 * 0.13) * 9);

  display.drawLine(cx + 12, 42, armX, armY, WHITE);
  display.drawLine(armX, armY, armX + 5, armY + 1, WHITE);
  display.drawLine(armX, armY, armX, armY - 6, WHITE);

  // Falling snow
  for (int i = 0; i < 15; i++) {

    int x = (i * 29 + 3) % SCREEN_WIDTH;
    int y = (i * 13 + frame3 / 2) % SCREEN_HEIGHT;

    if (x < 30 || x > 98) {
      display.drawPixel(x, y, WHITE);
    }
  }

  displayOLED();

  frame3++;
}

// Merry Christmas
void pattern4(){
  int localFrame = frame4 % CYCLE4;

  if (localFrame < HOLD4) {
    // --- Phase 1: static message ---
    display.clearDisplay();
    drawMessage4();
    displayOLED();
    particlesCaptured4 = false; // ready to re-capture next time we explode

  } else if (localFrame < HOLD4 + EXPLODE4) {
    // --- Phase 2: explosion ---
    if (!particlesCaptured4) {
      // Buffer still holds the last frame's text from phase 1 — sample it now
      captureParticles4();
      particlesCaptured4 = true;
    }

    int explodeFrame = localFrame - HOLD4;

    display.clearDisplay();
    for (int i = 0; i < numParticles4; i++) {
      int px = particles4[i].x + (int)(particles4[i].vx * explodeFrame);
      int py = particles4[i].y + (int)(particles4[i].vy * explodeFrame);

      if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
        display.drawPixel(px, py, WHITE);
      }
    }
    displayOLED();

  } else {
    // --- Phase 3: pause, blank screen ---
    display.clearDisplay();
    displayOLED();
  }

  frame4++;
}

// fireworks!!
void pattern5(){
  display.clearDisplay();

  drawFirework(0,  30, 15);  // firework 1: launches at frame 0, from x=30, peaks at y=15
  drawFirework(15, 90, 10);  // firework 2: launches offset by 15 frames, from x=90, peaks higher

  displayOLED();

  frame5++;
}

// penguin waving
void pattern6(){
  display.clearDisplay();

  int cx = SCREEN_WIDTH / 2;   // horizontal center of penguin
  int cy = SCREEN_HEIGHT / 2;  // vertical center-ish

  // --- Head ---
  int headCx = cx;
  int headCy = cy - 18;
  int headR = 10;
  display.drawCircle(headCx, headCy, headR, WHITE);

  // Eyes
  display.fillCircle(headCx - 4, headCy - 2, 1, WHITE);
  display.fillCircle(headCx + 4, headCy - 2, 1, WHITE);

  // Beak (small triangle pointing down)
  display.fillTriangle(
    headCx - 3, headCy + 3,
    headCx + 3, headCy + 3,
    headCx,     headCy + 8,
    WHITE
  );

  // --- Body ---
  int bodyX = cx - 14;
  int bodyY = cy - 8;
  int bodyW = 28;
  int bodyH = 32;
  display.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, WHITE);

  // Belly line (optional detail, a smaller rounded rect inside)
  display.drawRoundRect(bodyX + 6, bodyY + 6, bodyW - 12, bodyH - 10, 8, WHITE);

  // --- Feet ---
  int footY = bodyY + bodyH;
  display.fillTriangle(cx - 10, footY, cx - 2, footY, cx - 6, footY + 5, WHITE);
  display.fillTriangle(cx + 2,  footY, cx + 10, footY, cx + 6, footY + 5, WHITE);

  // --- Static (left) flipper, resting against body ---
  int leftShoulderX = bodyX;
  int leftShoulderY = bodyY + 8;
  display.drawLine(leftShoulderX, leftShoulderY, leftShoulderX - 6, leftShoulderY + 12, WHITE);
  display.drawLine(leftShoulderX - 1, leftShoulderY, leftShoulderX - 7, leftShoulderY + 12, WHITE);

  // --- Waving (right) flipper ---
  int rightShoulderX = bodyX + bodyW;
  int rightShoulderY = bodyY + 8;
  int flipperLength = 16;

  // Oscillate angle between roughly -30 and +30 degrees for a wave motion
  float waveAngle = -30 + 60 * (sin(frame6 * 0.15) * 0.5 + 0.5);
  float rad = waveAngle * DEG_TO_RAD;

  int flipperEndX = rightShoulderX + (int)(flipperLength * cos(rad));
  int flipperEndY = rightShoulderY + (int)(flipperLength * sin(rad));

  // Draw flipper as two close lines for a bit of thickness
  display.drawLine(rightShoulderX, rightShoulderY, flipperEndX, flipperEndY, WHITE);
  display.drawLine(rightShoulderX, rightShoulderY + 1, flipperEndX, flipperEndY + 1, WHITE);

  displayOLED();

  frame6++;
}

// snoopy walking across the screen
void pattern7(){
  display.clearDisplay();

  // --- Walk cycle timing ---
  int walkSpeed = 1;
  int spriteWidth = 34; // rough width incl. snout/tail, used for wrap-around
  int walkX = -spriteWidth + ((frame7 * walkSpeed) % (SCREEN_WIDTH + spriteWidth));

  int groundY = SCREEN_HEIGHT - 12; // baseline for feet

  float legPhase = frame7 * 0.35;
  int bob = (int)(sin(legPhase * 2) * 1.0); // subtle bounce synced to gait

  // --- Body ---
  int bodyW = 20;
  int bodyH = 10;
  int bodyX = walkX + 10;
  int bodyY = groundY - bodyH - 6 + bob;
  display.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 4, WHITE);

  // --- Head (leading edge, facing direction of travel) ---
  int headR = 8;
  int headCx = bodyX + bodyW - 2;
  int headCy = bodyY - headR + 4 + bob;
  display.drawCircle(headCx, headCy, headR, WHITE);

  // Eye
  display.fillCircle(headCx + 2, headCy - 2, 1, WHITE);

  // --- Snout ---
  int snoutCx = headCx + 8;
  int snoutCy = headCy + 3;
  display.fillCircle(snoutCx, snoutCy, 3, WHITE);
  display.fillCircle(snoutCx + 3, snoutCy, 1, WHITE); // nose tip

  // --- Ear (floppy, sways with gait) ---
  float earSwing = sin(legPhase) * 2.0;
  int earTopX = headCx - 4;
  int earTopY = headCy - 2;
  int earBotX = earTopX - 3 + (int)earSwing;
  int earBotY = earTopY + 9;
  display.drawLine(earTopX, earTopY, earBotX, earBotY, WHITE);
  display.drawLine(earTopX - 2, earTopY, earBotX - 2, earBotY, WHITE);

  // --- Legs (scissor walk cycle, front/back opposite phase) ---
  int legBaseY = bodyY + bodyH;
  float frontSwing = sin(legPhase) * 4.0;
  float backSwing = sin(legPhase + PI) * 4.0;

  int frontLegX = bodyX + bodyW - 5;
  int backLegX = bodyX + 4;

  display.drawLine(frontLegX, legBaseY, frontLegX + (int)frontSwing, legBaseY + 6, WHITE);
  display.drawLine(backLegX, legBaseY, backLegX + (int)backSwing, legBaseY + 6, WHITE);

  // --- Tail (wagging, curls up at the back) ---
  float tailAngle = -40 + 30 * (sin(legPhase * 1.5) * 0.5 + 0.5);
  float rad = tailAngle * DEG_TO_RAD;
  int tailBaseX = bodyX;
  int tailBaseY = bodyY + 2;
  int tailLen = 8;
  int tailEndX = tailBaseX - (int)(tailLen * cos(rad));
  int tailEndY = tailBaseY - (int)(tailLen * sin(rad));
  display.drawLine(tailBaseX, tailBaseY, tailEndX, tailEndY, WHITE);

  displayOLED();

  frame7++;
}

//
void pattern8() {

  display.clearDisplay();

  // Stars
  for (int i = 0; i < 18; i++) {

    int x = (i * 37 + 9) % SCREEN_WIDTH;
    int y = (i * 11 + 3) % SCREEN_HEIGHT;

    if ((frame8 / 8 + i) % 4 != 0) {
      display.drawPixel(x, y, WHITE);
    }
  }

  // Crescent moon
  display.fillCircle(108, 13, 9, WHITE);
  display.fillCircle(112, 10, 8, 0);

  // Moving sleigh position
  int x = -100 + ((frame8 / 2) % (SCREEN_WIDTH + 100));
  int y = 30 + (int)(sin(frame8 * 0.08) * 4);

  // Sleigh body
  display.drawLine(x, y, x + 33, y, WHITE);

  display.drawLine(x, y, x + 7, y + 10, WHITE);

  display.drawLine(x + 7, y + 10,
                   x + 30, y + 10, WHITE);

  display.drawLine(x + 30, y + 10,
                   x + 33, y, WHITE);

  // Sleigh runners
  display.drawLine(x + 3, y + 15,
                   x + 37, y + 15, WHITE);

  display.drawLine(x + 37, y + 15,
                   x + 41, y + 11, WHITE);

  display.drawFastVLine(x + 10, y + 10, 5, WHITE);
  display.drawFastVLine(x + 27, y + 10, 5, WHITE);

  // Santa's head
  display.drawCircle(x + 22, y - 12, 5, WHITE);

  // Santa hat
  display.fillTriangle(
    x + 16, y - 15,
    x + 28, y - 15,
    x + 21, y - 23,
    WHITE
  );

  // Santa's body
  display.drawRoundRect(
    x + 14, y - 7, 15, 8, 3, WHITE
  );

  // Sack of presents
  display.drawCircle(x + 7, y - 5, 5, WHITE);

  // Reindeer reins
  display.drawLine(x + 30, y - 4,
                   x + 65, y - 3, WHITE);

  // Reindeer body
  display.drawRoundRect(
    x + 59, y - 3, 20, 9, 3, WHITE
  );

  // Reindeer head
  display.drawLine(x + 76, y - 2,
                   x + 83, y - 12, WHITE);

  display.drawCircle(x + 85, y - 12, 4, WHITE);

  // Antlers
  display.drawLine(x + 84, y - 16,
                   x + 81, y - 23, WHITE);

  display.drawLine(x + 81, y - 20,
                   x + 77, y - 22, WHITE);

  display.drawLine(x + 86, y - 16,
                   x + 89, y - 23, WHITE);

  // Animated reindeer legs
  int legSwing = (int)(sin(frame8 * 0.25) * 5);

  display.drawLine(x + 63, y + 6,
                   x + 63 + legSwing, y + 13, WHITE);

  display.drawLine(x + 75, y + 6,
                   x + 75 - legSwing, y + 13, WHITE);

  // Snowy ground
  for (int i = 0; i < SCREEN_WIDTH; i++) {

    int groundY = 57 + (int)(sin(i * 0.07) * 3);

    display.drawPixel(i, groundY, WHITE);
  }

  displayOLED();

  frame8++;
}

// 
  
void pattern9() {

  display.clearDisplay();

  int cx = SCREEN_WIDTH / 2;
  int cy = 23;

  // Animate the size of the heart
  float pulse = 1.0 + 0.12 * sin(frame9 * 0.15);

  // Parametric heart equation
  for (int i = 0; i < 120; i++) {

    float t = (2.0 * PI * i) / 120;

    float x = 16 * pow(sin(t), 3);

    float y = 13 * cos(t)
              - 5 * cos(2 * t)
              - 2 * cos(3 * t)
              - cos(4 * t);

    int px = cx + (int)(x * pulse);
    int py = cy - (int)(y * pulse);

    // Filled heart
    display.drawLine(cx, cy, px, py, WHITE);
  }

  // Sparkles around the heart
  const int sparkleX[] = {26, 100, 19, 111};
  const int sparkleY[] = {16, 31, 38, 9};

  for (int i = 0; i < 4; i++) {

    if ((frame9 / 6 + i) % 3 != 0) {

      int x = sparkleX[i];
      int y = sparkleY[i];

      display.drawFastHLine(x - 2, y, 5, WHITE);
      display.drawFastVLine(x, y - 2, 5, WHITE);
    }
  }

  // Christmas message
  const char* message = "Made with love";

  display.setTextSize(1);
  display.setTextColor(WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(message, 0, 0,
                        &x1, &y1, &w, &h);

  display.setCursor((SCREEN_WIDTH - w) / 2, 54);
  display.print(message);

  displayOLED();

  frame9++;
}

void powerLED(int on){
  if (on == 1){
    digitalWrite(POWER_LED, HIGH);
  } else {
    digitalWrite(POWER_LED, LOW);
  }
}

void resetPatternState(){
  frame0 = 0;
  frame1 = 0;
  frame2 = 0;
  frame3 = 0;
  frame4 = 0;
  frame5 = 0;
  frame6 = 0;
  frame7 = 0;
  frame8 = 0;
  frame9 = 0;

  particlesCaptured4 = false;
  numParticles4 = 0;
  // reset any other pattern's counters here too
}

void buttonSetup(){
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}



