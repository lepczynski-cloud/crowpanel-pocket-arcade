#include "games/PairQuestGame.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t PairQuestGame::CARDS;
constexpr int16_t PairQuestGame::BOARD_X;
constexpr int16_t PairQuestGame::BOARD_Y;
constexpr int16_t PairQuestGame::CARD_W;
constexpr int16_t PairQuestGame::CARD_H;
constexpr int16_t PairQuestGame::GAP;
constexpr Rect PairQuestGame::SOLO_BUTTON;
constexpr Rect PairQuestGame::DUO_BUTTON;
constexpr Rect PairQuestGame::NEW_BUTTON;
constexpr Rect PairQuestGame::MODE_BUTTON;

PairQuestGame::PairQuestGame(TFT_eSPI& display) : tft_(display) {}

void PairQuestGame::enter() {
  phase_ = Phase::Intro;
  flipIndex_ = -1;
  drawIntro();
}

void PairQuestGame::drawSymbol(uint8_t kind, int16_t cx, int16_t cy) {
  switch (kind) {
    case 0: // Sun
      tft_.fillCircle(cx, cy, 7, Theme::GOLD);
      tft_.drawCircle(cx, cy, 10, Theme::YELLOW);
      for (uint8_t i = 0; i < 4; ++i) {
        if (i == 0) tft_.drawFastHLine(cx - 15, cy, 4, Theme::YELLOW);
        if (i == 1) tft_.drawFastHLine(cx + 11, cy, 4, Theme::YELLOW);
        if (i == 2) tft_.drawFastVLine(cx, cy - 15, 4, Theme::YELLOW);
        if (i == 3) tft_.drawFastVLine(cx, cy + 11, 4, Theme::YELLOW);
      }
      break;
    case 1: // Crescent
      tft_.fillCircle(cx, cy, 12, Theme::PURPLE);
      tft_.fillCircle(cx + 6, cy - 5, 11, Theme::BG_2);
      break;
    case 2: // Diamond
      tft_.fillTriangle(cx, cy - 14, cx - 13, cy, cx + 13, cy, Theme::CYAN);
      tft_.fillTriangle(cx, cy + 13, cx - 13, cy, cx + 13, cy, Theme::CYAN_DARK);
      tft_.drawLine(cx, cy - 12, cx, cy + 11, Theme::WHITE);
      break;
    case 3: // Planet
      tft_.drawLine(cx - 16, cy + 2, cx + 16, cy - 2, Theme::YELLOW);
      tft_.drawLine(cx - 13, cy + 5, cx + 13, cy + 1, Theme::YELLOW);
      tft_.fillCircle(cx, cy, 10, Theme::PINK);
      tft_.drawLine(cx - 15, cy + 4, cx - 10, cy + 7, Theme::YELLOW);
      break;
    case 4: // Lightning
      tft_.fillTriangle(cx + 3, cy - 14, cx - 9, cy + 2, cx + 2, cy + 2, Theme::YELLOW);
      tft_.fillTriangle(cx - 3, cy + 14, cx + 9, cy - 2, cx - 2, cy - 2, Theme::ORANGE);
      break;
    case 5: // Flower
      tft_.fillCircle(cx - 7, cy - 5, 7, Theme::PINK);
      tft_.fillCircle(cx + 7, cy - 5, 7, Theme::PINK);
      tft_.fillCircle(cx - 7, cy + 6, 7, Theme::PINK);
      tft_.fillCircle(cx + 7, cy + 6, 7, Theme::PINK);
      tft_.fillCircle(cx, cy, 6, Theme::YELLOW);
      break;
    case 6: // Wave
      for (uint8_t n = 0; n < 3; ++n) {
        const int16_t y = cy - 10 + n * 10;
        tft_.drawLine(cx - 14, y + 4, cx - 7, y - 2, Theme::CYAN);
        tft_.drawLine(cx - 7, y - 2, cx, y + 4, Theme::CYAN);
        tft_.drawLine(cx, y + 4, cx + 7, y - 2, Theme::CYAN);
        tft_.drawLine(cx + 7, y - 2, cx + 14, y + 4, Theme::CYAN);
      }
      break;
    default: // Four-leaf emblem
      tft_.fillCircle(cx - 5, cy - 5, 7, Theme::LIME);
      tft_.fillCircle(cx + 5, cy - 5, 7, Theme::LIME);
      tft_.fillCircle(cx - 5, cy + 5, 7, Theme::LIME);
      tft_.fillCircle(cx + 5, cy + 5, 7, Theme::LIME);
      tft_.fillCircle(cx, cy, 3, Theme::WHITE);
      break;
  }
}

void PairQuestGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "PAIR QUEST");
  tft_.fillRoundRect(83, 57, 49, 54, 9, Theme::PANEL_2);
  tft_.drawRoundRect(83, 57, 49, 54, 9, Theme::PURPLE);
  tft_.fillRoundRect(137, 57, 49, 54, 9, Theme::CYAN_DARK);
  drawSymbol(2, 161, 84);
  tft_.fillRoundRect(191, 57, 49, 54, 9, Theme::PANEL_2);
  tft_.drawRoundRect(191, 57, 49, 54, 9, Theme::PINK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Find all eight matching pairs", 160, 128);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Solo challenge or local head-to-head", 160, 147);
  Ui::drawButton(tft_, SOLO_BUTTON, "SOLO", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, DUO_BUTTON, "2 PLAYERS", Theme::PANEL_2, Theme::WHITE, Theme::PURPLE);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Match to keep your turn", 160, 223);
}

void PairQuestGame::startGame(uint32_t now) {
  for (uint8_t i = 0; i < CARDS; ++i) {
    values_[i] = i % 8;
  }
  for (int8_t i = CARDS - 1; i > 0; --i) {
    const uint8_t j = random(i + 1);
    const uint8_t temp = values_[i];
    values_[i] = values_[j];
    values_[j] = temp;
  }
  std::memset(shown_, 0, sizeof(shown_));
  std::memset(matched_, 0, sizeof(matched_));
  points_[0] = points_[1] = 0;
  turn_ = 1;
  tries_ = 0;
  matches_ = 0;
  first_ = second_ = flipIndex_ = -1;
  hideAt_ = 0;
  startedAt_ = now;
  lastClock_ = now;
  phase_ = Phase::Playing;
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "PAIR QUEST");
  drawBoard();
  drawSide(now);
}

void PairQuestGame::drawCard(uint8_t index, uint8_t widthOverride, bool faceOverride) {
  const uint8_t row = index / 4;
  const uint8_t col = index % 4;
  const int16_t left = BOARD_X + col * (CARD_W + GAP);
  const int16_t top = BOARD_Y + row * (CARD_H + GAP);
  const int16_t width = widthOverride;
  const int16_t x = left + (CARD_W - width) / 2;
  tft_.fillRect(left, top, CARD_W, CARD_H, Theme::BG);
  if (width < 4) return;
  const bool open = shown_[index] || matched_[index] || faceOverride;
  const uint16_t fill = !open ? Theme::PANEL_2 : Theme::BG_2;
  tft_.fillRoundRect(x, top, width, CARD_H, 6, fill);
  tft_.drawRoundRect(x, top, width, CARD_H, 6,
                     matched_[index] ? Theme::LIME : (!open ? Theme::PURPLE : Theme::CYAN_DARK));
  if (width < 34) return;
  const int16_t cx = left + CARD_W / 2;
  const int16_t cy = top + CARD_H / 2;
  if (open) {
    drawSymbol(values_[index], cx, cy);
    if (matched_[index]) tft_.fillCircle(left + CARD_W - 7, top + 7, 3, Theme::LIME);
  } else {
    tft_.drawRoundRect(cx - 9, cy - 9, 18, 18, 4, Theme::CYAN_DARK);
    tft_.fillCircle(cx, cy, 4, Theme::PURPLE);
    tft_.drawLine(cx - 4, cy - 13, cx + 4, cy - 13, Theme::PINK);
  }
}

void PairQuestGame::drawBoard() {
  for (uint8_t i = 0; i < CARDS; ++i) drawCard(i);
}

void PairQuestGame::drawSide(uint32_t now) {
  tft_.fillRoundRect(226, 48, 90, 185, 10, Theme::PANEL);
  tft_.drawRoundRect(226, 48, 90, 185, 10, Theme::BORDER);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString(solo_ ? "SOLO" : "LOCAL 2P", 271, 59);
  char score[24];
  tft_.setTextColor(Theme::CYAN, Theme::PANEL);
  if (solo_) {
    std::snprintf(score, sizeof(score), "%u / 8 PAIRS", matches_);
    tft_.drawString(score, 271, 81);
    tft_.setTextColor(Theme::WHITE, Theme::PANEL);
    std::snprintf(score, sizeof(score), "%u MOVES", tries_);
    tft_.drawString(score, 271, 102);
    const uint32_t elapsed = (now - startedAt_) / 1000;
    std::snprintf(score, sizeof(score), "%02lu:%02lu", static_cast<unsigned long>(elapsed / 60),
                  static_cast<unsigned long>(elapsed % 60));
    tft_.drawString(score, 271, 124);
  } else {
    std::snprintf(score, sizeof(score), "P1: %u", points_[0]);
    tft_.drawString(score, 271, 81);
    tft_.setTextColor(Theme::PINK, Theme::PANEL);
    std::snprintf(score, sizeof(score), "P2: %u", points_[1]);
    tft_.drawString(score, 271, 102);
    tft_.setTextColor(turn_ == 1 ? Theme::CYAN : Theme::PINK, Theme::PANEL);
    tft_.drawString(turn_ == 1 ? "P1 TURN" : "P2 TURN", 271, 124);
  }
  tft_.setTextColor(phase_ == Phase::Finished ? Theme::LIME : Theme::MUTED, Theme::PANEL);
  if (phase_ == Phase::Finished) {
    if (solo_) tft_.drawString("ALL MATCHED!", 271, 146);
    else tft_.drawString(points_[0] == points_[1] ? "DRAW!" :
                         (points_[0] > points_[1] ? "P1 WINS!" : "P2 WINS!"), 271, 146);
  } else {
    tft_.drawString("FLIP 2 CARDS", 271, 146);
  }
  Ui::drawButton(tft_, NEW_BUTTON, "NEW", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
  Ui::drawButton(tft_, MODE_BUTTON, "MODE", Theme::PANEL_2, Theme::TEXT, Theme::BORDER);
}

int8_t PairQuestGame::at(uint16_t x, uint16_t y) const {
  if (x < BOARD_X || y < BOARD_Y) return -1;
  const uint16_t bx = x - BOARD_X;
  const uint16_t by = y - BOARD_Y;
  const uint8_t col = bx / (CARD_W + GAP);
  const uint8_t row = by / (CARD_H + GAP);
  if (col >= 4 || row >= 4 || bx % (CARD_W + GAP) >= CARD_W ||
      by % (CARD_H + GAP) >= CARD_H) return -1;
  return row * 4 + col;
}

void PairQuestGame::beginFlip(uint8_t index, uint32_t now) {
  flipIndex_ = index;
  flipFrame_ = 0;
  flipAt_ = now;
}

void PairQuestGame::finishFlip(uint32_t now) {
  const int8_t index = flipIndex_;
  flipIndex_ = -1;
  shown_[index] = true;
  drawCard(index);
  if (first_ < 0) {
    first_ = index;
  } else {
    second_ = index;
    ++tries_;
    if (values_[first_] == values_[second_]) {
      matched_[first_] = true;
      matched_[second_] = true;
      ++matches_;
      ++points_[turn_ - 1];
      drawCard(first_);
      drawCard(second_);
      first_ = second_ = -1;
      if (matches_ == 8) phase_ = Phase::Finished;
      drawSide(now);
    } else {
      hideAt_ = now + 850;
      drawSide(now);
    }
  }
}

void PairQuestGame::update(const InputFrame& input, uint32_t now) {
  if (phase_ == Phase::Intro) {
    if (input.pressed && (contains(SOLO_BUTTON, input.x, input.y) ||
                          contains(DUO_BUTTON, input.x, input.y))) {
      solo_ = contains(SOLO_BUTTON, input.x, input.y);
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
  if (phase_ == Phase::Finished) return;

  if (flipIndex_ >= 0) {
    if (static_cast<uint32_t>(now - flipAt_) >= 25) {
      flipAt_ = now;
      ++flipFrame_;
      const uint8_t widths[] = {38, 25, 8, 8, 25, 38, CARD_W};
      const bool front = flipFrame_ >= 4;
      if (flipFrame_ <= 6) drawCard(flipIndex_, widths[flipFrame_ - 1], front);
      if (flipFrame_ >= 6) finishFlip(now);
    }
    return;
  }
  if (hideAt_ != 0) {
    if (static_cast<int32_t>(now - hideAt_) >= 0) {
      shown_[first_] = false;
      shown_[second_] = false;
      drawCard(first_);
      drawCard(second_);
      first_ = second_ = -1;
      hideAt_ = 0;
      if (!solo_) turn_ = turn_ == 1 ? 2 : 1;
      drawSide(now);
    }
    return;
  }
  if (solo_ && now - lastClock_ >= 1000) {
    lastClock_ = now;
    drawSide(now);
  }
  if (!input.pressed) return;
  const int8_t index = at(input.x, input.y);
  if (index < 0 || shown_[index] || matched_[index]) return;
  beginFlip(index, now);
}
