#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_system.h>

#include "App.h"
#include "AppConfig.h"
#include "TouchInput.h"

namespace {

TFT_eSPI tft;
TouchInput touch(tft);
App app(tft);

void runTouchCalibration() {
  if (!Config::RUN_TOUCH_CALIBRATION) {
    return;
  }

  uint16_t calibrated[5] = {0, 0, 0, 0, 0};
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(16, 12);
  tft.println("Touch the corners as shown");
  tft.calibrateTouch(calibrated, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println("Copy these values into Config::touchCalibration:");
  Serial.printf("{%u, %u, %u, %u, %u}\n", calibrated[0], calibrated[1], calibrated[2],
                calibrated[3], calibrated[4]);
  for (uint8_t i = 0; i < 5; ++i) {
    Config::touchCalibration[i] = calibrated[i];
  }
  delay(1200);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(Config::BACKLIGHT_PIN, OUTPUT);
  digitalWrite(Config::BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(false);

  runTouchCalibration();
  tft.setTouch(Config::touchCalibration);

  randomSeed(esp_random());
  app.begin();
}

void loop() {
  const InputFrame input = touch.update();
  app.update(input, millis());
  delay(8);
}
