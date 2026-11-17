#pragma once

#include <TFT_eSPI.h>
#include "Types.h"

// Solvable number-tile puzzle: each scramble is a sequence of legal moves.
class TileSlideGame {
 public:
  explicit TileSlideGame(TFT_eSPI& display);
  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Phase : uint8_t { Intro, Playing, Finished };
  static constexpr int16_t BOARD_X = 10;
  static constexpr int16_t BOARD_Y = 52;
  static constexpr int16_t GAP = 3;
  static constexpr Rect EASY_BUTTON{24, 163, 130, 42};
  static constexpr Rect HARD_BUTTON{166, 163, 130, 42};
  static constexpr Rect NEW_BUTTON{200, 167, 114, 27};
  static constexpr Rect MODE_BUTTON{200, 202, 114, 27};

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  uint8_t dimension_ = 3;
  uint8_t tiles_[16]{};
  uint8_t blank_ = 8;
  uint16_t moves_ = 0;
  uint32_t startedAt_ = 0;
  uint32_t lastClock_ = 0;
  bool sliding_ = false;
  uint8_t from_ = 0;
  uint8_t to_ = 0;
  uint8_t moving_ = 0;
  uint8_t slideStep_ = 0;
  uint32_t lastSlideFrame_ = 0;

  int16_t tileSize() const;
  int16_t pitch() const;
  void tilePosition(uint8_t index, int16_t& x, int16_t& y) const;
  void drawIntro();
  void startGame(uint32_t now);
  void scramble();
  bool solved() const;
  uint16_t manhattan() const;
  void drawBoard();
  void drawCell(uint8_t index);
  void drawNumber(int16_t x, int16_t y, uint8_t value);
  void drawSide(uint32_t now);
  int8_t tileAt(uint16_t x, uint16_t y) const;
  void beginSlide(uint8_t index, uint32_t now);
  void animateSlide(uint32_t now);
};
