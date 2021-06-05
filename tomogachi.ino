#define TONE_USE_INT
#define TONE_PITCH 440
#include <Pitch.h>

#include <SoftwareSerial.h>
#include <ButtonDebounce.h>

/*
ST7735 TFT SPI display pins for Arduino Uno/Nano:
 * LED =   3.3V
 * SCK =   13
 * SDA =   11
 * A0 =    8
 * RESET = 9
 * CS =    10
 * GND =   GND
 * VCC =   5V
*/


#include "Ucglib.h"  // Include Ucglib library to drive the display

#define REFRESH_PERIOD 3000
#define DEBOUNCE_TIME 100
#define RIGHT_BUTTON_PIN PD7
#define SELECT_BUTTON_PIN PD4
#define LEFT_BUTTON_PIN PD2
#define LIGHT_PIN A0
#define BUZZER_PIN PD3
#define BUF_LEN 20

#define NR_METERS 4
#define NR_MENU_ITEMS 4
#define WATER 0
#define FOOD 1
#define SLEEP 2
#define JOY 3

#define SPEED 0.02
#define WATER_SPEED 1.5
#define HUNGER_SPEED 1.0
#define SLEEP_SPEED 1.2
#define JOY_SPEED 0.7
#define SLEEP_REPLENISH_SPEED 1.5

#define METER_GAP 25
#define METER_THICKNESS 20 // strictly less than METER_GAP
#define METER_LENGTH 100
#define METER_START_X 20
#define METER_START_Y 5 

#define WATER_REPLENISH 0.2
#define HUNGER_REPLENISH 0.5
#define SLEEP_REPLENISH 0
#define JOY_REPLENISH 0.1
#define WARNING_BEEP_THRESHOLD 0.2
#define SLEEP_LIGHT_THRESHOLD 500

ButtonDebounce leftButton(LEFT_BUTTON_PIN, DEBOUNCE_TIME);
ButtonDebounce selectButton(SELECT_BUTTON_PIN, DEBOUNCE_TIME);
ButtonDebounce rightButton(RIGHT_BUTTON_PIN, DEBOUNCE_TIME);

char numberBuffer[BUF_LEN];
int menuIndex = 0;
char menuMessages[][BUF_LEN] = {"DRINK", "EAT", "SLEEP", "PLAY"};
double meters[] = {1, 1, 1, 1};
double meterSpeeds[] = {WATER_SPEED, HUNGER_SPEED, SLEEP_SPEED, JOY_SPEED};
double meterReplenish[] = {WATER_REPLENISH, HUNGER_REPLENISH, SLEEP_REPLENISH, JOY_REPLENISH};
int meterWarningNote[] = {NOTE_E7, NOTE_E7, NOTE_E7, NOTE_E7};
int lightValue = 0;

bool sleeping = false;
bool newToSleeping = true;

// Create display and define the pins:
Ucglib_ST7735_18x128x160_HWSPI ucg(8, 10, 9);  // (A0=8, CS=10, RESET=9)
// The rest of the pins are pre-selected as the default hardware SPI for Arduino Uno (SCK=13 and SDA=11)

char *causeOfDeath = NULL;

char deathMessages[][BUF_LEN] = {"THIRST", "HUNGER","EXHAUSTION", "SADNESS"};
bool checkForDeath() {
  int it;
  bool noWarning = true;
  for (it = 0; it < NR_METERS; it++) {
    if (meters[it] <= WARNING_BEEP_THRESHOLD) {
      tone(BUZZER_PIN, meters[it]);
      noWarning = false;
    }
    if (meters[it] <= 0) {
      causeOfDeath = deathMessages[it];
      return true;
    }
  }

  if (noWarning) {
    noTone(BUZZER_PIN);  
  }
  
  return false;
}
void wakeUp() {
  sleeping = false;
  newToSleeping = true;
}

void updateMeters(unsigned long deltaTime) {
  double deltaVirtualTimeSeconds = SPEED * deltaTime / 1000;
  int it;

  for (it = 0; it < NR_METERS; it++) {
    meters[it] -= meterSpeeds[it] * deltaVirtualTimeSeconds;
  }
}

void displayMeters() {
  ucg.setColor(0, 255, 255);  // Set color (0,R,G,B)
  ucg.drawBox(METER_START_X,
              METER_START_Y,
              METER_LENGTH * meters[WATER],
              METER_THICKNESS);  // Start from top-left pixel (x,y,wigth,height)

  ucg.setColor(255, 165, 0);
  ucg.drawBox(METER_START_X,
              METER_START_Y + METER_GAP,
              METER_LENGTH * meters[FOOD],
              METER_THICKNESS);

  ucg.setColor(180 , 180, 180);
  ucg.drawBox(METER_START_X,
              METER_START_Y + METER_GAP * 2,
              METER_LENGTH * meters[SLEEP],
              METER_THICKNESS);


  ucg.setColor(255, 255, 0);
  ucg.drawBox(METER_START_X,
              METER_START_Y + METER_GAP * 3,
              METER_LENGTH * meters[JOY],
              METER_THICKNESS);

}

void onLeftInMenu(const int state) {
  if (state % 2 == 0) {
    wakeUp();
    menuIndex = menuIndex == 0 ? (NR_MENU_ITEMS - 1) : (menuIndex - 1);
  }
}

void onRightInMenu(const int state) {
  if (state % 2 == 0) {
    wakeUp();
    menuIndex = (menuIndex + 1) % NR_MENU_ITEMS;
  }
}

void onSelectInMenu(const int state) {
  if (state % 2 == 0) {
    wakeUp();
    if (menuIndex < NR_METERS) {
      meters[menuIndex] += meterReplenish[menuIndex];
      if (meters[menuIndex] > 1) {
        meters[menuIndex] = 1;
      }
    }
    if (menuIndex == SLEEP && analogRead(LIGHT_PIN) < SLEEP_LIGHT_THRESHOLD) {
      sleeping = true;
    }
  }
}

void setup(void)  // Start of setup
{
  leftButton.setCallback(onLeftInMenu);
  selectButton.setCallback(onSelectInMenu);
  rightButton.setCallback(onRightInMenu);

  // Display setup:
  
  // Select a type of text background:
  //ucg.begin(UCG_FONT_MODE_TRANSPARENT);  // It doesn't overwrite background, so it's a mess for text that changes
  ucg.begin(UCG_FONT_MODE_SOLID);  // It writes a background for the text. This is the recommended option

  ucg.clearScreen();  // Clear the screen

  // Set display orientation:
  ucg.setRotate180();  // Put 90, 180 or 270, or comment to leave default

  ucg.setFont(ucg_font_inr21_mr);
 
} 

unsigned long currentMillis = 0;
unsigned long prevMillis = 0;

unsigned long clearCounter = 0;
void loop(void)
{
  leftButton.update();
  rightButton.update();
  selectButton.update();
  currentMillis = millis();

  if (analogRead(LIGHT_PIN) >= SLEEP_LIGHT_THRESHOLD) {
    wakeUp();
  }

  if(sleeping) {
    int deltaVirtualTimeInSeconds = (currentMillis - prevMillis) / 1000 * SPEED;
    if(newToSleeping) {
      newToSleeping = false;
      ucg.clearScreen();
      ucg.setPrintPos(35,70);
      ucg.print("ZZZ");
    }

    meters[SLEEP] += SLEEP_REPLENISH_SPEED * deltaVirtualTimeInSeconds;
    prevMillis = currentMillis;
    if (meters[SLEEP] > 1) {
      meters[SLEEP] = 1;
      wakeUp();
    }

    return;
  }

  if (checkForDeath()) {
    ucg.clearScreen();
    ucg.setPrintPos(5,35);
    ucg.print("YOUR PET");
    ucg.setPrintPos(5,55);
    ucg.print("DIED OF");
    ucg.setPrintPos(5,75);
    ucg.print(causeOfDeath);
    noTone(BUZZER_PIN);
    while (true);
  }
  clearCounter++;
  if (clearCounter == REFRESH_PERIOD) {
    ucg.clearScreen();

    ucg.setFont(ucg_font_inb16_mr);
    ucg.setColor(0, 255, 255, 255);
    ucg.setPrintPos(40,135);
    ucg.print(menuMessages[menuIndex]);
    
    displayMeters();
    clearCounter = 0;
  }
  
  updateMeters(currentMillis - prevMillis);

  prevMillis = currentMillis;
}
