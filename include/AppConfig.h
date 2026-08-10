#pragma once

#include <Arduino.h>

namespace Config {

constexpr int16_t SCREEN_WIDTH = 320;
constexpr int16_t SCREEN_HEIGHT = 240;
constexpr uint8_t BACKLIGHT_PIN = 27;
constexpr bool RUN_TOUCH_CALIBRATION = false;

// Default calibration for the Elecrow CrowPanel 2.8-inch ESP32 display used by
// the original Pocket Admin Toolkit project. Recalibrate if your unit differs.
extern uint16_t touchCalibration[5];

}  // namespace Config
