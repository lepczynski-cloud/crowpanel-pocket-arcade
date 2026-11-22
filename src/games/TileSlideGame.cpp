#include "games/TileSlideGame.h"

#include <Arduino.h>
#include <cstdio>
#include <cstdlib>

#include "Theme.h"
#include "Ui.h"

constexpr int16_t TileSlideGame::BOARD_X;
constexpr int16_t TileSlideGame::BOARD_Y;
constexpr int16_t TileSlideGame::GAP;
constexpr Rect TileSlideGame::EASY_BUTTON;
constexpr Rect TileSlideGame::HARD_BUTTON;
constexpr Rect TileSlideGame::NEW_BUTTON;
constexpr Rect TileSlideGame::MODE_BUTTON;

TileSlideGame::TileSlideGame(TFT_eSPI& display) : tft_(display) {}

int16_t TileSlideGame::tileSize() const { return dimension_ == 3 ? 57 : 42; }
int16_t TileSlideGame::pitch() const { return tileSize() + GAP; }

void TileSlideGame::tilePosition(uint8_t index, int16_t& x, int16_t& y) const {
  x = BOARD_X + (index % dimension_) * pitch();
  y = BOARD_Y + (index / dimension_) * pitch();
}

void TileSlideGame::enter() {
  phase_ = Phase::Intro;
  sliding_ = false;
  drawIntro();
}

void TileSlideGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "TILE SLIDE");
  tft_.fillRoundRect(90, 53, 140, 82, 12, Theme::PANEL);
  for (uint8_t i = 0; i < 6; ++i) {
    const int16_t x = 99 + (i % 3) * 43;
    const int16_t y = 61 + (i / 3) * 35;
    if (i != 4) {
      tft_.fillRoundRect(x, y, 38, 30, 5, Theme::CYAN_DARK);
      tft_.drawRoundRect(x, y, 38, 30, 5, Theme::CYAN);
      char number[4];
      std::snprintf(number, sizeof(number), "%d", i + 1);
      tft_.setTextColor(Theme::TEXT, Theme::CYAN_DARK);
      tft_.setTextDatum(MC_DATUM);
      tft_.setTextFont(2);
      tft_.drawString(number, x + 19, y + 15);
    } else {
      tft_.drawRoundRect(x, y, 38, 30, 5, Theme::GRID);
    }
  }
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Put the numbers back in order", 160, 148);
  Ui::drawButton(tft_, EASY_BUTTON, "3 x 3", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, HARD_BUTTON, "4 x 4", Theme::PANEL_2, Theme::WHITE, Theme::PURPLE);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap a tile next to the empty space", 160, 223);
}

bool TileSlideGame::solved() const {
  const uint8_t total = dimension_ * dimension_;
  for (uint8_t i = 0; i < total - 1; ++i) {
    if (tiles_[i] != i + 1) return false;
  }
  return tiles_[total - 1] == 0;
}

uint16_t TileSlideGame::manhattan() const {
  uint16_t dist = 0;
  const uint8_t total = dimension_ * dimension_;
  for (uint8_t i = 0; i < total; ++i) {
    if (tiles_[i] == 0) continue;
    const uint8_t target = tiles_[i] - 1;
    dist += abs(static_cast<int>(i / dimension_) - static_cast<int>(target / dimension_));
    dist += abs(static_cast<int>(i % dimension_) - static_cast<int>(target % dimension_));
  }
  return dist;
}

void TileSlideGame::scramble() {
  const uint8_t total = dimension_ * dimension_;
  for (uint8_t attempt = 0; attempt < 6; ++attempt) {
    for (uint8_t i = 0; i < total - 1; ++i) tiles_[i] = i + 1;
    tiles_[total - 1] = 0;
    blank_ = total - 1;
    int8_t previous = -1;
    const uint16_t steps = dimension_ == 3 ? 150 : 390;
    for (uint16_t s = 0; s < steps; ++s) {
      uint8_t choices[4];
      uint8_t numChoices = 0;
      const uint8_t r = blank_ / dimension_;
      const uint8_t c = blank_ % dimension_;
      if (r > 0 && blank_ - dimension_ != previous) choices[numChoices++] = blank_ - dimension_;
      if (r + 1 < dimension_ && blank_ + dimension_ != previous) choices[numChoices++] = blank_ + dimension_;
      if (c > 0 && blank_ - 1 != previous) choices[numChoices++] = blank_ - 1;
      if (c + 1 < dimension_ && blank_ + 1 != previous) choices[numChoices++] = blank_ + 1;
      // The board never gets stuck: all moves are reversible, and after the
      // first shuffle step there is always another neighbour available.
      if (numChoices == 0) break;
      const uint8_t chosen = choices[random(numChoices)];
      tiles_[blank_] = tiles_[chosen];
      tiles_[chosen] = 0;
      previous = blank_;
      blank_ = chosen;
    }
    if (!solved() && manhattan() >= (dimension_ == 3 ? 9 : 20)) break;
  }
}

void TileSlideGame::startGame(uint32_t now) {
  moves_ = 0;
  startedAt_ = now;
  lastClock_ = now;
  sliding_ = false;
  phase_ = Phase::Playing;
  scramble();
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "TILE SLIDE");
  drawBoard();
  drawSideFrame();
  drawSide(now);
}

void TileSlideGame::drawNumber(int16_t x, int16_t y, uint8_t value) {
  const int16_t size = tileSize();
  const bool inPlace = value == static_cast<uint8_t>(((y - BOARD_Y) / pitch()) * dimension_ +
                                                    ((x - BOARD_X) / pitch()) + 1);
  const uint16_t fill = inPlace ? Theme::CYAN_DARK : Theme::PANEL_2;
  const uint16_t edge = inPlace ? Theme::LIME : Theme::PURPLE;
  tft_.fillRoundRect(x, y, size, size, 7, fill);
  tft_.drawRoundRect(x, y, size, size, 7, edge);
  tft_.drawFastHLine(x + 9, y + 5, size - 18, inPlace ? Theme::CYAN : Theme::BORDER);
  char text[4];
  std::snprintf(text, sizeof(text), "%u", value);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(dimension_ == 3 ? 4 : 2);
  tft_.setTextColor(Theme::WHITE, fill);
  tft_.drawString(text, x + size / 2, y + size / 2 + 1);
  tft_.fillCircle(x + size - 7, y + size - 7, 2, inPlace ? Theme::LIME : Theme::CYAN);
}

void TileSlideGame::drawCell(uint8_t index) {
  int16_t x, y;
  tilePosition(index, x, y);
  const int16_t size = tileSize();
  tft_.fillRoundRect(x, y, size, size, 7, Theme::BG_2);
  if (tiles_[index]) drawNumber(x, y, tiles_[index]);
  else tft_.drawRoundRect(x + 3, y + 3, size - 6, size - 6, 6, Theme::GRID);
}

void TileSlideGame::drawBoard() {
  const int16_t total = dimension_ * tileSize() + (dimension_ - 1) * GAP;
  tft_.fillRoundRect(BOARD_X - 4, BOARD_Y - 4, total + 8, total + 8, 10, Theme::PANEL);
  tft_.drawRoundRect(BOARD_X - 4, BOARD_Y - 4, total + 8, total + 8, 10, Theme::CYAN_DARK);
  for (uint8_t i = 0; i < dimension_ * dimension_; ++i) drawCell(i);
}

void TileSlideGame::drawSideFrame() {
  tft_.fillRoundRect(196, 48, 120, 185, 10, Theme::PANEL);
  tft_.drawRoundRect(196, 48, 120, 185, 10, Theme::BORDER);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString(dimension_ == 3 ? "3 x 3 / CLASSIC" : "4 x 4 / EXPERT", 256, 63);
  Ui::drawButton(tft_, NEW_BUTTON, "RESHUFFLE", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, MODE_BUTTON, "SIZE", Theme::PANEL_2, Theme::TEXT, Theme::BORDER);
}

void TileSlideGame::drawElapsed(uint32_t now) {
  const uint32_t elapsed = (now - startedAt_) / 1000;
  char text[16];
  std::snprintf(text, sizeof(text), "%02lu:%02lu", static_cast<unsigned long>(elapsed / 60),
                static_cast<unsigned long>(elapsed % 60));
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextPadding(108);
  tft_.setTextColor(Theme::TEXT, Theme::PANEL);
  tft_.drawString(text, 256, 121);
  tft_.setTextPadding(0);
}

void TileSlideGame::drawSide(uint32_t now) {
  char text[24];
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextPadding(108);
  tft_.setTextColor(Theme::CYAN, Theme::PANEL);
  std::snprintf(text, sizeof(text), "%u MOVES", moves_);
  tft_.drawString(text, 256, 91);
  tft_.setTextPadding(0);
  drawElapsed(now);

  tft_.setTextFont(1);
  tft_.setTextPadding(108);
  tft_.setTextColor(phase_ == Phase::Finished ? Theme::LIME : Theme::MUTED, Theme::PANEL);
  tft_.drawString(phase_ == Phase::Finished ? "SOLVED!" : "TAP TO MOVE", 256, 147);
  tft_.setTextPadding(0);
}

int8_t TileSlideGame::tileAt(uint16_t x, uint16_t y) const {
  if (x < BOARD_X || y < BOARD_Y) return -1;
  const uint16_t bx = x - BOARD_X;
  const uint16_t by = y - BOARD_Y;
  const uint8_t col = bx / pitch();
  const uint8_t row = by / pitch();
  if (col >= dimension_ || row >= dimension_ ||
      bx % pitch() >= tileSize() || by % pitch() >= tileSize()) return -1;
  return row * dimension_ + col;
}

void TileSlideGame::beginSlide(uint8_t index, uint32_t now) {
  if (tiles_[index] == 0 || sliding_ || phase_ != Phase::Playing) return;
  const uint8_t sr = index / dimension_;
  const uint8_t sc = index % dimension_;
  const uint8_t br = blank_ / dimension_;
  const uint8_t bc = blank_ % dimension_;
  if (abs(static_cast<int>(sr) - static_cast<int>(br)) +
      abs(static_cast<int>(sc) - static_cast<int>(bc)) != 1) return;
  from_ = index;
  to_ = blank_;
  moving_ = tiles_[from_];
  slideStep_ = 0;
  lastSlideFrame_ = now;
  sliding_ = true;
}

void TileSlideGame::animateSlide(uint32_t now) {
  if (!sliding_ || static_cast<uint32_t>(now - lastSlideFrame_) < 22) return;
  lastSlideFrame_ = now;
  ++slideStep_;
  int16_t sx, sy, dx, dy;
  tilePosition(from_, sx, sy);
  tilePosition(to_, dx, dy);
  const int16_t minX = sx < dx ? sx : dx;
  const int16_t minY = sy < dy ? sy : dy;
  const int16_t maxX = sx > dx ? sx : dx;
  const int16_t maxY = sy > dy ? sy : dy;
  const int16_t size = tileSize();
  tft_.fillRect(minX, minY, maxX - minX + size, maxY - minY + size, Theme::PANEL);
  const uint8_t frame = slideStep_ > 7 ? 7 : slideStep_;
  drawNumber(sx + (dx - sx) * frame / 7, sy + (dy - sy) * frame / 7, moving_);
  if (frame == 7) {
    tiles_[to_] = moving_;
    tiles_[from_] = 0;
    blank_ = from_;
    sliding_ = false;
    ++moves_;
    drawCell(to_);
    drawCell(from_);
    if (solved()) phase_ = Phase::Finished;
    drawSide(now);
  }
}

void TileSlideGame::update(const InputFrame& input, uint32_t now) {
  if (phase_ == Phase::Intro) {
    if (input.pressed && (contains(EASY_BUTTON, input.x, input.y) ||
                          contains(HARD_BUTTON, input.x, input.y))) {
      dimension_ = contains(EASY_BUTTON, input.x, input.y) ? 3 : 4;
      startGame(now);
    }
    return;
  }
  if (input.pressed && contains(MODE_BUTTON, input.x, input.y)) {
    enter();
    return;
  }
  if (input.pressed && contains(NEW_BUTTON, input.x, input.y)) {
    startGame(now);
    return;
  }
  if (sliding_) {
    animateSlide(now);
    return;
  }
  if (phase_ != Phase::Playing) return;
  if (now - lastClock_ >= 1000) {
    lastClock_ = now;
    drawElapsed(now);
  }
  if (!input.released || input.swipe) return;
  const int8_t index = tileAt(input.x, input.y);
  if (index >= 0) beginSlide(index, now);
}
