#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class WormGame {
 public:
  explicit WormGame(TFT_eSPI& display);

  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  struct Cell {
    int8_t x;
    int8_t y;
  };

  enum class State {
    Intro,
    Running,
    Paused,
    GameOver,
  };

  enum class Direction {
    Up,
    Down,
    Left,
    Right,
  };

  TFT_eSPI& tft_;
  State state_ = State::Intro;
  Direction direction_ = Direction::Right;
  Direction queuedDirection_ = Direction::Right;

  static constexpr int16_t BOARD_X = 6;
  static constexpr int16_t BOARD_Y = 45;
  static constexpr int16_t CELL_SIZE = 14;
  static constexpr uint8_t COLS = 22;
  static constexpr uint8_t ROWS = 13;
  static constexpr uint16_t MAX_BODY = COLS * ROWS;
  static constexpr uint8_t MAX_OBSTACLES = 20;
  static constexpr Rect PAUSE_BUTTON{278, 7, 35, 27};
  static constexpr Rect START_BUTTON{91, 177, 138, 40};
  static constexpr Rect AGAIN_BUTTON{89, 177, 142, 40};

  Cell body_[MAX_BODY]{};
  Cell obstacles_[MAX_OBSTACLES]{};
  Cell food_{};
  Cell bonus_{};
  uint16_t length_ = 0;
  uint8_t obstacleCount_ = 0;
  uint16_t eaten_ = 0;
  uint32_t score_ = 0;
  uint32_t highScore_ = 0;
  uint32_t nextMoveAt_ = 0;
  uint16_t moveInterval_ = 165;
  bool bonusActive_ = false;
  uint32_t bonusUntil_ = 0;

  void drawIntro();
  void startGame(uint32_t now);
  void drawGameFrame();
  void drawHud();
  void drawPauseButton();
  void drawCell(const Cell& cell, uint16_t color, bool rounded = true);
  void clearCell(const Cell& cell);
  void drawFood();
  void drawBonus();
  void drawObstacle(const Cell& cell);
  void step(uint32_t now);
  void setDirectionFromSwipe(int16_t dx, int16_t dy);
  bool isOpposite(Direction a, Direction b) const;
  bool sameCell(const Cell& a, const Cell& b) const;
  bool bodyOccupies(const Cell& cell, uint16_t limit) const;
  bool obstacleOccupies(const Cell& cell) const;
  bool freeCell(const Cell& cell) const;
  void spawnFood();
  void spawnBonus(uint32_t now);
  void addObstacle();
  void showPauseOverlay();
  void hidePauseOverlay();
  void endGame();
  void drawGameOver();
};
