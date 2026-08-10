#include "games/WormGame.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "Theme.h"
#include "Ui.h"

constexpr int16_t WormGame::BOARD_X;
constexpr int16_t WormGame::BOARD_Y;
constexpr int16_t WormGame::CELL_SIZE;
constexpr uint8_t WormGame::COLS;
constexpr uint8_t WormGame::ROWS;
constexpr uint16_t WormGame::MAX_BODY;
constexpr uint8_t WormGame::MAX_OBSTACLES;
constexpr Rect WormGame::PAUSE_BUTTON;
constexpr Rect WormGame::START_BUTTON;
constexpr Rect WormGame::AGAIN_BUTTON;

WormGame::WormGame(TFT_eSPI& display) : tft_(display) {}

void WormGame::enter() {
  state_ = State::Intro;
  drawIntro();
}

void WormGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "CIRCUIT WORM");
  Ui::drawSparkles(tft_, 24, 47, 227);
  Ui::drawWormIcon(tft_, 151, 96, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Swipe to steer. Eat the energy.", 160, 133);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Speed rises and obstacles appear", 160, 153);

  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void WormGame::startGame(uint32_t now) {
  state_ = State::Running;
  direction_ = Direction::Right;
  queuedDirection_ = Direction::Right;
  length_ = 5;
  obstacleCount_ = 0;
  eaten_ = 0;
  score_ = 0;
  moveInterval_ = 165;
  bonusActive_ = false;

  const int8_t startX = 9;
  const int8_t startY = 6;
  for (uint16_t i = 0; i < length_; ++i) {
    body_[i] = Cell{static_cast<int8_t>(startX - i), startY};
  }

  spawnFood();
  nextMoveAt_ = now + moveInterval_;
  drawGameFrame();
}

void WormGame::drawPauseButton() {
  const uint16_t fill = state_ == State::Paused ? Theme::CYAN_DARK : Theme::PANEL_2;
  tft_.fillRoundRect(PAUSE_BUTTON.x, PAUSE_BUTTON.y, PAUSE_BUTTON.w, PAUSE_BUTTON.h, 6, fill);
  tft_.drawRoundRect(PAUSE_BUTTON.x, PAUSE_BUTTON.y, PAUSE_BUTTON.w, PAUSE_BUTTON.h, 6, Theme::BORDER);
  if (state_ == State::Paused) {
    tft_.fillTriangle(291, 13, 291, 29, 303, 21, Theme::WHITE);
  } else {
    tft_.fillRect(289, 13, 4, 16, Theme::TEXT);
    tft_.fillRect(299, 13, 4, 16, Theme::TEXT);
  }
}

void WormGame::drawHud() {
  tft_.fillRect(60, 0, 218, 39, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  tft_.drawString("SCORE", 71, 11);
  tft_.drawString("HIGH", 172, 11);

  char text[24];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
  tft_.drawString(text, 71, 25);
  std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(highScore_));
  tft_.drawString(text, 172, 25);
  drawPauseButton();
}

void WormGame::drawCell(const Cell& cell, uint16_t color, bool rounded) {
  const int16_t x = BOARD_X + cell.x * CELL_SIZE + 1;
  const int16_t y = BOARD_Y + cell.y * CELL_SIZE + 1;
  if (rounded) {
    tft_.fillRoundRect(x, y, CELL_SIZE - 2, CELL_SIZE - 2, 3, color);
  } else {
    tft_.fillRect(x, y, CELL_SIZE - 2, CELL_SIZE - 2, color);
  }
}

void WormGame::clearCell(const Cell& cell) {
  drawCell(cell, Theme::BG_2, false);
}

void WormGame::drawFood() {
  const int16_t cx = BOARD_X + food_.x * CELL_SIZE + CELL_SIZE / 2;
  const int16_t cy = BOARD_Y + food_.y * CELL_SIZE + CELL_SIZE / 2;
  tft_.fillCircle(cx, cy, 5, Theme::PINK);
  tft_.drawCircle(cx, cy, 6, Theme::WHITE);
}

void WormGame::drawBonus() {
  if (!bonusActive_) {
    return;
  }
  const int16_t cx = BOARD_X + bonus_.x * CELL_SIZE + CELL_SIZE / 2;
  const int16_t cy = BOARD_Y + bonus_.y * CELL_SIZE + CELL_SIZE / 2;
  tft_.fillCircle(cx, cy, 5, Theme::YELLOW);
  tft_.drawFastHLine(cx - 7, cy, 15, Theme::ORANGE);
  tft_.drawFastVLine(cx, cy - 7, 15, Theme::ORANGE);
}

void WormGame::drawObstacle(const Cell& cell) {
  drawCell(cell, Theme::PURPLE);
  const int16_t x = BOARD_X + cell.x * CELL_SIZE + 4;
  const int16_t y = BOARD_Y + cell.y * CELL_SIZE + 4;
  tft_.drawLine(x, y, x + 6, y + 6, Theme::BG_2);
  tft_.drawLine(x + 6, y, x, y + 6, Theme::BG_2);
}

void WormGame::drawGameFrame() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "CIRCUIT WORM");
  tft_.fillRoundRect(BOARD_X - 2, BOARD_Y - 2, COLS * CELL_SIZE + 4, ROWS * CELL_SIZE + 4, 7,
                     Theme::PANEL);
  tft_.fillRect(BOARD_X, BOARD_Y, COLS * CELL_SIZE, ROWS * CELL_SIZE, Theme::BG_2);
  tft_.drawRoundRect(BOARD_X - 2, BOARD_Y - 2, COLS * CELL_SIZE + 4, ROWS * CELL_SIZE + 4, 7,
                     Theme::BORDER);

  for (uint8_t i = 0; i < obstacleCount_; ++i) {
    drawObstacle(obstacles_[i]);
  }
  for (int16_t i = static_cast<int16_t>(length_) - 1; i >= 0; --i) {
    drawCell(body_[i], i == 0 ? Theme::LIME : Theme::CYAN);
  }
  drawFood();
  drawBonus();
  drawHud();
}

bool WormGame::sameCell(const Cell& a, const Cell& b) const {
  return a.x == b.x && a.y == b.y;
}

bool WormGame::bodyOccupies(const Cell& cell, uint16_t limit) const {
  const uint16_t count = std::min<uint16_t>(limit, length_);
  for (uint16_t i = 0; i < count; ++i) {
    if (sameCell(body_[i], cell)) {
      return true;
    }
  }
  return false;
}

bool WormGame::obstacleOccupies(const Cell& cell) const {
  for (uint8_t i = 0; i < obstacleCount_; ++i) {
    if (sameCell(obstacles_[i], cell)) {
      return true;
    }
  }
  return false;
}

bool WormGame::freeCell(const Cell& cell) const {
  return cell.x >= 0 && cell.x < COLS && cell.y >= 0 && cell.y < ROWS &&
         !bodyOccupies(cell, length_) && !obstacleOccupies(cell) && !sameCell(cell, food_) &&
         (!bonusActive_ || !sameCell(cell, bonus_));
}

void WormGame::spawnFood() {
  for (uint16_t attempt = 0; attempt < 500; ++attempt) {
    const Cell candidate{static_cast<int8_t>(random(0, COLS)), static_cast<int8_t>(random(0, ROWS))};
    if (!bodyOccupies(candidate, length_) && !obstacleOccupies(candidate) &&
        (!bonusActive_ || !sameCell(candidate, bonus_))) {
      food_ = candidate;
      return;
    }
  }
  food_ = Cell{1, 1};
}

void WormGame::spawnBonus(uint32_t now) {
  for (uint16_t attempt = 0; attempt < 200; ++attempt) {
    const Cell candidate{static_cast<int8_t>(random(0, COLS)), static_cast<int8_t>(random(0, ROWS))};
    if (!bodyOccupies(candidate, length_) && !obstacleOccupies(candidate) && !sameCell(candidate, food_)) {
      bonus_ = candidate;
      bonusActive_ = true;
      bonusUntil_ = now + 4500;
      drawBonus();
      return;
    }
  }
}

void WormGame::addObstacle() {
  if (obstacleCount_ >= MAX_OBSTACLES) {
    return;
  }

  for (uint16_t attempt = 0; attempt < 300; ++attempt) {
    const Cell candidate{static_cast<int8_t>(random(1, COLS - 1)),
                         static_cast<int8_t>(random(1, ROWS - 1))};
    const int16_t distance = std::abs(candidate.x - body_[0].x) + std::abs(candidate.y - body_[0].y);
    if (distance >= 5 && freeCell(candidate)) {
      obstacles_[obstacleCount_++] = candidate;
      drawObstacle(candidate);
      return;
    }
  }
}

bool WormGame::isOpposite(Direction a, Direction b) const {
  return (a == Direction::Up && b == Direction::Down) ||
         (a == Direction::Down && b == Direction::Up) ||
         (a == Direction::Left && b == Direction::Right) ||
         (a == Direction::Right && b == Direction::Left);
}

void WormGame::setDirectionFromSwipe(int16_t dx, int16_t dy) {
  Direction requested = queuedDirection_;
  if (std::abs(dx) > std::abs(dy)) {
    requested = dx > 0 ? Direction::Right : Direction::Left;
  } else {
    requested = dy > 0 ? Direction::Down : Direction::Up;
  }

  if (!isOpposite(requested, direction_)) {
    queuedDirection_ = requested;
  }
}

void WormGame::step(uint32_t now) {
  direction_ = queuedDirection_;
  Cell next = body_[0];
  switch (direction_) {
    case Direction::Up:
      --next.y;
      break;
    case Direction::Down:
      ++next.y;
      break;
    case Direction::Left:
      --next.x;
      break;
    case Direction::Right:
      ++next.x;
      break;
  }

  const bool ateFood = sameCell(next, food_);
  const bool ateBonus = bonusActive_ && sameCell(next, bonus_);
  const uint16_t collisionLimit = ateFood ? length_ : (length_ > 0 ? length_ - 1 : 0);
  if (next.x < 0 || next.x >= COLS || next.y < 0 || next.y >= ROWS ||
      bodyOccupies(next, collisionLimit) || obstacleOccupies(next)) {
    endGame();
    return;
  }

  const Cell oldTail = body_[length_ - 1];
  if (ateFood && length_ < MAX_BODY) {
    for (uint16_t i = length_; i > 0; --i) {
      body_[i] = body_[i - 1];
    }
    ++length_;
  } else {
    for (uint16_t i = length_ - 1; i > 0; --i) {
      body_[i] = body_[i - 1];
    }
    clearCell(oldTail);
  }
  body_[0] = next;

  if (length_ > 1) {
    drawCell(body_[1], Theme::CYAN);
  }
  drawCell(body_[0], Theme::LIME);

  if (ateBonus) {
    bonusActive_ = false;
    score_ += 50;
  }

  if (ateFood) {
    ++eaten_;
    score_ += 10 + static_cast<uint32_t>(std::min<uint16_t>(eaten_, 20));
    moveInterval_ = static_cast<uint16_t>(std::max<int>(70, 165 - eaten_ * 5));
    spawnFood();
    drawFood();

    if (eaten_ % 4 == 0) {
      addObstacle();
    }
    if (eaten_ % 5 == 0 && !bonusActive_) {
      spawnBonus(now);
    }
    drawHud();
  } else if (ateBonus) {
    drawHud();
  }

  if (bonusActive_ && static_cast<int32_t>(now - bonusUntil_) >= 0) {
    clearCell(bonus_);
    bonusActive_ = false;
  }

  nextMoveAt_ = now + moveInterval_;
}

void WormGame::showPauseOverlay() {
  tft_.fillRoundRect(76, 93, 168, 78, 10, Theme::BG);
  tft_.drawRoundRect(76, 93, 168, 78, 10, Theme::CYAN);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("PAUSED", 160, 118);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap pause to continue", 160, 150);
  drawHud();
}

void WormGame::hidePauseOverlay() {
  drawGameFrame();
}

void WormGame::endGame() {
  state_ = State::GameOver;
  if (score_ > highScore_) {
    highScore_ = score_;
  }
  drawGameOver();
}

void WormGame::drawGameOver() {
  tft_.fillRoundRect(55, 82, 210, 139, 12, Theme::BG);
  tft_.drawRoundRect(55, 82, 210, 139, 12, Theme::PINK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::PINK, Theme::BG);
  tft_.drawString("SIGNAL LOST", 160, 108);

  char text[40];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "Score %lu   High %lu", static_cast<unsigned long>(score_),
                static_cast<unsigned long>(highScore_));
  tft_.drawString(text, 160, 142);
  Ui::drawButton(tft_, AGAIN_BUTTON, "PLAY AGAIN", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void WormGame::update(const InputFrame& input, uint32_t now) {
  if (state_ == State::Intro) {
    if (input.pressed && contains(START_BUTTON, input.x, input.y)) {
      startGame(now);
    }
    return;
  }

  if (state_ == State::GameOver) {
    if (input.pressed && contains(AGAIN_BUTTON, input.x, input.y)) {
      startGame(now);
    }
    return;
  }

  if (input.pressed && contains(PAUSE_BUTTON, input.x, input.y)) {
    if (state_ == State::Running) {
      state_ = State::Paused;
      showPauseOverlay();
    } else if (state_ == State::Paused) {
      state_ = State::Running;
      nextMoveAt_ = now + moveInterval_;
      hidePauseOverlay();
    }
    return;
  }

  if (state_ == State::Paused) {
    return;
  }

  if (input.swipe) {
    setDirectionFromSwipe(input.swipeDx, input.swipeDy);
  }

  if (bonusActive_ && static_cast<int32_t>(now - bonusUntil_) >= 0) {
    clearCell(bonus_);
    bonusActive_ = false;
  }

  if (static_cast<int32_t>(now - nextMoveAt_) >= 0) {
    step(now);
  }
}
