#include <SPI.h>
#include <Gamebuino.h>
#include <EEPROM.h>

#include "constants.h"
#include "enemy.h"
#include "player.h"
#include "target.h"

#define MAX_ENEMIES 50

#define MODE_PLAYING 0
#define MODE_GAME_OVER 1
#define MODE_TITLE_SCREEN 2
#define MODE_MAIN_MENU 3
#define MODE_INSTRUCTIONS 4

#define TITLE_X ((LCDWIDTH - (5 * 6)) >> 1)

Gamebuino gb;
// Represents the player
Player player;
// Represents the obstacles to avoid
Enemy enemies[MAX_ENEMIES];
byte enemyCount;
// Represents what the player is trying to capture
Target target;
byte score;
byte highScore;
byte mode;
unsigned long referenceTime;

extern const byte font3x5[];
extern const byte font5x7[];

const byte logo[] PROGMEM = {32,11,
0x0,0x0,0x73,0x80,
0x0,0x0,0x52,0x80,
0x67,0xFF,0xDE,0xC0,
0xF4,0x54,0x54,0x40,
0xF5,0x55,0x4E,0xC0,
0x65,0x55,0x56,0x80,
0x5,0x55,0x56,0x80,
0x4,0x45,0x56,0x80,
0x5,0xFF,0xFF,0x80,
0x5,0x0,0x0,0x0,
0x7,0x0,0x0,0x0,
};

const byte logoLarge[] PROGMEM = {64,29,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0xFC,0x3F,0x0,
0x0,0x0,0x0,0x0,0x0,0xFC,0x3F,0x0,
0x0,0x0,0x0,0x0,0x0,0xCC,0x33,0x0,
0x0,0x0,0x0,0x0,0x0,0xCC,0x33,0x0,
0x0,0xF0,0xFF,0xFF,0xFF,0xCF,0xF3,0xC0,
0x0,0xF0,0xFF,0xFF,0xFF,0xCF,0xF3,0xC0,
0x3,0xFC,0xC0,0xCC,0xC0,0xCC,0xC0,0xC0,
0x3,0xFC,0xC0,0xCC,0xC0,0xCC,0xC0,0xC0,
0x3,0xFC,0xCC,0xCC,0xCC,0xC3,0xF3,0xC0,
0x3,0xFC,0xCC,0xCC,0xCC,0xC3,0xF3,0xC0,
0x0,0xF0,0xCC,0xCC,0xCC,0xCC,0xF3,0x0,
0x0,0xF0,0xCC,0xCC,0xCC,0xCC,0xF3,0x0,
0x0,0x0,0xCC,0xCC,0xCC,0xCC,0xF3,0x0,
0x0,0x0,0xCC,0xCC,0xCC,0xCC,0xF3,0x0,
0x0,0x0,0xC0,0xC0,0xCC,0xCC,0xF3,0x0,
0x0,0x0,0xC0,0xC0,0xCC,0xCC,0xF3,0x0,
0x0,0x0,0xCF,0xFF,0xFF,0xFF,0xFF,0x0,
0x0,0x0,0xCF,0xFF,0xFF,0xFF,0xFF,0x0,
0x0,0x0,0xCC,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0xCC,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0xFC,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0xFC,0x0,0x0,0x0,0x0,0x0,
};

void setup() {
  gb.begin();
  reset();
  // Load high score
  if (EEPROM.read(0) == 42) {
    highScore = EEPROM.read(1);
  }
  else {
    EEPROM.write(0, 42);
    EEPROM.update(1, 0);
  }
}

void loop() {
  if (gb.update()) {
    if (gb.buttons.pressed(BTN_C)) reset();

    switch (mode) {
      case MODE_PLAYING:
        doPlaying();
        break;
      case MODE_GAME_OVER:
        doGameOver();
        break;
      case MODE_TITLE_SCREEN:
        referenceTime = gb.frameCount;
        mode = MODE_MAIN_MENU;
        break;
      case MODE_MAIN_MENU:
        doMainMenu();
        break;
      case MODE_INSTRUCTIONS:
        doInstructions();
        break;
    }    
  }
}

void reset() {
  gb.titleScreen(F(""), logoLarge);
  gb.setFrameRate(30);
  gb.battery.show = false;
  mode = MODE_TITLE_SCREEN;
}

void initializeGame() {
  gb.pickRandomSeed();
  enemyCount = 0;
  score = 0;
  player = Player();
  target = Target();
  mode = MODE_PLAYING;
  gb.sound.playOK();
}

void doPlaying() {
  player.update(gb);

  // Check if we've hit an enemy
  for (byte i = 0; i < enemyCount; i++) 
  {
    enemies[i].update(gb.frameCount);
    if (enemies[i].isHit(player.getX(), player.getY(), 4)) {
      gb.sound.playCancel();
      mode = MODE_GAME_OVER;
    }
  }

  // Check if we've captured a target
  if (target.isHit(player.getX(), player.getY(), 4)) {
    gb.sound.playOK();
    do {
      target.moveToNewLocation();
    } while (target.isHit(player.getX(), player.getY(), 4));
    if (enemyCount < MAX_ENEMIES) {
      enemies[enemyCount++].spawn(gb.frameCount, player.getX(), player.getY());
    }
    // Update score
    if (score < 100) score++;
    if (score > highScore) highScore = score;
  }

  drawBoard();
}

void doGameOver() {
  drawBoard();

  gb.display.setColor(BLACK);
  if (gb.frameCount & 0x08) drawScore();
  gb.display.fillRect((BOARD_WIDTH - ((4 * 9) + 1)) >> 1, (BOARD_HEIGHT - 7) >> 1, (4 * 9) + 1, 7);
  gb.display.setFont(font3x5);
  gb.display.setColor(WHITE);
  gb.display.cursorX = ((BOARD_WIDTH - (4 * 9) + 1) >> 1);
  gb.display.cursorY = ((BOARD_HEIGHT - 7) >> 1) + 1;
  gb.display.print("Game Over");

  if (gb.buttons.pressed(BTN_A) || gb.buttons.pressed(BTN_B)) {
    referenceTime = gb.frameCount;
    mode = MODE_MAIN_MENU;
  }

  // Save the high score
  EEPROM.update(1, highScore);
}

void doMainMenu() {
  if (gb.buttons.pressed(BTN_A)) {
    initializeGame();
    return;
  }
  else if (gb.buttons.pressed(BTN_B)) {
    referenceTime = gb.frameCount;
    mode = MODE_INSTRUCTIONS;
    // So instructions always look the same.
    randomSeed(1);
    target.moveToNewLocation();
    enemies[0].spawn(gb.frameCount, 0, 0);
    enemies[1].spawn(gb.frameCount, 0, 0);
    player = Player();
    return;
  }
  
  unsigned long t = gb.frameCount - referenceTime;
  
  gb.display.setColor(BLACK);
  gb.display.drawBitmap(TITLE_X, t < 22 << 1 ? -11 + (t >> 1) : 11, logo);
  gb.display.fillRect(0, 0, GUTTER_WIDTH, BOARD_HEIGHT);
  gb.display.fillRect(LCDWIDTH - GUTTER_WIDTH, 0, GUTTER_WIDTH, BOARD_HEIGHT);

  if (t > 22 << 1) {
    gb.display.setFont(font3x5);
    gb.display.cursorY = 25;
    gb.display.cursorX = TITLE_X - 5;
    gb.display.print(F("start"));
    gb.display.cursorY = 33;
    gb.display.cursorX = TITLE_X - 5;
    gb.display.print(F("instructions"));

    if (t & 0x08)
    {
      gb.display.setFont(font5x7);
      gb.display.cursorY = 24;
      gb.display.cursorX = TITLE_X - 11;
      gb.display.print('\25');
      gb.display.cursorY = 32;
      gb.display.cursorX = TITLE_X - 11;
      gb.display.print('\26');
    }
  }
}

void doInstructions() {
  if (gb.buttons.pressed(BTN_A) || gb.buttons.pressed(BTN_B)) {
    referenceTime = gb.frameCount;
    mode = MODE_MAIN_MENU;
    gb.pickRandomSeed();
    return;
  }
  
  unsigned long t = gb.frameCount - referenceTime;

  gb.display.setColor(BLACK);
  gb.display.setFont(font3x5);
  
  if (t < 100) {
    gb.display.cursorX = 31;
    gb.display.cursorY = 14;
    gb.display.print(F("you"));
    player.draw(gb);
  }
  else if (t < 250) {
    gb.display.cursorX = 10;
    gb.display.cursorY = 14;
    gb.display.print(F("collect these"));
    target.draw(gb); 
  }
  else if (t < 400) {
    gb.display.cursorX = 14;
    gb.display.cursorY = 14;
    gb.display.print(F("avoid these"));
    enemies[0].update(gb.frameCount);
    enemies[1].update(gb.frameCount);
    enemies[0].draw(gb);
    enemies[1].draw(gb);
  }
  else { 
    referenceTime = gb.frameCount;
    mode = MODE_MAIN_MENU;
    gb.pickRandomSeed();
  }
}

void drawBoard() {
  target.draw(gb);
  
  player.draw(gb);

  for (byte i = 0; i < enemyCount; i++) 
  {
    enemies[i].draw(gb);
  }
  
  drawGutter();
}

void drawGutter() {
  gb.display.setColor(BLACK);
  gb.display.fillRect(LCDWIDTH - GUTTER_WIDTH, 0, GUTTER_WIDTH, BOARD_HEIGHT);

  gb.display.setColor(WHITE);
  drawScore();
  drawHighScore();
}

void drawScore() {
  gb.display.setFont(font3x5);
  gb.display.cursorX = BOARD_WIDTH + 1;
  gb.display.cursorY = 1;
  if (score < 10) gb.display.print('0');
  gb.display.print(score);
}

void drawHighScore() {
  gb.display.setFont(font3x5);
  gb.display.cursorX = BOARD_WIDTH + 1;
  gb.display.cursorY = BOARD_HEIGHT - 12;
  gb.display.print("hi");
  gb.display.cursorX = BOARD_WIDTH + 1;
  gb.display.cursorY = BOARD_HEIGHT - 6;
  if (highScore < 10) gb.display.print('0');
  gb.display.print(highScore);
}

