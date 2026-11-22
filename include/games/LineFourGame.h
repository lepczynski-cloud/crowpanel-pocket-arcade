#pragma once

#include <TFT_eSPI.h>
#include "Types.h"

// Original four-in-a-row implementation with a lightweight local opponent.
class LineFourGame {
 public:
  explicit LineFourGame(TFT_eSPI& display);
  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Phase : uint8_t { Intro, Playing, Finished };
  static constexpr uint8_t ROWS = 6;
  static constexpr uint8_t COLS = 7;
  static constexpr int16_t BOARD_X = 7;
  static constexpr int16_t BOARD_Y = 49;
  static constexpr int16_t CELL = 29;
  static constexpr Rect SOLO_BUTTON{24, 163, 130, 42};
  static constexpr Rect DUO_BUTTON{166, 163, 130, 42};
  static constexpr Rect NEW_BUTTON{221, 167, 93, 27};
  static constexpr Rect MODE_BUTTON{221, 202, 93, 27};

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  uint8_t board_[ROWS][COLS]{};
  bool solo_ = true;
  uint8_t turn_ = 1;  // 1 is cyan; 2 is pink
  uint8_t winner_ = 0;  // 3 means a draw
  uint8_t winningRows_[4]{};
  uint8_t winningCols_[4]{};
  uint8_t count_ = 0;
  bool dropping_ = false;
  uint8_t dropCol_ = 0;
  uint8_t dropRow_ = 0;
  int16_t dropY_ = 0;
  uint32_t lastDropFrame_ = 0;
  uint32_t cpuReadyAt_ = 0;

  void drawIntro();
  void drawGame();
  void drawCell(uint8_t row, uint8_t col);
  void drawColumn(uint8_t col);
  void drawDisc(int16_t x, int16_t y, uint8_t player, uint8_t radius = 11);
  void drawSideFrame();
  void drawStatus();
  void newGame(uint32_t now);
  int8_t landingRow(uint8_t col) const;
  bool checkWin(uint8_t row, uint8_t col, uint8_t player, bool store);
  bool wouldWin(uint8_t col, uint8_t player);
  uint8_t chooseCpuColumn();
  void startDrop(uint8_t col, uint32_t now);
  void finishDrop(uint32_t now);
};
