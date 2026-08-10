#pragma once

#include <Arduino.h>

struct Rect {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
};

inline bool contains(const Rect& rect, uint16_t x, uint16_t y) {
  return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

struct InputFrame {
  bool down = false;
  bool pressed = false;
  bool released = false;
  bool swipe = false;
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t startX = 0;
  uint16_t startY = 0;
  int16_t swipeDx = 0;
  int16_t swipeDy = 0;
};
