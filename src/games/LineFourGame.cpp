#include "games/LineFourGame.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t LineFourGame::ROWS;
constexpr uint8_t LineFourGame::COLS;
constexpr int16_t LineFourGame::BOARD_X;
constexpr int16_t LineFourGame::BOARD_Y;
constexpr int16_t LineFourGame::CELL;
constexpr Rect LineFourGame::SOLO_BUTTON;
constexpr Rect LineFourGame::DUO_BUTTON;
constexpr Rect LineFourGame::NEW_BUTTON;
constexpr Rect LineFourGame::MODE_BUTTON;

LineFourGame::LineFourGame(TFT_eSPI& display) : tft_(display) {}

void LineFourGame::enter() {
  phase_ = Phase::Intro;
  dropping_ = false;
  drawIntro();
}

void LineFourGame::drawDisc(int16_t x, int16_t y, uint8_t player, uint8_t radius) {
  const uint16_t base = player == 1 ? Theme::CYAN : Theme::PINK;
  const uint16_t rim = player == 1 ? Theme::CYAN_DARK : Theme::PURPLE;
  tft_.fillCircle(x, y, radius, rim);
  tft_.fillCircle(x, y, radius - 2, base);
  tft_.drawCircle(x, y, radius - 3, Theme::WHITE);
  tft_.fillCircle(x - radius / 3, y - radius / 3, 2, Theme::WHITE);
}

void LineFourGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "LINE FOUR");
  tft_.fillRoundRect(74, 54, 172, 80, 12, Theme::PANEL);
  tft_.drawRoundRect(74, 54, 172, 80, 12, Theme::PURPLE);
  for (uint8_t r = 0; r < 3; ++r) {
    for (uint8_t c = 0; c < 6; ++c) {
      const int16_t x = 90 + 28 * c;
      const int16_t y = 68 + 24 * r;
      tft_.fillCircle(x, y, 9, Theme::BG_2);
      if ((r == 2 && c < 4) || (r == 1 && c == 3)) {
        drawDisc(x, y, (r == 1 ? 2 : 1), 9);
      }
    }
  }
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Connect four glowing discs", 160, 147);
  Ui::drawButton(tft_, SOLO_BUTTON, "VS CPU", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, DUO_BUTTON, "2 PLAYERS", Theme::PANEL_2, Theme::WHITE, Theme::PURPLE);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap a column to drop your disc", 160, 223);
}

void LineFourGame::newGame(uint32_t now) {
  std::memset(board_, 0, sizeof(board_));
  phase_ = Phase::Playing;
  count_ = 0;
  winner_ = 0;
  turn_ = 1;
  dropping_ = false;
  cpuReadyAt_ = now;
  drawGame();
}

void LineFourGame::drawCell(uint8_t row, uint8_t col) {
  const int16_t x = BOARD_X + col * CELL;
  const int16_t y = BOARD_Y + row * CELL;
  tft_.fillRoundRect(x + 1, y + 1, CELL - 2, CELL - 2, 5, Theme::PANEL_2);
  tft_.fillCircle(x + CELL / 2, y + CELL / 2, 12, Theme::BG);
  tft_.drawCircle(x + CELL / 2, y + CELL / 2, 12, Theme::GRID);
  if (board_[row][col] != 0) {
    drawDisc(x + CELL / 2, y + CELL / 2, board_[row][col]);
  }
}

void LineFourGame::drawColumn(uint8_t col) {
  for (uint8_t row = 0; row < ROWS; ++row) drawCell(row, col);
}

void LineFourGame::drawGame() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "LINE FOUR");
  tft_.fillRoundRect(BOARD_X - 2, BOARD_Y - 2, CELL * COLS + 4, CELL * ROWS + 4,
                     9, Theme::CYAN_DARK);
  for (uint8_t row = 0; row < ROWS; ++row) {
    for (uint8_t col = 0; col < COLS; ++col) drawCell(row, col);
  }
  drawSideFrame();
  drawStatus();
}

void LineFourGame::drawSideFrame() {
  tft_.fillRoundRect(216, 48, 100, 185, 10, Theme::PANEL);
  tft_.drawRoundRect(216, 48, 100, 185, 10, Theme::BORDER);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString(solo_ ? "SOLO / CPU" : "LOCAL / 2P", 266, 61);
  drawDisc(243, 89, 1, 9);
  drawDisc(289, 89, 2, 9);
  tft_.setTextColor(Theme::TEXT, Theme::PANEL);
  tft_.drawString("P1", 243, 105);
  tft_.drawString(solo_ ? "CPU" : "P2", 289, 105);
  Ui::drawButton(tft_, NEW_BUTTON, "RESTART", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, MODE_BUTTON, "MODE", Theme::PANEL_2, Theme::TEXT, Theme::BORDER);
}

void LineFourGame::drawStatus() {
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextPadding(92);
  if (phase_ == Phase::Finished) {
    tft_.setTextColor(winner_ == 3 ? Theme::YELLOW :
                          (winner_ == 1 ? Theme::CYAN : Theme::PINK), Theme::PANEL);
    tft_.drawString(winner_ == 3 ? "DRAW!" : "WINNER!", 266, 129);
  } else {
    tft_.setTextColor(turn_ == 1 ? Theme::CYAN : Theme::PINK, Theme::PANEL);
    tft_.drawString((solo_ && turn_ == 2) ? "CPU TURN" :
                    (turn_ == 1 ? "P1 TURN" : "P2 TURN"), 266, 129);
  }
  tft_.setTextPadding(0);

  tft_.setTextFont(1);
  tft_.setTextPadding(92);
  if (phase_ == Phase::Finished) {
    tft_.setTextColor(Theme::TEXT, Theme::PANEL);
    tft_.drawString(winner_ == 3 ? "NO MORE MOVES" :
                      (winner_ == 1 ? "PLAYER ONE" : (solo_ ? "THE CPU" : "PLAYER TWO")),
                    266, 146);
  } else {
    tft_.setTextColor(Theme::MUTED, Theme::PANEL);
    tft_.drawString("TAP A COLUMN", 266, 146);
  }
  tft_.setTextPadding(0);
}

int8_t LineFourGame::landingRow(uint8_t col) const {
  if (col >= COLS) return -1;
  for (int8_t row = ROWS - 1; row >= 0; --row) {
    if (board_[row][col] == 0) return row;
  }
  return -1;
}

bool LineFourGame::checkWin(uint8_t row, uint8_t col, uint8_t player, bool store) {
  constexpr int8_t directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  for (const auto& dir : directions) {
    uint8_t positionsR[7]{};
    uint8_t positionsC[7]{};
    uint8_t n = 0;
    for (int8_t delta = -3; delta <= 3; ++delta) {
      const int16_t r = static_cast<int16_t>(row) + delta * dir[0];
      const int16_t c = static_cast<int16_t>(col) + delta * dir[1];
      if (r >= 0 && r < ROWS && c >= 0 && c < COLS && board_[r][c] == player) {
        positionsR[n] = r;
        positionsC[n] = c;
        ++n;
      } else {
        n = 0;
      }
      if (n >= 4) {
        if (store) {
          for (uint8_t i = 0; i < 4; ++i) {
            winningRows_[i] = positionsR[n - 4 + i];
            winningCols_[i] = positionsC[n - 4 + i];
          }
        }
        return true;
      }
    }
  }
  return false;
}

bool LineFourGame::wouldWin(uint8_t col, uint8_t player) {
  const int8_t row = landingRow(col);
  if (row < 0) return false;
  board_[row][col] = player;
  const bool result = checkWin(row, col, player, false);
  board_[row][col] = 0;
  return result;
}

uint8_t LineFourGame::chooseCpuColumn() {
  const uint8_t order[COLS] = {3, 2, 4, 1, 5, 0, 6};
  for (uint8_t col : order) if (wouldWin(col, 2)) return col;
  for (uint8_t col : order) if (wouldWin(col, 1)) return col;

  // Look one reply ahead: avoid setting up an instant human win, and
  // favour centre control and moves that create multiple future threats.
  int16_t best = -30000;
  uint8_t chosen = 3;
  for (uint8_t col : order) {
    const int8_t row = landingRow(col);
    if (row < 0) continue;
    board_[row][col] = 2;
    int16_t score = 20 - 4 * abs(static_cast<int>(col) - 3);
    uint8_t ourThreats = 0;
    uint8_t humanThreats = 0;
    for (uint8_t next : order) {
      if (wouldWin(next, 1)) ++humanThreats;
      if (wouldWin(next, 2)) ++ourThreats;
    }
    if (humanThreats > 0) score -= 1500;
    score += ourThreats * 55;
    if (ourThreats >= 2) score += 500;
    board_[row][col] = 0;
    if (score > best) { best = score; chosen = col; }
  }
  return chosen;
}

void LineFourGame::startDrop(uint8_t col, uint32_t now) {
  const int8_t row = landingRow(col);
  if (row < 0 || dropping_ || phase_ != Phase::Playing) return;
  dropCol_ = col;
  dropRow_ = row;
  dropY_ = BOARD_Y + CELL / 2;
  lastDropFrame_ = now;
  dropping_ = true;
  drawDisc(BOARD_X + col * CELL + CELL / 2, dropY_, turn_);
}

void LineFourGame::finishDrop(uint32_t now) {
  dropping_ = false;
  board_[dropRow_][dropCol_] = turn_;
  ++count_;
  drawCell(dropRow_, dropCol_);
  if (checkWin(dropRow_, dropCol_, turn_, true)) {
    winner_ = turn_;
    phase_ = Phase::Finished;
    for (uint8_t i = 0; i < 4; ++i) {
      tft_.drawCircle(BOARD_X + winningCols_[i] * CELL + CELL / 2,
                      BOARD_Y + winningRows_[i] * CELL + CELL / 2, 13, Theme::YELLOW);
    }
  } else if (count_ == ROWS * COLS) {
    winner_ = 3;
    phase_ = Phase::Finished;
  } else {
    turn_ = turn_ == 1 ? 2 : 1;
    if (solo_ && turn_ == 2) cpuReadyAt_ = now + 430;
  }
  drawStatus();
}

void LineFourGame::update(const InputFrame& input, uint32_t now) {
  if (phase_ == Phase::Intro) {
    if (input.pressed && (contains(SOLO_BUTTON, input.x, input.y) ||
                          contains(DUO_BUTTON, input.x, input.y))) {
      solo_ = contains(SOLO_BUTTON, input.x, input.y);
      newGame(now);
    }
    return;
  }
  if (input.pressed && contains(MODE_BUTTON, input.x, input.y)) {
    enter();
    return;
  }
  if (input.pressed && contains(NEW_BUTTON, input.x, input.y)) {
    newGame(now);
    return;
  }
  if (dropping_) {
    if (static_cast<uint32_t>(now - lastDropFrame_) >= 18) {
      lastDropFrame_ = now;
      const int16_t targetY = BOARD_Y + dropRow_ * CELL + CELL / 2;
      dropY_ = (dropY_ + 8 > targetY) ? targetY : dropY_ + 8;
      drawColumn(dropCol_);
      drawDisc(BOARD_X + dropCol_ * CELL + CELL / 2, dropY_, turn_);
      if (dropY_ >= targetY) finishDrop(now);
    }
    return;
  }
  if (phase_ != Phase::Playing) return;
  if (solo_ && turn_ == 2) {
    if (static_cast<int32_t>(now - cpuReadyAt_) >= 0) startDrop(chooseCpuColumn(), now);
    return;
  }
  if (input.pressed && input.x >= BOARD_X && input.x < BOARD_X + CELL * COLS &&
      input.y >= BOARD_Y && input.y < BOARD_Y + CELL * ROWS) {
    startDrop(static_cast<uint8_t>((input.x - BOARD_X) / CELL), now);
  }
}
