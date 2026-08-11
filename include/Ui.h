#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

namespace Ui {

void fillBackground(TFT_eSPI& tft);
void drawTopBar(TFT_eSPI& tft, const char* title, bool homeButton = true);
void drawButton(TFT_eSPI& tft, const Rect& rect, const char* label, uint16_t fill,
                uint16_t text, uint16_t border);
void drawButton(TFT_eSPI& tft, const Rect& rect, const char* label);
void drawHomeButton(TFT_eSPI& tft);
void drawTargetIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color);
void drawBlocksIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color);
void drawRunnerIcon(TFT_eSPI& tft, int16_t cx, int16_t cy, uint16_t color);
void drawSparkles(TFT_eSPI& tft, uint8_t count, int16_t yMin, int16_t yMax);

}  // namespace Ui
