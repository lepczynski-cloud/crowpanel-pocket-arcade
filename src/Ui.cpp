#include "Ui.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "Theme.h"

namespace Ui {

void fillBackground(TFT_eSPI& tft) {
  tft.fillScreen(Theme::BG);
  tft.fillRect(0, 0, Config::SCREEN_WIDTH, 3, Theme::CYAN_DARK);
}

void drawHomeButton(TFT_eSPI& tft) {
  const Rect rect{6, 6, 52, 28};
  tft.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 7, Theme::PANEL_2);
  tft.drawRoundRect(rect.x, rect.y, rect.w, rect.h, 7, Theme::BORDER);
  tft.fillTriangle(15, 20, 23, 13, 23, 27, Theme::CYAN);
  tft.setTextColor(Theme::TEXT, Theme::PANEL_2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.drawString("HOME", 39, 20);
}

void drawTopBar(TFT_eSPI& tft, const char* title, bool homeButton) {
  tft.fillRect(0, 0, Config::SCREEN_WIDTH, 40, Theme::BG_2);
  tft.fillRect(0, 38, Config::SCREEN_WIDTH, 2, Theme::CYAN_DARK);
  if (homeButton) {
    drawHomeButton(tft);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(Theme::TEXT, Theme::BG_2);
  tft.drawString(title, homeButton ? 188 : 160, 20);
}

void drawButton(TFT_eSPI& tft, const Rect& rect, const char* label, uint16_t fill,
                uint16_t text, uint16_t border) {
  tft.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 8, fill);
  tft.drawRoundRect(rect.x, rect.y, rect.w, rect.h, 8, border);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(text, fill);
  tft.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
}

void drawButton(TFT_eSPI& tft, const Rect& rect, const char* label) {
  drawButton(tft, rect, label, Theme::PANEL_2, Theme::TEXT, Theme::BORDER);
}

void drawTargetIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color) {
  tft.drawCircle(cx, cy, 25, color);
  tft.drawCircle(cx, cy, 17, color);
  tft.fillCircle(cx, cy, 7, color);
  tft.drawFastHLine(cx - 33, cy, 14, Theme::MUTED);
  tft.drawFastHLine(cx + 20, cy, 14, Theme::MUTED);
  tft.drawFastVLine(cx, cy - 33, 14, Theme::MUTED);
  tft.drawFastVLine(cx, cy + 20, 14, Theme::MUTED);
}

void drawBlocksIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color) {
  tft.fillRoundRect(cx - 30, cy - 25, 24, 50, 5, Theme::PURPLE);
  tft.fillRoundRect(cx - 1, cy - 25, 52, 24, 5, color);
  tft.fillRoundRect(cx - 1, cy + 5, 35, 20, 5, Theme::PINK);
  tft.drawFastHLine(cx + 38, cy + 15, 18, Theme::MUTED);
  tft.fillTriangle(cx + 57, cy + 15, cx + 49, cy + 10, cx + 49, cy + 20, Theme::MUTED);
}

void drawWormIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color) {
  const int16_t points[6][2] = {{-28, 12}, {-14, 12}, {-2, 2}, {10, -8}, {24, -8}, {32, 2}};
  for (uint8_t i = 0; i < 6; ++i) {
    const uint16_t partColor = i == 5 ? Theme::LIME : color;
    tft.fillCircle(cx + points[i][0], cy + points[i][1], i == 5 ? 9 : 7, partColor);
  }
  tft.fillCircle(cx + 35, cy, 2, Theme::BG);
}

void drawSparkles(TFT_eSPI& tft, uint8_t count, int16_t yMin, int16_t yMax) {
  for (uint8_t i = 0; i < count; ++i) {
    const int16_t x = random(5, Config::SCREEN_WIDTH - 5);
    const int16_t y = random(yMin, yMax);
    const uint16_t color = (i % 3 == 0) ? Theme::CYAN_DARK : Theme::GRID;
    tft.drawPixel(x, y, color);
    if (i % 4 == 0) {
      tft.drawPixel(x + 1, y, color);
    }
  }
}

}  // namespace Ui
