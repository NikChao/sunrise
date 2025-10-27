#include "esp32-hal-gpio.h"
#include <Adafruit_NeoPixel.h>
#include <TM1637.h>
#include <Wire.h>

// Misc
const unsigned long DEBOUNCE_MS = 110;

// Buttons
const int powerButtonPin = 13;
const int settingButtonPin = 12;
const int hourUpPin = 14;
const int minUpPin = 27;

// WS2812B LED strip
const int PIN_WS2812B = 23; // LED
#define NUM_PIXELS 12       // The number of LEDs (pixels) on WS2812B LED strip
int area_num = 3, area1 = 4, area2 = 8, area3 = 12;
Adafruit_NeoPixel strip(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

// TM1637 ()
int CLK = 19;
int DIO = 18;
TM1637 tm(CLK, DIO);

// State
int lastBlink = 0;
int isSetting = LOW;
int ledState = LOW;

// Alarm
volatile int alarmH = 12;
volatile int alarmM = 0;
const int alarmIncrement = 5;

volatile unsigned long lastToggle = 0;
volatile unsigned long lastSetAlarmToggle = 0;
volatile unsigned long lastHourUp = 0;
volatile unsigned long lastMinUp = 0;

#define DS1307_ADDRESS 0x68

// Convert BCD to decimal
byte bcdToDec(byte val) { return (val / 16 * 10) + (val % 16); }
// Read hours and minutes from DS1307
int getTime() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0); // start at register 0
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 3);
  byte sec = Wire.read();
  byte min = Wire.read();
  byte hr = Wire.read();

  int minutes = bcdToDec(min);
  int hours = bcdToDec(hr);

  return hours * 100 + minutes; // HHMM format
}

/** ========= INTERRUPTS ========== */
void IRAM_ATTR togglePowerISR() {
  unsigned long now = millis();
  if ((now - lastToggle) > DEBOUNCE_MS) {
    lastToggle = now;
    ledState = !ledState;
  }
}

void IRAM_ATTR toggleSetAlarmISR() {
  unsigned long now = millis();
  if ((now - lastSetAlarmToggle) > DEBOUNCE_MS) {
    lastSetAlarmToggle = now;
    isSetting = !isSetting;
  }
}

void IRAM_ATTR hourUpISR() {
  unsigned long now = millis();
  if ((now - lastHourUp) > DEBOUNCE_MS) {
    lastHourUp = now;
    if (alarmH >= 23) {
      alarmH = 0;
    } else {
      alarmH++;
    }
  }
}

void IRAM_ATTR minUpISR() {
  unsigned long now = millis();
  if ((now - lastMinUp) > DEBOUNCE_MS) {
    lastMinUp = now;
    if (alarmM >= 55) {
      alarmM = 0;
    } else {
      alarmM += alarmIncrement;
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(settingButtonPin, INPUT_PULLUP);
  pinMode(powerButtonPin, INPUT_PULLUP);
  pinMode(hourUpPin, INPUT_PULLUP);
  pinMode(minUpPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(powerButtonPin), togglePowerISR,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(settingButtonPin), toggleSetAlarmISR,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(hourUpPin), hourUpISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(minUpPin), minUpISR, FALLING);

  Wire.begin();
  strip.begin();
  strip.show();

  tm.init();
  tm.set(BRIGHT_TYPICAL);
  tm.point(false);
}

void writeTime(int displayTime) {
  tm.displayNum(displayTime);
  tm.set(BRIGHT_TYPICAL, 0);
  tm.point(true);
}

void clearTime() {
  tm.set(BRIGHT_DARKEST, 0);
  tm.display(0, 0x7f);
  tm.display(1, 0x7f);
  tm.display(2, 0x7f);
  tm.display(3, 0x7f);
  tm.point(false);
}

void clearStrip() {
  strip.clear();
  strip.show();
}

int MAX_BRIGHTNESS = 255;
int brightness = 10;
void showSunrise() {
  strip.setBrightness(brightness);
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint8_t r = 255;
    uint8_t g = 255 - (i * 10); // green increases along the strip
    uint8_t b = 0;
    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  if (brightness < MAX_BRIGHTNESS) {
    brightness++;
  }

  strip.show();
}

const int delayMs = 2;
const int blinkStart = 140;
const int blinkEnd = 200;

void loop() {
  bool isClearing = lastBlink >= blinkStart && lastBlink <= blinkEnd;

  int displayTime = getTime();
  int alarmTime = alarmH * 100 + alarmM;

  if (displayTime == alarmTime) {
    ledState = HIGH;
  }

  if (ledState) {
    if (!isSetting) {
      writeTime(displayTime);
    } else {
      if (!isClearing) {
        writeTime(alarmTime);
      }
    }

    showSunrise();
  } else {
    clearStrip();
    clearTime();
  }

  if (isSetting) {
    if (isClearing) {
      clearTime();
    }

    lastBlink += delayMs;
  }

  if (lastBlink >= blinkEnd || !isSetting) {
    lastBlink = 0;
  }

  delay(delayMs);
}
