#pragma once

#include <TFT_eSPI.h>

#include "Types.h"
#include "games/ReactionGame.h"
#include "games/RunnerGame.h"
#include "games/SlidingGame.h"
#include "games/LineFourGame.h"
#include "games/PairQuestGame.h"
#include "games/TileSlideGame.h"

class App {
 public:
  explicit App(TFT_eSPI& display);
  void begin();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Screen : uint8_t {
    Home,
    Reaction,
    Sliding,
    Runner,
    LineFour,
    PairQuest,
    TileSlide,
    About,
  };

  TFT_eSPI& tft_;
  Screen screen_ = Screen::Home;
  ReactionGame reaction_;
  SlidingGame sliding_;
  RunnerGame runner_;
  LineFourGame lineFour_;
  PairQuestGame pairs_;
  TileSlideGame tiles_;
  uint8_t glowCard_ = 0;
  uint32_t lastHomeFrame_ = 0;

  static constexpr Rect HOME_BUTTON{6, 6, 52, 28};
  static constexpr Rect ABOUT_LINK{210, 220, 106, 20};

  void showSplash();
  void drawHome();
  void drawAbout();
  void drawAboutQr(int16_t x, int16_t y, uint8_t moduleSize);
  void drawGameCard(uint8_t index);
  void drawMiniIcon(uint8_t index, int16_t x, int16_t y, uint16_t color);
  void drawCardGlow(uint8_t index, bool active);
  void updateHomeAnimation(uint32_t now);
  void open(Screen screen);
};
