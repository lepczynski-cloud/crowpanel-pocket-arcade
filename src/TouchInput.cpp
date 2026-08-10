#include "TouchInput.h"

#include <cstdlib>

TouchInput::TouchInput(TFT_eSPI& display) : tft_(display) {}

InputFrame TouchInput::update() {
  InputFrame frame;
  uint16_t x = lastX_;
  uint16_t y = lastY_;
  const bool down = tft_.getTouch(&x, &y, 600);

  frame.down = down;
  frame.x = x;
  frame.y = y;
  frame.startX = startX_;
  frame.startY = startY_;

  if (down && !wasDown_) {
    startX_ = x;
    startY_ = y;
    lastX_ = x;
    lastY_ = y;
    swipeIssued_ = false;
    frame.pressed = true;
    frame.startX = startX_;
    frame.startY = startY_;
  } else if (down && wasDown_) {
    lastX_ = x;
    lastY_ = y;
    const int16_t dx = static_cast<int16_t>(x) - static_cast<int16_t>(startX_);
    const int16_t dy = static_cast<int16_t>(y) - static_cast<int16_t>(startY_);
    if (!swipeIssued_ && (std::abs(dx) >= 20 || std::abs(dy) >= 20)) {
      swipeIssued_ = true;
      frame.swipe = true;
      frame.swipeDx = dx;
      frame.swipeDy = dy;
    }
  } else if (!down && wasDown_) {
    frame.released = true;
    frame.x = lastX_;
    frame.y = lastY_;
    frame.startX = startX_;
    frame.startY = startY_;
  }

  wasDown_ = down;
  return frame;
}
