#pragma once

#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 10
#define NUMPIXELS 1

struct NeoColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

constexpr NeoColor redColor = {10, 0, 0};
constexpr NeoColor greenColor = {0, 10, 0};
constexpr NeoColor blueColor = {0, 0, 10};
constexpr NeoColor whiteColor = {10, 10, 10};

constexpr NeoColor colors[] = {redColor, greenColor, blueColor, whiteColor};
enum ColorIndex { NEO_RED = 0, NEO_GREEN = 1, NEO_BLUE = 2, NEO_WHITE = 3 };

void neopixel_init();
void neopixel_set_color(NeoColor color);
