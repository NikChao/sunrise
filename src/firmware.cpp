#include "esp32-hal-gpio.h"
#include "time.h"
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <RTCLib.h>
#include <TM1637.h>
#include <Wire.h>
#include <time.h>
#include <unistd.h>

const int btnPin = 12;
const int powerPin = 13;

// TM1637 ()
int CLK = 19;
int DIO = 18;
TM1637 tm(CLK, DIO);

volatile bool on = false;

#define LED_PIN 16
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

#define DS1307_ADDRESS 0x68 // I2C address for DS1307
#define SDA_PIN 21
#define SCL_PIN 22
RTC_DS1307 rtc;

void IRAM_ATTR toggleLedISR() {
  if (on) {
    digitalWrite(powerPin, HIGH);
  } else {
    digitalWrite(powerPin, LOW);
  }

  on = !on;
}

void setup() {
  pinMode(powerPin, OUTPUT);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);

  // 7seg
  tm.init();
  tm.set(BRIGHT_TYPICAL);
  tm.point(false);

  // Clock
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  attachInterrupt(digitalPinToInterrupt(btnPin), toggleLedISR, FALLING);
}

int bcdToDec(byte val) { return (val / 16 * 10) + (val % 16); }

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

  return hours * 100 + minutes; // returns HHMM
}

void writeTime(int value) {
  int d3 = (value / 1000) % 10;
  int d2 = (value / 100) % 10;
  int d1 = (value / 10) % 10;
  int d0 = value % 10;
  tm.display(0, d3);
  tm.display(1, d2);
  tm.display(2, d1);
  tm.display(3, d0);
  tm.set(BRIGHT_TYPICAL, 0);
  tm.point(true);
}

int lastEdit = 0;
bool state = false;
int onDuration = 0;

void loop() {

  int displayTime = getTime();
  writeTime(displayTime);

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.setBrightness(255);
  FastLED.show();

  delay(1000);
}
