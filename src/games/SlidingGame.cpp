#include "games/SlidingGame.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t SlidingGame::BOARD_SIZE;
constexpr uint8_t SlidingGame::PIECE_COUNT;
constexpr int16_t SlidingGame::GRID_X;
constexpr int16_t SlidingGame::GRID_Y;
constexpr int16_t SlidingGame::CELL;
constexpr Rect SlidingGame::MOVE_NEG_BUTTON;
constexpr Rect SlidingGame::MOVE_POS_BUTTON;
constexpr Rect SlidingGame::RESTART_BUTTON;
constexpr Rect SlidingGame::NEW_BUTTON;
constexpr Rect SlidingGame::NEXT_BUTTON;

SlidingGame::SlidingGame(TFT_eSPI& display) : tft_(display) {}

void SlidingGame::enter() {
  if (puzzle_ == 0) {
    puzzle_ = 1;
  }
  createPuzzle();
  drawScreen();
}

void SlidingGame::loadSolvedTemplate() {
  pieces_[0] = Piece{2, 3, 2, true};
  pieces_[1] = Piece{0, 0, 2, false};
  pieces_[2] = Piece{0, 1, 2, true};
  pieces_[3] = Piece{0, 3, 2, false};
  pieces_[4] = Piece{0, 4, 2, false};
  pieces_[5] = Piece{1, 1, 2, true};
  pieces_[6] = Piece{2, 0, 2, false};
  pieces_[7] = Piece{3, 1, 2, true};
  pieces_[8] = Piece{3, 4, 2, false};
  pieces_[9] = Piece{4, 0, 3, true};
}

void SlidingGame::createPuzzle() {
  bool good = false;
  for (uint8_t attempt = 0; attempt < 12 && !good; ++attempt) {
    loadSolvedTemplate();

    // Build an original unsolved position through legal reversible moves.
    movePiece(0, -1, false);
    movePiece(0, -1, false);
    movePiece(3, 1, false);
    movePiece(4, 1, false);

    shuffle(static_cast<uint16_t>(30 + std::min<uint16_t>(puzzle_, 12) * 8));
    good = pieces_[0].col <= 1 && pathIsBlocked();
  }

  if (!good) {
    loadSolvedTemplate();
    movePiece(0, -1, false);
    movePiece(0, -1, false);
    movePiece(3, 1, false);
    movePiece(4, 1, false);
  }

  moves_ = 0;
  selected_ = 0;
  solved_ = false;
  saveInitialState();
}

void SlidingGame::saveInitialState() {
  std::memcpy(initialPieces_, pieces_, sizeof(pieces_));
}

void SlidingGame::restartPuzzle() {
  std::memcpy(pieces_, initialPieces_, sizeof(pieces_));
  selected_ = 0;
  moves_ = 0;
  solved_ = false;
  drawScreen();
}

int8_t SlidingGame::pieceAt(uint8_t row, uint8_t col, int8_t ignore) const {
  for (uint8_t i = 0; i < PIECE_COUNT; ++i) {
    if (static_cast<int8_t>(i) == ignore) {
      continue;
    }
    const Piece& piece = pieces_[i];
    for (uint8_t offset = 0; offset < piece.length; ++offset) {
      const int8_t r = piece.row + (piece.horizontal ? 0 : offset);
      const int8_t c = piece.col + (piece.horizontal ? offset : 0);
      if (r == row && c == col) {
        return static_cast<int8_t>(i);
      }
    }
  }
  return -1;
}

bool SlidingGame::canMove(uint8_t index, int8_t direction) const {
  if (index >= PIECE_COUNT || (direction != -1 && direction != 1)) {
    return false;
  }

  const Piece& piece = pieces_[index];
  const int8_t nextRow = piece.row + (piece.horizontal ? 0 : direction);
  const int8_t nextCol = piece.col + (piece.horizontal ? direction : 0);

  for (uint8_t offset = 0; offset < piece.length; ++offset) {
    const int8_t row = nextRow + (piece.horizontal ? 0 : offset);
    const int8_t col = nextCol + (piece.horizontal ? offset : 0);
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
      return false;
    }
    if (pieceAt(static_cast<uint8_t>(row), static_cast<uint8_t>(col), static_cast<int8_t>(index)) >= 0) {
      return false;
    }
  }
  return true;
}

void SlidingGame::movePiece(uint8_t index, int8_t direction, bool countMove) {
  if (!canMove(index, direction)) {
    return;
  }

  if (pieces_[index].horizontal) {
    pieces_[index].col += direction;
  } else {
    pieces_[index].row += direction;
  }

  if (countMove) {
    ++moves_;
  }
}

bool SlidingGame::pathIsBlocked() const {
  const Piece& target = pieces_[0];
  const int8_t firstCell = target.col + target.length;
  for (int8_t col = firstCell; col < BOARD_SIZE; ++col) {
    const int8_t occupant = pieceAt(static_cast<uint8_t>(target.row), static_cast<uint8_t>(col));
    if (occupant > 0) {
      return true;
    }
  }
  return false;
}

void SlidingGame::shuffle(uint16_t steps) {
  int8_t lastPiece = -1;
  int8_t lastDirection = 0;

  for (uint16_t step = 0; step < steps; ++step) {
    struct Candidate {
      uint8_t piece;
      int8_t direction;
    };
    Candidate candidates[20];
    uint8_t count = 0;

    for (uint8_t piece = 0; piece < PIECE_COUNT; ++piece) {
      for (int8_t direction : {-1, 1}) {
        if (static_cast<int8_t>(piece) == lastPiece && direction == -lastDirection) {
          continue;
        }
        if (!canMove(piece, direction)) {
          continue;
        }
        if (piece == 0) {
          const int8_t nextCol = pieces_[0].col + direction;
          if (nextCol > 1) {
            continue;
          }
        }
        if (count < 20) {
          candidates[count++] = Candidate{piece, direction};
        }
      }
    }

    if (count == 0) {
      break;
    }

    const Candidate choice = candidates[random(0, count)];
    movePiece(choice.piece, choice.direction, false);
    lastPiece = static_cast<int8_t>(choice.piece);
    lastDirection = choice.direction;
  }
}

bool SlidingGame::isSolved() const {
  return pieces_[0].row == 2 && pieces_[0].col == 3;
}

uint16_t SlidingGame::pieceColor(uint8_t index) const {
  static constexpr uint16_t colors[] = {
      Theme::CYAN, Theme::PURPLE, Theme::PINK, Theme::ORANGE, Theme::LIME,
      0x6D7F, Theme::YELLOW, 0xA4BF, 0xF81F, 0x7FE0,
  };
  return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

void SlidingGame::drawPiece(uint8_t index) {
  const Piece& piece = pieces_[index];
  const int16_t x = GRID_X + piece.col * CELL + 3;
  const int16_t y = GRID_Y + piece.row * CELL + 3;
  const int16_t w = (piece.horizontal ? piece.length * CELL : CELL) - 6;
  const int16_t h = (piece.horizontal ? CELL : piece.length * CELL) - 6;
  const uint16_t color = pieceColor(index);

  tft_.fillRoundRect(x, y, w, h, 7, color);
  tft_.drawRoundRect(x, y, w, h, 7, index == selected_ ? Theme::WHITE : Theme::BORDER);
  if (index == selected_) {
    tft_.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 5, Theme::WHITE);
  }

  if (index == 0) {
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::BG, color);
    tft_.drawString("EXIT", x + w / 2, y + h / 2);
  } else {
    const int16_t cx = x + w / 2;
    const int16_t cy = y + h / 2;
    if (piece.horizontal) {
      tft_.drawFastHLine(cx - 8, cy, 16, Theme::BG_2);
      tft_.fillTriangle(cx - 10, cy, cx - 5, cy - 4, cx - 5, cy + 4, Theme::BG_2);
      tft_.fillTriangle(cx + 10, cy, cx + 5, cy - 4, cx + 5, cy + 4, Theme::BG_2);
    } else {
      tft_.drawFastVLine(cx, cy - 8, 16, Theme::BG_2);
      tft_.fillTriangle(cx, cy - 10, cx - 4, cy - 5, cx + 4, cy - 5, Theme::BG_2);
      tft_.fillTriangle(cx, cy + 10, cx - 4, cy + 5, cx + 4, cy + 5, Theme::BG_2);
    }
  }
}

void SlidingGame::drawBoard() {
  tft_.fillRoundRect(GRID_X - 2, GRID_Y - 2, CELL * BOARD_SIZE + 4, CELL * BOARD_SIZE + 4, 8,
                     Theme::PANEL);
  for (uint8_t row = 0; row < BOARD_SIZE; ++row) {
    for (uint8_t col = 0; col < BOARD_SIZE; ++col) {
      const int16_t x = GRID_X + col * CELL;
      const int16_t y = GRID_Y + row * CELL;
      tft_.drawRect(x, y, CELL, CELL, Theme::GRID);
    }
  }

  const int16_t exitY = GRID_Y + 2 * CELL + CELL / 2;
  tft_.fillTriangle(GRID_X + CELL * BOARD_SIZE + 2, exitY,
                    GRID_X + CELL * BOARD_SIZE - 5, exitY - 7,
                    GRID_X + CELL * BOARD_SIZE - 5, exitY + 7, Theme::CYAN);

  for (uint8_t i = 0; i < PIECE_COUNT; ++i) {
    drawPiece(i);
  }
}

void SlidingGame::drawMoveButtons() {
  const bool horizontal = pieces_[selected_].horizontal;
  const bool canNegative = canMove(static_cast<uint8_t>(selected_), -1);
  const bool canPositive = canMove(static_cast<uint8_t>(selected_), 1);

  const uint16_t negFill = canNegative ? Theme::CYAN_DARK : Theme::PANEL;
  const uint16_t posFill = canPositive ? Theme::CYAN_DARK : Theme::PANEL;
  const uint16_t negText = canNegative ? Theme::WHITE : Theme::MUTED;
  const uint16_t posText = canPositive ? Theme::WHITE : Theme::MUTED;

  Ui::drawButton(tft_, MOVE_NEG_BUTTON, horizontal ? "<" : "^", negFill, negText,
                 canNegative ? Theme::CYAN : Theme::BORDER);
  Ui::drawButton(tft_, MOVE_POS_BUTTON, horizontal ? ">" : "v", posFill, posText,
                 canPositive ? Theme::CYAN : Theme::BORDER);
}

void SlidingGame::drawSidePanel() {
  tft_.fillRect(196, 41, 124, 199, Theme::BG);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("PUZZLE", 202, 51);
  tft_.drawString("MOVES", 262, 51);

  char text[20];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(puzzle_));
  tft_.drawString(text, 202, 67);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(moves_));
  tft_.drawString(text, 262, 67);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("SELECT A BLOCK", 202, 94);
  drawMoveButtons();
  Ui::drawButton(tft_, RESTART_BUTTON, "RESTART");
  Ui::drawButton(tft_, NEW_BUTTON, "NEW PUZZLE", Theme::PANEL, Theme::MUTED, Theme::BORDER);
}

void SlidingGame::drawSolvedOverlay() {
  tft_.fillRoundRect(42, 91, 236, 122, 12, Theme::BG_2);
  tft_.drawRoundRect(42, 91, 236, 122, 12, Theme::CYAN);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::LIME, Theme::BG_2);
  tft_.drawString("ESCAPED!", 160, 119);

  char text[40];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%u moves", static_cast<unsigned>(moves_));
  tft_.drawString(text, 160, 146);

  Ui::drawButton(tft_, NEXT_BUTTON, "NEXT PUZZLE", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void SlidingGame::drawScreen() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "SHIFT VAULT");
  drawBoard();
  drawSidePanel();
}

void SlidingGame::update(const InputFrame& input, uint32_t now) {
  (void)now;

  if (!input.pressed) {
    return;
  }

  if (solved_) {
    if (contains(NEXT_BUTTON, input.x, input.y)) {
      ++puzzle_;
      createPuzzle();
      drawScreen();
    }
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

  if (contains(MOVE_NEG_BUTTON, input.x, input.y)) {
    if (canMove(static_cast<uint8_t>(selected_), -1)) {
      movePiece(static_cast<uint8_t>(selected_), -1, true);
      drawBoard();
      drawSidePanel();
    }
  } else if (contains(MOVE_POS_BUTTON, input.x, input.y)) {
    if (canMove(static_cast<uint8_t>(selected_), 1)) {
      movePiece(static_cast<uint8_t>(selected_), 1, true);
      drawBoard();
      drawSidePanel();
    }
  } else if (input.x >= GRID_X && input.x < GRID_X + CELL * BOARD_SIZE && input.y >= GRID_Y &&
             input.y < GRID_Y + CELL * BOARD_SIZE) {
    const uint8_t col = static_cast<uint8_t>((input.x - GRID_X) / CELL);
    const uint8_t row = static_cast<uint8_t>((input.y - GRID_Y) / CELL);
    const int8_t found = pieceAt(row, col);
    if (found >= 0) {
      selected_ = found;
      drawBoard();
      drawSidePanel();
    }
  }

  if (isSolved()) {
    solved_ = true;
    drawSolvedOverlay();
  }
}
