#pragma once

#include <TFT_eSPI.h>

#include "Types.h"
#include "games/WormGame.h"
#include "games/ReactionGame.h"
#include "games/SlidingGame.h"

class App {
 public:
  explicit App(TFT_eSPI& display);

  void begin();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Screen {
    Home,
    Reaction,
    Sliding,
    Worm,
  };

  TFT_eSPI& tft_;
  Screen screen_ = Screen::Home;
  ReactionGame reaction_;
  SlidingGame sliding_;
  WormGame worm_;

  static constexpr Rect HOME_BUTTON{6, 6, 52, 28};
  static constexpr Rect REACTION_CARD{6, 53, 98, 160};
  static constexpr Rect SLIDING_CARD{111, 53, 98, 160};
  static constexpr Rect WORM_CARD{216, 53, 98, 160};

  void showSplash();
  void drawHome();
  void drawGameCard(const Rect& rect, const char* title, const char* subtitle, uint8_t icon);
  void open(Screen screen);
};
