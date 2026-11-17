#pragma once

#include <TFT_eSPI.h>
#include "Types.h"

// Original matching game; the cards use procedural shapes, not image assets.
class PairQuestGame {
 public:
  explicit PairQuestGame(TFT_eSPI& display);
  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Phase : uint8_t { Intro, Playing, Finished };
  static constexpr uint8_t CARDS = 16;
  static constexpr int16_t BOARD_X = 8;
  static constexpr int16_t BOARD_Y = 51;
  static constexpr int16_t CARD_W = 49;
  static constexpr int16_t CARD_H = 40;
  static constexpr int16_t GAP = 5;
  static constexpr Rect SOLO_BUTTON{24, 163, 130, 42};
  static constexpr Rect DUO_BUTTON{166, 163, 130, 42};
  static constexpr Rect NEW_BUTTON{227, 167, 87, 27};
  static constexpr Rect MODE_BUTTON{227, 202, 87, 27};

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  uint8_t values_[CARDS]{};
  bool shown_[CARDS]{};
  bool matched_[CARDS]{};
  bool solo_ = true;
  uint8_t turn_ = 1;
  uint8_t points_[2]{};
  uint8_t matches_ = 0;
  uint16_t tries_ = 0;
  int8_t first_ = -1;
  int8_t second_ = -1;
  int8_t flipIndex_ = -1;
  uint8_t flipFrame_ = 0;
  uint32_t flipAt_ = 0;
  uint32_t hideAt_ = 0;
  uint32_t startedAt_ = 0;
  uint32_t lastClock_ = 0;

  void drawIntro();
  void startGame(uint32_t now);
  void drawBoard();
  void drawCard(uint8_t index, uint8_t widthOverride = CARD_W, bool faceOverride = false);
  void drawSymbol(uint8_t kind, int16_t cx, int16_t cy);
  void drawSide(uint32_t now);
  void beginFlip(uint8_t index, uint32_t now);
  void finishFlip(uint32_t now);
  int8_t at(uint16_t x, uint16_t y) const;
};
