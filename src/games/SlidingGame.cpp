#include "games/SlidingGame.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t SlidingGame::BOARD_SIZE;
constexpr uint8_t SlidingGame::MAX_PIECES;
constexpr int16_t SlidingGame::GRID_X;
constexpr int16_t SlidingGame::GRID_Y;
constexpr int16_t SlidingGame::CELL;
constexpr Rect SlidingGame::EASY_BUTTON;
constexpr Rect SlidingGame::NORMAL_BUTTON;
constexpr Rect SlidingGame::HARD_BUTTON;
constexpr Rect SlidingGame::START_BUTTON;
constexpr Rect SlidingGame::MOVE_NEG_BUTTON;
constexpr Rect SlidingGame::MOVE_POS_BUTTON;
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
  Ui::drawSparkles(tft_, 18, 44, 228);
  Ui::drawBlocksIcon(tft_, 155, 87, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Slide the gold key block to the exit", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap a block, then swipe along its direction", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

uint8_t SlidingGame::desiredPieceCount() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 8;
    case Difficulty::Hard:
      return 12;
    case Difficulty::Normal:
    default:
      return 10;
  }
}

uint16_t SlidingGame::shuffleSteps() const {
  switch (difficulty_) {
    case Difficulty::Easy:
      return 40;
    case Difficulty::Hard:
      return 220;
    case Difficulty::Normal:
    default:
      return 110;
  }
}

bool SlidingGame::addPiece(const Piece& piece, bool occupied[BOARD_SIZE][BOARD_SIZE]) {
  if (piece.length < 2 || piece.length > 3) {
    return false;
  }

  for (uint8_t offset = 0; offset < piece.length; ++offset) {
    const int8_t row = piece.row + (piece.horizontal ? 0 : offset);
    const int8_t col = piece.col + (piece.horizontal ? offset : 0);
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || occupied[row][col]) {
      return false;
    }
  }

  if (pieceCount_ >= MAX_PIECES) {
    return false;
  }

  pieces_[pieceCount_++] = piece;
  for (uint8_t offset = 0; offset < piece.length; ++offset) {
    const uint8_t row = static_cast<uint8_t>(piece.row + (piece.horizontal ? 0 : offset));
    const uint8_t col = static_cast<uint8_t>(piece.col + (piece.horizontal ? offset : 0));
    occupied[row][col] = true;
  }
  return true;
}

bool SlidingGame::generateSolvedLayout() {
  bool occupied[BOARD_SIZE][BOARD_SIZE]{};
  pieceCount_ = 0;

  if (!addPiece(Piece{2, 4, 2, true}, occupied)) {
    return false;
  }

  int8_t upperCol = static_cast<int8_t>(random(2, 6));
  int8_t lowerCol = static_cast<int8_t>(random(1, 6));
  if (lowerCol == upperCol) {
    lowerCol = static_cast<int8_t>((lowerCol + 2) % 6);
    if (lowerCol == 0) {
      lowerCol = 1;
    }
  }

  if (!addPiece(Piece{0, upperCol, 2, false}, occupied)) {
    return false;
  }
  if (!addPiece(Piece{3, lowerCol, 2, false}, occupied)) {
    return false;
  }

  const uint8_t targetCount = desiredPieceCount();
  uint16_t attempts = 0;
  while (pieceCount_ < targetCount && attempts < 700) {
    ++attempts;
    const bool horizontal = random(0, 100) < 58;
    const bool useLong = difficulty_ != Difficulty::Easy && random(0, 100) < 34;
    const uint8_t length = useLong ? 3 : 2;
    Piece candidate{};
    candidate.length = length;
    candidate.horizontal = horizontal;

    if (horizontal) {
      static const int8_t rows[] = {0, 1, 3, 4, 5};
      candidate.row = rows[random(0, 5)];
      candidate.col = static_cast<int8_t>(random(0, BOARD_SIZE - length + 1));
    } else {
      candidate.col = static_cast<int8_t>(random(0, BOARD_SIZE));
      if (random(0, 2) == 0) {
        candidate.row = 0;
        candidate.length = 2;
      } else {
        candidate.row = static_cast<int8_t>(length == 3 ? 3 : random(3, 5));
      }
    }

    bool crossesTargetRow = false;
    for (uint8_t offset = 0; offset < candidate.length; ++offset) {
      const int8_t row = candidate.row + (candidate.horizontal ? 0 : offset);
      if (row == 2) {
        crossesTargetRow = true;
      }
    }
    if (crossesTargetRow) {
      continue;
    }

    addPiece(candidate, occupied);
  }

  if (pieceCount_ < targetCount) {
    return false;
  }

  std::memcpy(templatePieces_, pieces_, sizeof(pieces_));
  return true;
}

int8_t SlidingGame::pieceAt(uint8_t row, uint8_t col, int8_t ignore) const {
  for (uint8_t i = 0; i < pieceCount_; ++i) {
    if (static_cast<int8_t>(i) == ignore) {
      continue;
    }
    const Piece& piece = pieces_[i];
    for (uint8_t offset = 0; offset < piece.length; ++offset) {
      const int8_t pieceRow = piece.row + (piece.horizontal ? 0 : offset);
      const int8_t pieceCol = piece.col + (piece.horizontal ? offset : 0);
      if (pieceRow == row && pieceCol == col) {
        return static_cast<int8_t>(i);
      }
    }
  }
  return -1;
}

bool SlidingGame::canMove(uint8_t index, int8_t direction) const {
  if (index >= pieceCount_ || (direction != -1 && direction != 1)) {
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
    if (pieceAt(static_cast<uint8_t>(row), static_cast<uint8_t>(col),
                static_cast<int8_t>(index)) >= 0) {
      return false;
    }
  }
  return true;
}

bool SlidingGame::movePiece(uint8_t index, int8_t direction, bool countMove) {
  if (!canMove(index, direction)) {
    return false;
  }

  if (pieces_[index].horizontal) {
    pieces_[index].col += direction;
  } else {
    pieces_[index].row += direction;
  }
  if (countMove) {
    ++moves_;
  }
  return true;
}

uint8_t SlidingGame::movePieceBy(uint8_t index, int8_t direction, uint8_t cells,
                                 bool countGesture) {
  uint8_t moved = 0;
  while (moved < cells && movePiece(index, direction, false)) {
    ++moved;
  }
  if (moved > 0 && countGesture) {
    ++moves_;
  }
  return moved;
}

void SlidingGame::shuffle(uint16_t steps) {
  int8_t lastPiece = -1;
  int8_t lastDirection = 0;

  for (uint16_t step = 0; step < steps; ++step) {
    struct Candidate {
      uint8_t piece;
      int8_t direction;
    };
    Candidate candidates[MAX_PIECES * 2];
    uint8_t count = 0;

    for (uint8_t piece = 1; piece < pieceCount_; ++piece) {
      for (int8_t direction = -1; direction <= 1; direction += 2) {
        if (static_cast<int8_t>(piece) == lastPiece && direction == -lastDirection) {
          continue;
        }
        if (canMove(piece, direction) && count < MAX_PIECES * 2) {
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

uint8_t SlidingGame::blockersOnExitPath() const {
  bool seen[MAX_PIECES]{};
  uint8_t blockers = 0;
  const Piece& target = pieces_[0];
  const int8_t startCol = target.col + target.length;
  for (int8_t col = startCol; col < BOARD_SIZE; ++col) {
    const int8_t index = pieceAt(static_cast<uint8_t>(target.row), static_cast<uint8_t>(col));
    if (index > 0 && !seen[index]) {
      seen[index] = true;
      ++blockers;
    }
  }
  return blockers;
}

uint8_t SlidingGame::movedPieceCount() const {
  uint8_t moved = 0;
  for (uint8_t i = 0; i < pieceCount_; ++i) {
    if (pieces_[i].row != templatePieces_[i].row || pieces_[i].col != templatePieces_[i].col) {
      ++moved;
    }
  }
  return moved;
}

uint16_t SlidingGame::puzzleScore() const {
  const uint16_t blockers = blockersOnExitPath();
  const uint16_t moved = movedPieceCount();
  const uint16_t distance = static_cast<uint16_t>(4 - pieces_[0].col);
  return blockers * 80U + moved * 9U + distance * 14U;
}

void SlidingGame::createPuzzle() {
  Piece bestPieces[MAX_PIECES]{};
  Piece bestTemplate[MAX_PIECES]{};
  Piece fallbackPieces[MAX_PIECES]{};
  Piece fallbackTemplate[MAX_PIECES]{};
  uint8_t bestCount = 0;
  uint8_t fallbackCount = 0;
  uint16_t bestScore = 0;
  uint16_t fallbackScore = 0;
  uint32_t bestMetric = 0xFFFFFFFFUL;

  uint8_t minimumBlockers = 2;
  uint8_t minimumMoved = 5;
  uint8_t attempts = 48;
  if (difficulty_ == Difficulty::Easy) {
    minimumBlockers = 1;
    minimumMoved = 0;
    attempts = 32;
  } else if (difficulty_ == Difficulty::Hard) {
    minimumBlockers = 3;
    minimumMoved = 8;
    attempts = 64;
  }

  for (uint8_t attempt = 0; attempt < attempts; ++attempt) {
    if (!generateSolvedLayout()) {
      continue;
    }

    uint8_t targetColumn = static_cast<uint8_t>(random(0, 2));
    if (difficulty_ == Difficulty::Easy) {
      targetColumn = 1;
    } else if (difficulty_ == Difficulty::Hard) {
      targetColumn = 0;
    }
    while (pieces_[0].col > static_cast<int8_t>(targetColumn)) {
      movePiece(0, -1, false);
    }

    const auto moveTowardExitRow = [this](uint8_t index) {
      if (index >= pieceCount_ || pieces_[index].horizontal) {
        return;
      }
      Piece& piece = pieces_[index];
      while (!(piece.row <= 2 && piece.row + piece.length - 1 >= 2)) {
        const int8_t direction = piece.row > 2 ? -1 : 1;
        if (!movePiece(index, direction, false)) {
          break;
        }
      }
    };

    if (difficulty_ == Difficulty::Easy) {
      moveTowardExitRow(random(0, 2) == 0 ? 1 : 2);
    } else {
      moveTowardExitRow(1);
      moveTowardExitRow(2);
    }

    shuffle(shuffleSteps());

    for (uint8_t pass = 0; pass < 2 && blockersOnExitPath() < minimumBlockers; ++pass) {
      const uint8_t offset = static_cast<uint8_t>(random(1, pieceCount_));
      for (uint8_t step = 0; step + 1 < pieceCount_ && blockersOnExitPath() < minimumBlockers;
           ++step) {
        const uint8_t index = static_cast<uint8_t>(1 + (offset - 1 + step) % (pieceCount_ - 1));
        moveTowardExitRow(index);
      }
    }

    const uint8_t blockers = blockersOnExitPath();
    if (blockers < minimumBlockers || pieces_[0].col > 1) {
      continue;
    }

    const uint16_t score = puzzleScore();
    const uint8_t moved = movedPieceCount();
    if (score > fallbackScore) {
      fallbackScore = score;
      fallbackCount = pieceCount_;
      std::memcpy(fallbackPieces, pieces_, sizeof(pieces_));
      std::memcpy(fallbackTemplate, templatePieces_, sizeof(templatePieces_));
    }

    if (moved < minimumMoved) {
      continue;
    }

    bool choose = false;
    if (difficulty_ == Difficulty::Easy) {
      choose = bestCount == 0 || score < bestScore || (score == bestScore && random(0, 2) == 0);
    } else if (difficulty_ == Difficulty::Normal) {
      const uint32_t metric = static_cast<uint32_t>(std::abs(static_cast<int32_t>(score) - 390));
      choose = bestCount == 0 || metric < bestMetric ||
               (metric == bestMetric && random(0, 2) == 0);
      if (choose) {
        bestMetric = metric;
      }
    } else {
      choose = bestCount == 0 || score > bestScore || (score == bestScore && random(0, 2) == 0);
    }

    if (choose) {
      bestScore = score;
      bestCount = pieceCount_;
      std::memcpy(bestPieces, pieces_, sizeof(pieces_));
      std::memcpy(bestTemplate, templatePieces_, sizeof(templatePieces_));
    }
  }

  if (bestCount == 0 && fallbackCount > 0) {
    bestCount = fallbackCount;
    std::memcpy(bestPieces, fallbackPieces, sizeof(bestPieces));
    std::memcpy(bestTemplate, fallbackTemplate, sizeof(bestTemplate));
  }

  if (bestCount == 0) {
    static const Piece safeLayout[MAX_PIECES] = {
        {2, 4, 2, true},  {0, 3, 2, false}, {3, 4, 2, false}, {0, 0, 3, true},
        {0, 4, 2, true},  {1, 0, 3, true},  {1, 4, 2, true},  {3, 0, 3, true},
        {3, 5, 3, false}, {4, 0, 3, true},  {5, 0, 3, true},  {3, 3, 2, false},
    };
    pieceCount_ = desiredPieceCount();
    std::memcpy(pieces_, safeLayout, sizeof(pieces_));
    std::memcpy(templatePieces_, safeLayout, sizeof(templatePieces_));
    while (pieces_[0].col > (difficulty_ == Difficulty::Easy ? 1 : 0)) {
      movePiece(0, -1, false);
    }
    movePiece(1, 1, false);
    if (difficulty_ != Difficulty::Easy) {
      movePiece(2, -1, false);
    }
    if (difficulty_ == Difficulty::Hard) {
      movePiece(8, -1, false);
    }
  } else {
    pieceCount_ = bestCount;
    std::memcpy(pieces_, bestPieces, sizeof(pieces_));
    std::memcpy(templatePieces_, bestTemplate, sizeof(templatePieces_));
  }

  moves_ = 0;
  selected_ = 0;
  phase_ = Phase::Playing;
  saveInitialState();
}

void SlidingGame::saveInitialState() {
  std::memcpy(initialPieces_, pieces_, sizeof(pieces_));
}

void SlidingGame::restartPuzzle() {
  std::memcpy(pieces_, initialPieces_, sizeof(pieces_));
  selected_ = 0;
  moves_ = 0;
  phase_ = Phase::Playing;
  drawScreen();
}

bool SlidingGame::isSolved() const {
  return pieces_[0].row == 2 && pieces_[0].col == BOARD_SIZE - pieces_[0].length;
}

void SlidingGame::cycleDifficulty() {
  switch (difficulty_) {
    case Difficulty::Easy:
      difficulty_ = Difficulty::Normal;
      break;
    case Difficulty::Normal:
      difficulty_ = Difficulty::Hard;
      break;
    case Difficulty::Hard:
      difficulty_ = Difficulty::Easy;
      break;
  }
  puzzle_ = 1;
  createPuzzle();
  drawScreen();
}

uint16_t SlidingGame::pieceColor(uint8_t index) const {
  static const uint16_t colors[] = {
      Theme::GOLD, Theme::PURPLE, Theme::PINK, Theme::ORANGE, Theme::LIME,
      0x6D7F, Theme::CYAN, 0xA4BF, 0xF81F, 0x7FE0, 0x9B3F, 0xFD45, 0x5D9F, 0xBDE0,
  };
  return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

void SlidingGame::drawPiece(uint8_t index) {
  const Piece& piece = pieces_[index];
  const int16_t x = GRID_X + piece.col * CELL + 3;
  const int16_t y = GRID_Y + piece.row * CELL + 3;
  const int16_t width = (piece.horizontal ? piece.length * CELL : CELL) - 6;
  const int16_t height = (piece.horizontal ? CELL : piece.length * CELL) - 6;
  const uint16_t color = pieceColor(index);

  tft_.fillRoundRect(x, y, width, height, 7, color);
  if (index == 0) {
    tft_.drawRoundRect(x, y, width, height, 7, Theme::WHITE);
    tft_.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 5, Theme::YELLOW);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::BG, color);
    tft_.drawString("KEY", x + width / 2 - 6, y + height / 2);
    tft_.fillTriangle(x + width - 7, y + height / 2, x + width - 15,
                      y + height / 2 - 6, x + width - 15, y + height / 2 + 6, Theme::BG);
    return;
  }

  const uint16_t outline = index == static_cast<uint8_t>(selected_) ? Theme::WHITE : Theme::BORDER;
  tft_.drawRoundRect(x, y, width, height, 7, outline);
  if (index == static_cast<uint8_t>(selected_)) {
    tft_.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 5, Theme::CYAN);
  }

  const int16_t cx = x + width / 2;
  const int16_t cy = y + height / 2;
  if (piece.horizontal) {
    tft_.drawFastHLine(cx - 8, cy, 16, Theme::BG_2);
    tft_.fillTriangle(cx - 10, cy, cx - 5, cy - 4, cx - 5, cy + 4, Theme::BG_2);
    tft_.fillTriangle(cx + 10, cy, cx + 5, cy - 4, cx + 5, cy + 4, Theme::BG_2);
  } else {
    tft_.drawFastVLine(cx, cy - 8, 16, Theme::BG_2);
    tft_.fillTriangle(cx, cy - 10, cx - 4, cy - 5, cx + 4, cy - 5, Theme::BG_2);
    tft_.fillTriangle(cx, cy + 10, cx - 4, cy + 5, cx + 4, cy + 5, Theme::BG_2);
  }

  if (piece.length == 3) {
    tft_.fillCircle(cx, cy, 2, Theme::WHITE);
  }
}

void SlidingGame::drawExit() {
  const int16_t y = GRID_Y + 2 * CELL;
  tft_.fillRect(GRID_X + BOARD_SIZE * CELL, y + 4, 13, CELL - 8, Theme::CYAN_DARK);
  tft_.drawFastHLine(GRID_X + BOARD_SIZE * CELL, y + 3, 13, Theme::LIME);
  tft_.drawFastHLine(GRID_X + BOARD_SIZE * CELL, y + CELL - 4, 13, Theme::LIME);
  for (uint8_t i = 0; i < 3; ++i) {
    const int16_t x = GRID_X + BOARD_SIZE * CELL + 1 + i * 4;
    tft_.fillTriangle(x, y + CELL / 2 - 6, x, y + CELL / 2 + 6, x + 5,
                      y + CELL / 2, i == 2 ? Theme::WHITE : Theme::LIME);
  }
}

void SlidingGame::drawBoard() {
  tft_.fillRoundRect(GRID_X - 2, GRID_Y - 2, CELL * BOARD_SIZE + 4, CELL * BOARD_SIZE + 4, 8,
                     Theme::PANEL);
  tft_.fillRect(GRID_X, GRID_Y, CELL * BOARD_SIZE, CELL * BOARD_SIZE, Theme::BG_2);
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

void SlidingGame::drawMoveButtons() {
  const bool horizontal = pieces_[selected_].horizontal;
  const bool canNegative = canMove(static_cast<uint8_t>(selected_), -1);
  const bool canPositive = canMove(static_cast<uint8_t>(selected_), 1);

  Ui::drawButton(tft_, MOVE_NEG_BUTTON, horizontal ? "<" : "^",
                 canNegative ? Theme::CYAN_DARK : Theme::PANEL,
                 canNegative ? Theme::WHITE : Theme::MUTED,
                 canNegative ? Theme::CYAN : Theme::BORDER);
  Ui::drawButton(tft_, MOVE_POS_BUTTON, horizontal ? ">" : "v",
                 canPositive ? Theme::CYAN_DARK : Theme::PANEL,
                 canPositive ? Theme::WHITE : Theme::MUTED,
                 canPositive ? Theme::CYAN : Theme::BORDER);
}

void SlidingGame::drawSidePanel() {
  tft_.fillRect(198, 40, 122, 200, Theme::BG);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("PUZZLE", 201, 49);
  tft_.drawString("MOVES", 263, 49);

  char text[28];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(puzzle_));
  tft_.drawString(text, 201, 65);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(moves_));
  tft_.drawString(text, 263, 65);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::LIME, Theme::BG);
  tft_.drawString("EXIT >>>", 240, 84);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("TAP + SWIPE", 219, 96);

  drawMoveButtons();

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
  const int16_t y = GRID_Y + 2 * CELL + CELL / 2;
  for (uint8_t frame = 0; frame < 5; ++frame) {
    const uint16_t color = frame % 2 == 0 ? Theme::LIME : Theme::WHITE;
    tft_.drawCircle(GRID_X + BOARD_SIZE * CELL + 5, y, 5 + frame * 3, color);
    tft_.drawFastHLine(GRID_X + BOARD_SIZE * CELL - 18 + frame * 4, y, 20, color);
    delay(32);
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
  if (x < GRID_X || x >= GRID_X + CELL * BOARD_SIZE || y < GRID_Y ||
      y >= GRID_Y + CELL * BOARD_SIZE) {
    return;
  }
  const uint8_t col = static_cast<uint8_t>((x - GRID_X) / CELL);
  const uint8_t row = static_cast<uint8_t>((y - GRID_Y) / CELL);
  const int8_t found = pieceAt(row, col);
  if (found >= 0) {
    selected_ = found;
    drawBoard();
    drawSidePanel();
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
  const Piece& piece = pieces_[selected_];
  const int16_t primary = piece.horizontal ? input.swipeDx : input.swipeDy;
  const int16_t secondary = piece.horizontal ? input.swipeDy : input.swipeDx;
  if (std::abs(primary) < 18 || std::abs(primary) < std::abs(secondary)) {
    drawBoard();
    drawSidePanel();
    return;
  }

  const int8_t direction = primary > 0 ? 1 : -1;
  const uint8_t cells = static_cast<uint8_t>(std::min<int>(5, std::max<int>(1, std::abs(primary) / CELL)));
  if (movePieceBy(static_cast<uint8_t>(selected_), direction, cells, true) > 0) {
    drawBoard();
    drawSidePanel();
    checkSolved();
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
    if (contains(MOVE_NEG_BUTTON, input.x, input.y)) {
      if (movePiece(static_cast<uint8_t>(selected_), -1, true)) {
        drawBoard();
        drawSidePanel();
        checkSolved();
      }
      return;
    }
    if (contains(MOVE_POS_BUTTON, input.x, input.y)) {
      if (movePiece(static_cast<uint8_t>(selected_), 1, true)) {
        drawBoard();
        drawSidePanel();
        checkSolved();
      }
      return;
    }
    selectAt(input.x, input.y);
  }

  if (input.swipe) {
    moveFromSwipe(input);
  }
}
