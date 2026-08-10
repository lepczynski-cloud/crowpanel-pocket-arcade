#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class TouchInput {
 public:
  explicit TouchInput(TFT_eSPI& display);
  InputFrame update();

 private:
  TFT_eSPI& tft_;
  bool wasDown_ = false;
  bool swipeIssued_ = false;
  uint16_t startX_ = 0;
  uint16_t startY_ = 0;
  uint16_t lastX_ = 0;
  uint16_t lastY_ = 0;
};
