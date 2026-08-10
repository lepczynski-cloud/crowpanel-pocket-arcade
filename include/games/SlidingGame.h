#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class SlidingGame {
 public:
  explicit SlidingGame(TFT_eSPI& display);

  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  struct Piece {
    int8_t row;
    int8_t col;
    uint8_t length;
    bool horizontal;
  };

  TFT_eSPI& tft_;
  Piece pieces_[10]{};
  Piece initialPieces_[10]{};
  int8_t selected_ = 0;
  uint16_t moves_ = 0;
  uint16_t puzzle_ = 1;
  bool solved_ = false;

  static constexpr uint8_t BOARD_SIZE = 5;
  static constexpr uint8_t PIECE_COUNT = 10;
  static constexpr int16_t GRID_X = 8;
  static constexpr int16_t GRID_Y = 47;
  static constexpr int16_t CELL = 36;
  static constexpr Rect MOVE_NEG_BUTTON{201, 116, 50, 48};
  static constexpr Rect MOVE_POS_BUTTON{260, 116, 50, 48};
  static constexpr Rect RESTART_BUTTON{201, 171, 109, 33};
  static constexpr Rect NEW_BUTTON{201, 210, 109, 27};
  static constexpr Rect NEXT_BUTTON{79, 164, 162, 40};

  void loadSolvedTemplate();
  void createPuzzle();
  void saveInitialState();
  void restartPuzzle();
  bool canMove(uint8_t index, int8_t direction) const;
  void movePiece(uint8_t index, int8_t direction, bool countMove);
  int8_t pieceAt(uint8_t row, uint8_t col, int8_t ignore = -1) const;
  bool pathIsBlocked() const;
  void shuffle(uint16_t steps);
  bool isSolved() const;

  void drawScreen();
  void drawBoard();
  void drawPiece(uint8_t index);
  void drawSidePanel();
  void drawMoveButtons();
  void drawSolvedOverlay();
  uint16_t pieceColor(uint8_t index) const;
};
