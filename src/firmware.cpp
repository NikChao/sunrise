#include "esp32-hal-gpio.h"
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define LED_PIN 22
#define NUM_LEDS 4
CRGB leds[NUM_LEDS];

const int powerPin = 13;

void setup() {
  pinMode(powerPin, OUTPUT);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

bool on = false;
void loop() {
  digitalWrite(powerPin, HIGH);
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}
