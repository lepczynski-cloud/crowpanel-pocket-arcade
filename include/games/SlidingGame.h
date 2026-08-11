#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class SlidingGame {
 public:
  explicit SlidingGame(TFT_eSPI& display);

  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Phase {
    Intro,
    Playing,
    Solved,
  };

  enum class Difficulty : uint8_t {
    Easy,
    Normal,
    Hard,
  };

  struct Piece {
    int8_t row;
    int8_t col;
    uint8_t length;
    bool horizontal;
  };

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  Difficulty difficulty_ = Difficulty::Normal;
  Piece pieces_[14]{};
  Piece initialPieces_[14]{};
  Piece templatePieces_[14]{};
  uint8_t pieceCount_ = 0;
  int8_t selected_ = 0;
  uint16_t moves_ = 0;
  uint16_t puzzle_ = 1;

  static constexpr uint8_t BOARD_SIZE = 6;
  static constexpr uint8_t MAX_PIECES = 14;
  static constexpr int16_t GRID_X = 5;
  static constexpr int16_t GRID_Y = 48;
  static constexpr int16_t CELL = 30;
  static constexpr Rect EASY_BUTTON{34, 151, 76, 30};
  static constexpr Rect NORMAL_BUTTON{122, 151, 76, 30};
  static constexpr Rect HARD_BUTTON{210, 151, 76, 30};
  static constexpr Rect START_BUTTON{86, 194, 148, 35};
  static constexpr Rect MOVE_NEG_BUTTON{198, 105, 54, 42};
  static constexpr Rect MOVE_POS_BUTTON{260, 105, 54, 42};
  static constexpr Rect DIFFICULTY_BUTTON{198, 153, 116, 25};
  static constexpr Rect RESTART_BUTTON{198, 183, 116, 24};
  static constexpr Rect NEW_BUTTON{198, 212, 116, 24};
  static constexpr Rect NEXT_BUTTON{79, 174, 162, 40};

  void drawIntro();
  void drawDifficultyButtons();
  void createPuzzle();
  bool generateSolvedLayout();
  bool addPiece(const Piece& piece, bool occupied[BOARD_SIZE][BOARD_SIZE]);
  void saveInitialState();
  void restartPuzzle();
  int8_t pieceAt(uint8_t row, uint8_t col, int8_t ignore = -1) const;
  bool canMove(uint8_t index, int8_t direction) const;
  bool movePiece(uint8_t index, int8_t direction, bool countMove);
  uint8_t movePieceBy(uint8_t index, int8_t direction, uint8_t cells, bool countGesture);
  void shuffle(uint16_t steps);
  uint8_t blockersOnExitPath() const;
  uint8_t movedPieceCount() const;
  uint16_t puzzleScore() const;
  bool isSolved() const;
  uint8_t desiredPieceCount() const;
  uint16_t shuffleSteps() const;
  const char* difficultyLabel() const;
  void cycleDifficulty();

  void drawScreen();
  void drawBoard();
  void drawPiece(uint8_t index);
  void drawExit();
  void drawSidePanel();
  void drawMoveButtons();
  void drawSolvedOverlay();
  void animateEscape();
  uint16_t pieceColor(uint8_t index) const;
  void selectAt(uint16_t x, uint16_t y);
  void moveFromSwipe(const InputFrame& input);
  void checkSolved();
};
