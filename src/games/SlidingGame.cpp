#include "games/SlidingGame.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t SlidingGame::BOARD_SIZE;
constexpr uint8_t SlidingGame::MAX_PIECES;
constexpr uint8_t SlidingGame::EXIT_ROW;
constexpr int16_t SlidingGame::GRID_X;
constexpr int16_t SlidingGame::GRID_Y;
constexpr int16_t SlidingGame::CELL;
constexpr int16_t SlidingGame::SIDE_X;
constexpr Rect SlidingGame::EASY_BUTTON;
constexpr Rect SlidingGame::NORMAL_BUTTON;
constexpr Rect SlidingGame::HARD_BUTTON;
constexpr Rect SlidingGame::START_BUTTON;
constexpr Rect SlidingGame::DIFFICULTY_BUTTON;
constexpr Rect SlidingGame::RESTART_BUTTON;
constexpr Rect SlidingGame::NEW_BUTTON;
constexpr Rect SlidingGame::NEXT_BUTTON;

SlidingGame::SlidingGame(TFT_eSPI& display) : tft_(display) {}

void SlidingGame::enter() {
  phase_ = Phase::Intro;
  drawIntro();
}

const char* SlidingGame::difficultyLabel() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return "EASY";
    case Difficulty::Hard:
      return "HARD";
    case Difficulty::Normal:
    default:
      return "NORMAL";
  }
}

void SlidingGame::drawDifficultyButtons() {
  const Rect buttons[] = {EASY_BUTTON, NORMAL_BUTTON, HARD_BUTTON};
  const char* labels[] = {"EASY", "NORMAL", "HARD"};
  const Difficulty values[] = {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard};

  for (uint8_t i = 0; i < 3; ++i) {
    const bool selected = difficulty_ == values[i];
    Ui::drawButton(tft_, buttons[i], labels[i], selected ? Theme::CYAN_DARK : Theme::PANEL,
                   selected ? Theme::WHITE : Theme::MUTED,
                   selected ? Theme::CYAN : Theme::BORDER);
  }
}

void SlidingGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "SHIFT VAULT");
  Ui::drawBlocksIcon(tft_, 155, 87, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Guide the gold key to the bright exit", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Every block can move in all four directions", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

uint8_t SlidingGame::shapeCells(Shape shape, CellOffset cells[4]) const {
  switch (shape) {
    case Shape::Target:
    case Shape::DominoH:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{0, 1};
      return 2;
    case Shape::DominoV:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{1, 0};
      return 2;
    case Shape::Bar3H:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{0, 1};
      cells[2] = CellOffset{0, 2};
      return 3;
    case Shape::Bar3V:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{1, 0};
      cells[2] = CellOffset{2, 0};
      return 3;
    case Shape::Square:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{0, 1};
      cells[2] = CellOffset{1, 0};
      cells[3] = CellOffset{1, 1};
      return 4;
    case Shape::LUpLeft:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{1, 0};
      cells[2] = CellOffset{1, 1};
      return 3;
    case Shape::LUpRight:
      cells[0] = CellOffset{0, 1};
      cells[1] = CellOffset{1, 0};
      cells[2] = CellOffset{1, 1};
      return 3;
    case Shape::LDownLeft:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{0, 1};
      cells[2] = CellOffset{1, 0};
      return 3;
    case Shape::LDownRight:
    default:
      cells[0] = CellOffset{0, 0};
      cells[1] = CellOffset{0, 1};
      cells[2] = CellOffset{1, 1};
      return 3;
  }
}

bool SlidingGame::pieceContainsCell(const Piece& piece, uint8_t row, uint8_t col) const {
  CellOffset cells[4]{};
  const uint8_t count = shapeCells(piece.shape, cells);
  for (uint8_t i = 0; i < count; ++i) {
    if (piece.row + cells[i].row == row && piece.col + cells[i].col == col) {
      return true;
    }
  }
  return false;
}

uint8_t SlidingGame::desiredPieceCount() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 7;
    case Difficulty::Hard:
      return 10;
    case Difficulty::Normal:
    default:
      return 9;
  }
}

uint16_t SlidingGame::shuffleSteps() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 100;
    case Difficulty::Hard:
      return 520;
    case Difficulty::Normal:
    default:
      return 260;
  }
}

uint8_t SlidingGame::minimumBlockers() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 1;
    case Difficulty::Hard:
      return 3;
    case Difficulty::Normal:
    default:
      return 2;
  }
}

uint8_t SlidingGame::minimumMovedPieces() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 3;
    case Difficulty::Hard:
      return 7;
    case Difficulty::Normal:
    default:
      return 5;
  }
}

bool SlidingGame::addPiece(const Piece& piece, bool occupied[BOARD_SIZE][BOARD_SIZE]) {
  if (pieceCount_ >= MAX_PIECES) {
    return false;
  }

  CellOffset cells[4]{};
  const uint8_t count = shapeCells(piece.shape, cells);
  for (uint8_t i = 0; i < count; ++i) {
    const int8_t row = piece.row + cells[i].row;
    const int8_t col = piece.col + cells[i].col;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || occupied[row][col]) {
      return false;
    }
  }

  pieces_[pieceCount_++] = piece;
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t row = static_cast<uint8_t>(piece.row + cells[i].row);
    const uint8_t col = static_cast<uint8_t>(piece.col + cells[i].col);
    occupied[row][col] = true;
  }
  return true;
}

bool SlidingGame::generateScrambleBase() {
  bool occupied[BOARD_SIZE][BOARD_SIZE]{};
  pieceCount_ = 0;

  if (!addPiece(Piece{static_cast<int8_t>(EXIT_ROW), 4, Shape::Target}, occupied)) {
    return false;
  }

  static const Shape shapes[] = {
      Shape::DominoH, Shape::DominoV, Shape::Bar3H, Shape::Bar3V, Shape::Square,
      Shape::LUpLeft, Shape::LUpRight, Shape::LDownLeft, Shape::LDownRight,
  };

  const uint8_t targetCount = desiredPieceCount();
  uint16_t attempts = 0;
  while (pieceCount_ < targetCount && attempts < 1600) {
    ++attempts;
    Shape shape = shapes[random(0, static_cast<long>(sizeof(shapes) / sizeof(shapes[0])))];
    if (difficulty_ == Difficulty::Easy &&
        (shape == Shape::Bar3V || shape == Shape::Square) && random(0, 100) < 55) {
      shape = random(0, 2) == 0 ? Shape::DominoH : Shape::DominoV;
    }

    Piece candidate{};
    candidate.shape = shape;
    candidate.row = static_cast<int8_t>(random(0, BOARD_SIZE));
    candidate.col = static_cast<int8_t>(random(0, BOARD_SIZE));

    CellOffset cells[4]{};
    const uint8_t count = shapeCells(candidate.shape, cells);
    bool touchesExitRow = false;
    for (uint8_t i = 0; i < count; ++i) {
      if (candidate.row + cells[i].row == EXIT_ROW) {
        touchesExitRow = true;
        break;
      }
    }
    if (touchesExitRow) {
      continue;
    }
    addPiece(candidate, occupied);
  }

  if (pieceCount_ != targetCount) {
    return false;
  }

  pieces_[0].col = 0;
  std::memcpy(scrambleBase_, pieces_, sizeof(Piece) * pieceCount_);
  return true;
}

void SlidingGame::loadFallbackBase() {
  static const Piece fallbackPieces[] = {
      Piece{static_cast<int8_t>(EXIT_ROW), 0, Shape::Target},
      Piece{0, 0, Shape::Square},
      Piece{0, 2, Shape::DominoH},
      Piece{0, 4, Shape::DominoV},
      Piece{3, 3, Shape::LDownRight},
      Piece{1, 2, Shape::DominoH},
      Piece{3, 0, Shape::Bar3H},
      Piece{0, 5, Shape::DominoV},
      Piece{4, 0, Shape::Square},
      Piece{5, 3, Shape::Bar3H},
  };

  pieceCount_ = desiredPieceCount();
  std::memcpy(pieces_, fallbackPieces, sizeof(Piece) * pieceCount_);
  std::memcpy(scrambleBase_, pieces_, sizeof(Piece) * pieceCount_);
}

int8_t SlidingGame::pieceAt(uint8_t row, uint8_t col, int8_t ignore) const {
  for (uint8_t i = 0; i < pieceCount_; ++i) {
    if (static_cast<int8_t>(i) == ignore) {
      continue;
    }
    if (pieceContainsCell(pieces_[i], row, col)) {
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

bool SlidingGame::canMove(uint8_t index, int8_t deltaRow, int8_t deltaCol) const {
  if (index >= pieceCount_ || (deltaRow == 0 && deltaCol == 0)) {
    return false;
  }

  const Piece& piece = pieces_[index];
  CellOffset cells[4]{};
  const uint8_t count = shapeCells(piece.shape, cells);
  for (uint8_t i = 0; i < count; ++i) {
    const int8_t row = piece.row + cells[i].row + deltaRow;
    const int8_t col = piece.col + cells[i].col + deltaCol;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
      return false;
    }
    if (pieceAt(static_cast<uint8_t>(row), static_cast<uint8_t>(col),
                static_cast<int8_t>(index)) >= 0) {
      return false;
    }
  }
  return true;
}

bool SlidingGame::movePiece(uint8_t index, int8_t deltaRow, int8_t deltaCol, bool countMove) {
  if (!canMove(index, deltaRow, deltaCol)) {
    return false;
  }
  pieces_[index].row += deltaRow;
  pieces_[index].col += deltaCol;
  if (countMove) {
    ++moves_;
  }
  return true;
}

uint8_t SlidingGame::movePieceBy(uint8_t index, int8_t deltaRow, int8_t deltaCol,
                                 uint8_t cells, bool countGesture) {
  uint8_t moved = 0;
  while (moved < cells && canMove(index, deltaRow, deltaCol)) {
    pieces_[index].row += deltaRow;
    pieces_[index].col += deltaCol;
    ++moved;
  }
  if (moved > 0 && countGesture) {
    ++moves_;
  }
  return moved;
}

void SlidingGame::shuffle(uint16_t steps) {
  int8_t lastPiece = -1;
  int8_t lastRow = 0;
  int8_t lastCol = 0;

  for (uint16_t step = 0; step < steps; ++step) {
    bool moved = false;
    for (uint8_t attempt = 0; attempt < 28 && !moved; ++attempt) {
      const uint8_t index = static_cast<uint8_t>(random(1, pieceCount_));
      static const int8_t directions[4][2] = {
          {-1, 0}, {1, 0}, {0, -1}, {0, 1},
      };

      uint8_t directionIndex = static_cast<uint8_t>(random(0, 4));
      CellOffset cells[4]{};
      const uint8_t count = shapeCells(pieces_[index].shape, cells);
      bool aboveExit = false;
      bool belowExit = false;
      for (uint8_t i = 0; i < count; ++i) {
        const int8_t row = pieces_[index].row + cells[i].row;
        aboveExit = aboveExit || row == EXIT_ROW - 1;
        belowExit = belowExit || row == EXIT_ROW + 1;
      }
      if (random(0, 100) < 34) {
        if (aboveExit) {
          directionIndex = 1;
        } else if (belowExit) {
          directionIndex = 0;
        }
      }

      const int8_t deltaRow = directions[directionIndex][0];
      const int8_t deltaCol = directions[directionIndex][1];
      if (static_cast<int8_t>(index) == lastPiece &&
          deltaRow == -lastRow && deltaCol == -lastCol) {
        continue;
      }
      if (movePiece(index, deltaRow, deltaCol, false)) {
        lastPiece = static_cast<int8_t>(index);
        lastRow = deltaRow;
        lastCol = deltaCol;
        moved = true;
      }
    }
  }
}

uint8_t SlidingGame::blockersOnExitPath() const {
  uint8_t blockers = 0;
  const int8_t firstCol = pieces_[0].col + 2;
  for (uint8_t i = 1; i < pieceCount_; ++i) {
    bool blocks = false;
    CellOffset cells[4]{};
    const uint8_t count = shapeCells(pieces_[i].shape, cells);
    for (uint8_t cell = 0; cell < count; ++cell) {
      const int8_t row = pieces_[i].row + cells[cell].row;
      const int8_t col = pieces_[i].col + cells[cell].col;
      if (row == EXIT_ROW && col >= firstCol) {
        blocks = true;
        break;
      }
    }
    if (blocks) {
      ++blockers;
    }
  }
  return blockers;
}

uint8_t SlidingGame::movedPieceCount() const {
  uint8_t moved = 0;
  for (uint8_t i = 1; i < pieceCount_; ++i) {
    if (pieces_[i].row != scrambleBase_[i].row || pieces_[i].col != scrambleBase_[i].col) {
      ++moved;
    }
  }
  return moved;
}

uint16_t SlidingGame::puzzleScore() const {
  return static_cast<uint16_t>(blockersOnExitPath() * 15U + movedPieceCount() * 4U);
}

bool SlidingGame::isSolved() const {
  return pieces_[0].row == EXIT_ROW && pieces_[0].col == BOARD_SIZE - 2;
}

void SlidingGame::saveInitialState() {
  std::memcpy(initialPieces_, pieces_, sizeof(Piece) * pieceCount_);
  selected_ = 0;
  moves_ = 0;
}

void SlidingGame::createPuzzle() {
  bool ready = false;
  for (uint8_t attempt = 0; attempt < 80 && !ready; ++attempt) {
    if (!generateScrambleBase()) {
      continue;
    }
    shuffle(shuffleSteps());
    ready = blockersOnExitPath() >= minimumBlockers() &&
            movedPieceCount() >= minimumMovedPieces() &&
            puzzleScore() >= static_cast<uint16_t>(minimumBlockers() * 15U +
                                                   minimumMovedPieces() * 4U);
  }

  if (!ready) {
    loadFallbackBase();
    for (uint8_t attempt = 0; attempt < 12 && !ready; ++attempt) {
      std::memcpy(pieces_, scrambleBase_, sizeof(Piece) * pieceCount_);
      shuffle(static_cast<uint16_t>(shuffleSteps() * 2U));
      ready = blockersOnExitPath() >= minimumBlockers() &&
              movedPieceCount() >= minimumMovedPieces();
    }
  }

  saveInitialState();
  phase_ = Phase::Playing;
}

void SlidingGame::restartPuzzle() {
  std::memcpy(pieces_, initialPieces_, sizeof(Piece) * pieceCount_);
  selected_ = 0;
  moves_ = 0;
  phase_ = Phase::Playing;
  drawScreen();
}

void SlidingGame::cycleDifficulty() {
  if (difficulty_ == Difficulty::Easy) {
    difficulty_ = Difficulty::Normal;
  } else if (difficulty_ == Difficulty::Normal) {
    difficulty_ = Difficulty::Hard;
  } else {
    difficulty_ = Difficulty::Easy;
  }
  ++puzzle_;
  createPuzzle();
  drawScreen();
}

uint16_t SlidingGame::pieceColor(uint8_t index) const {
  if (index == 0) {
    return Theme::GOLD;
  }
  static const uint16_t colors[] = {
      Theme::PURPLE, Theme::CYAN_DARK, Theme::PINK, Theme::ORANGE,
      Theme::CYAN, Theme::LIME, Theme::PANEL_2,
  };
  return colors[(index - 1) % (sizeof(colors) / sizeof(colors[0]))];
}

void SlidingGame::drawPiece(uint8_t index) {
  const Piece& piece = pieces_[index];
  const uint16_t color = pieceColor(index);

  if (index == 0) {
    const int16_t x = GRID_X + piece.col * CELL + 2;
    const int16_t y = GRID_Y + piece.row * CELL + 3;
    const int16_t width = CELL * 2 - 4;
    const int16_t height = CELL - 6;
    tft_.fillRoundRect(x, y, width, height, 9, color);
    tft_.drawRoundRect(x, y, width, height, 9, Theme::WHITE);
    tft_.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 7, Theme::YELLOW);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::BLACK, color);
    tft_.drawString("KEY", x + width / 2 - 7, y + height / 2);
    tft_.fillTriangle(x + width - 7, y + height / 2,
                      x + width - 15, y + height / 2 - 6,
                      x + width - 15, y + height / 2 + 6, Theme::BLACK);
    if (selected_ == 0) {
      tft_.drawRoundRect(x - 1, y - 1, width + 2, height + 2, 10, Theme::LIME);
    }
    return;
  }

  CellOffset cells[4]{};
  const uint8_t count = shapeCells(piece.shape, cells);

  for (uint8_t a = 0; a < count; ++a) {
    for (uint8_t b = a + 1; b < count; ++b) {
      const int8_t rowDiff = cells[b].row - cells[a].row;
      const int8_t colDiff = cells[b].col - cells[a].col;
      const int16_t cellX = GRID_X + (piece.col + cells[a].col) * CELL;
      const int16_t cellY = GRID_Y + (piece.row + cells[a].row) * CELL;
      if (rowDiff == 0 && std::abs(colDiff) == 1) {
        const int16_t bridgeX = colDiff > 0 ? cellX + CELL - 5 : cellX - 5;
        tft_.fillRect(bridgeX, cellY + 5, 10, CELL - 10, color);
      } else if (colDiff == 0 && std::abs(rowDiff) == 1) {
        const int16_t bridgeY = rowDiff > 0 ? cellY + CELL - 5 : cellY - 5;
        tft_.fillRect(cellX + 5, bridgeY, CELL - 10, 10, color);
      }
    }
  }

  for (uint8_t i = 0; i < count; ++i) {
    const int16_t x = GRID_X + (piece.col + cells[i].col) * CELL + 3;
    const int16_t y = GRID_Y + (piece.row + cells[i].row) * CELL + 3;
    tft_.fillRoundRect(x, y, CELL - 6, CELL - 6, 6, color);
    tft_.drawRoundRect(x, y, CELL - 6, CELL - 6, 6,
                       index == static_cast<uint8_t>(selected_) ? Theme::WHITE : Theme::BORDER);
  }

  const int16_t markerX = GRID_X + piece.col * CELL + CELL / 2;
  const int16_t markerY = GRID_Y + piece.row * CELL + CELL / 2;
  tft_.fillCircle(markerX, markerY, 2,
                  index == static_cast<uint8_t>(selected_) ? Theme::WHITE : Theme::BG_2);
}

void SlidingGame::drawExit() {
  const int16_t x = GRID_X + BOARD_SIZE * CELL + 2;
  const int16_t y = GRID_Y + EXIT_ROW * CELL;
  tft_.fillRoundRect(x, y + 2, 17, CELL - 4, 5, Theme::BLACK);
  tft_.drawRoundRect(x, y + 2, 17, CELL - 4, 5, Theme::LIME);
  tft_.drawFastHLine(x + 1, y + 4, 15, Theme::WHITE);
  tft_.drawFastHLine(x + 1, y + CELL - 5, 15, Theme::WHITE);
  for (uint8_t i = 0; i < 2; ++i) {
    const int16_t arrowX = x + 3 + i * 6;
    tft_.fillTriangle(arrowX, y + CELL / 2 - 5,
                      arrowX, y + CELL / 2 + 5,
                      arrowX + 5, y + CELL / 2, Theme::LIME);
  }
}

void SlidingGame::drawBoard() {
  const int16_t boardPixels = CELL * BOARD_SIZE;
  tft_.fillRoundRect(GRID_X - 2, GRID_Y - 2, boardPixels + 4, boardPixels + 4, 8,
                     Theme::PANEL);
  tft_.fillRect(GRID_X, GRID_Y, boardPixels, boardPixels, Theme::BG_2);
  for (uint8_t row = 0; row < BOARD_SIZE; ++row) {
    for (uint8_t col = 0; col < BOARD_SIZE; ++col) {
      const int16_t x = GRID_X + col * CELL;
      const int16_t y = GRID_Y + row * CELL;
      tft_.drawRect(x, y, CELL, CELL, Theme::GRID);
    }
  }

  drawExit();
  for (uint8_t i = 0; i < pieceCount_; ++i) {
    drawPiece(i);
  }
}

void SlidingGame::drawSidePanel() {
  tft_.fillRect(SIDE_X, 40, 320 - SIDE_X, 200, Theme::BG);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("PUZZLE", SIDE_X + 2, 49);
  tft_.drawString("MOVES", SIDE_X + 64, 49);

  char text[28];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(puzzle_));
  tft_.drawString(text, SIDE_X + 2, 65);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(moves_));
  tft_.drawString(text, SIDE_X + 64, 65);

  tft_.fillRoundRect(SIDE_X, 83, 114, 58, 8, Theme::PANEL);
  tft_.drawRoundRect(SIDE_X, 83, 114, 58, 8, Theme::LIME);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::LIME, Theme::PANEL);
  tft_.drawString("EXIT", SIDE_X + 57, 98);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::WHITE, Theme::PANEL);
  tft_.drawString("MOVE GOLD KEY >>>", SIDE_X + 57, 118);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString("SWIPE 4 WAYS", SIDE_X + 57, 132);

  std::snprintf(text, sizeof(text), "MODE %s", difficultyLabel());
  Ui::drawButton(tft_, DIFFICULTY_BUTTON, text, Theme::PANEL, Theme::CYAN, Theme::BORDER);
  Ui::drawButton(tft_, RESTART_BUTTON, "RESTART");
  Ui::drawButton(tft_, NEW_BUTTON, "NEW PUZZLE", Theme::PANEL, Theme::MUTED, Theme::BORDER);
}

void SlidingGame::drawScreen() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "SHIFT VAULT");
  drawBoard();
  drawSidePanel();
}

void SlidingGame::animateEscape() {
  const int16_t x = GRID_X + BOARD_SIZE * CELL + 10;
  const int16_t y = GRID_Y + EXIT_ROW * CELL + CELL / 2;
  for (uint8_t frame = 0; frame < 3; ++frame) {
    const uint16_t color = frame % 2 == 0 ? Theme::LIME : Theme::WHITE;
    tft_.drawCircle(x, y, 5 + frame * 4, color);
    delay(28);
  }
}

void SlidingGame::drawSolvedOverlay() {
  tft_.fillRoundRect(42, 83, 236, 139, 12, Theme::BG_2);
  tft_.drawRoundRect(42, 83, 236, 139, 12, Theme::LIME);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::LIME, Theme::BG_2);
  tft_.drawString("VAULT OPEN", 160, 111);

  char text[44];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%u moves   %s", static_cast<unsigned>(moves_),
                difficultyLabel());
  tft_.drawString(text, 160, 145);

  Ui::drawButton(tft_, NEXT_BUTTON, "NEXT PUZZLE", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void SlidingGame::selectAt(uint16_t x, uint16_t y) {
  if (x < GRID_X || x >= GRID_X + CELL * BOARD_SIZE ||
      y < GRID_Y || y >= GRID_Y + CELL * BOARD_SIZE) {
    return;
  }
  const uint8_t col = static_cast<uint8_t>((x - GRID_X) / CELL);
  const uint8_t row = static_cast<uint8_t>((y - GRID_Y) / CELL);
  const int8_t found = pieceAt(row, col);
  if (found >= 0) {
    selected_ = found;
    drawBoard();
  }
}

void SlidingGame::moveFromSwipe(const InputFrame& input) {
  if (input.startX < GRID_X || input.startX >= GRID_X + CELL * BOARD_SIZE ||
      input.startY < GRID_Y || input.startY >= GRID_Y + CELL * BOARD_SIZE) {
    return;
  }

  const uint8_t col = static_cast<uint8_t>((input.startX - GRID_X) / CELL);
  const uint8_t row = static_cast<uint8_t>((input.startY - GRID_Y) / CELL);
  const int8_t found = pieceAt(row, col);
  if (found < 0) {
    return;
  }

  selected_ = found;
  const int16_t absX = std::abs(input.swipeDx);
  const int16_t absY = std::abs(input.swipeDy);
  if (std::max(absX, absY) < 18) {
    drawBoard();
    return;
  }

  int8_t deltaRow = 0;
  int8_t deltaCol = 0;
  int16_t distance = 0;
  if (absX >= absY) {
    deltaCol = input.swipeDx > 0 ? 1 : -1;
    distance = absX;
  } else {
    deltaRow = input.swipeDy > 0 ? 1 : -1;
    distance = absY;
  }

  const uint8_t cells = static_cast<uint8_t>(
      std::min<int>(5, std::max<int>(1, distance / CELL)));
  if (movePieceBy(static_cast<uint8_t>(selected_), deltaRow, deltaCol, cells, true) > 0) {
    drawBoard();
    drawSidePanel();
    checkSolved();
  } else {
    drawBoard();
  }
}

void SlidingGame::checkSolved() {
  if (!isSolved()) {
    return;
  }
  phase_ = Phase::Solved;
  animateEscape();
  drawSolvedOverlay();
}

void SlidingGame::update(const InputFrame& input, uint32_t now) {
  (void)now;

  if (phase_ == Phase::Intro) {
    if (!input.pressed) {
      return;
    }
    if (contains(EASY_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Easy;
      drawDifficultyButtons();
    } else if (contains(NORMAL_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Normal;
      drawDifficultyButtons();
    } else if (contains(HARD_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Hard;
      drawDifficultyButtons();
    } else if (contains(START_BUTTON, input.x, input.y)) {
      puzzle_ = 1;
      createPuzzle();
      drawScreen();
    }
    return;
  }

  if (phase_ == Phase::Solved) {
    if (input.pressed && contains(NEXT_BUTTON, input.x, input.y)) {
      ++puzzle_;
      createPuzzle();
      drawScreen();
    }
    return;
  }

  if (input.pressed) {
    if (contains(DIFFICULTY_BUTTON, input.x, input.y)) {
      cycleDifficulty();
      return;
    }
    if (contains(RESTART_BUTTON, input.x, input.y)) {
      restartPuzzle();
      return;
    }
    if (contains(NEW_BUTTON, input.x, input.y)) {
      ++puzzle_;
      createPuzzle();
      drawScreen();
      return;
    }
    selectAt(input.x, input.y);
  }

  if (input.swipe) {
    moveFromSwipe(input);
  }
}
