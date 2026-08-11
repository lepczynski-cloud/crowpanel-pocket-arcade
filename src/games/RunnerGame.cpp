#include "games/RunnerGame.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Theme.h"
#include "Ui.h"

constexpr int16_t RunnerGame::PLAYER_X;
constexpr int16_t RunnerGame::PLAYER_W;
constexpr int16_t RunnerGame::PLAYER_H;
constexpr int16_t RunnerGame::PLAY_TOP;
constexpr int16_t RunnerGame::DYNAMIC_TOP;
constexpr int16_t RunnerGame::GROUND_Y;
constexpr uint8_t RunnerGame::MAX_OBSTACLES;
constexpr uint8_t RunnerGame::MAX_COINS;
constexpr Rect RunnerGame::CHILL_BUTTON;
constexpr Rect RunnerGame::ARCADE_BUTTON;
constexpr Rect RunnerGame::TURBO_BUTTON;
constexpr Rect RunnerGame::START_BUTTON;
constexpr Rect RunnerGame::PAUSE_BUTTON;
constexpr Rect RunnerGame::AGAIN_BUTTON;

RunnerGame::RunnerGame(TFT_eSPI& display) : tft_(display) {}

void RunnerGame::enter() {
  state_ = State::Intro;
  drawIntro();
}

const char* RunnerGame::difficultyLabel() const {
  switch (difficulty_) {
    case Difficulty::Chill:
      return "CHILL";
    case Difficulty::Turbo:
      return "TURBO";
    case Difficulty::Arcade:
    default:
      return "ARCADE";
  }
}

void RunnerGame::drawDifficultyButtons() {
  const Rect buttons[] = {CHILL_BUTTON, ARCADE_BUTTON, TURBO_BUTTON};
  const char* labels[] = {"CHILL", "ARCADE", "TURBO"};
  const Difficulty values[] = {Difficulty::Chill, Difficulty::Arcade, Difficulty::Turbo};

  for (uint8_t i = 0; i < 3; ++i) {
    const bool selected = difficulty_ == values[i];
    Ui::drawButton(tft_, buttons[i], labels[i], selected ? Theme::CYAN_DARK : Theme::PANEL,
                   selected ? Theme::WHITE : Theme::MUTED,
                   selected ? Theme::CYAN : Theme::BORDER);
  }
}

void RunnerGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "STAR POD SPRINT");
  Ui::drawRunnerIcon(tft_, 160, 86, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Tap to jump over spikes and barriers", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Collect gold coins and keep running", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void RunnerGame::configureDifficulty() {
  switch (difficulty_) {
    case Difficulty::Chill:
      speed_ = 78.0f;
      break;
    case Difficulty::Turbo:
      speed_ = 112.0f;
      break;
    case Difficulty::Arcade:
    default:
      speed_ = 94.0f;
      break;
  }
}

void RunnerGame::startGame(uint32_t now) {
  configureDifficulty();
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    obstacles_[i].active = false;
    obstacles_[i].drawn = false;
  }
  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    coins_[i].active = false;
    coins_[i].drawn = false;
  }

  state_ = State::Running;
  playerY_ = static_cast<float>(GROUND_Y - PLAYER_H);
  velocityY_ = 0.0f;
  distance_ = 0.0f;
  score_ = 0;
  coinCount_ = 0;
  dodged_ = 0;
  level_ = 1;
  onGround_ = true;
  renderedPlayerValid_ = false;
  nextObstacleAt_ = now + 1200;
  lastUpdateAt_ = now;
  lastRenderAt_ = 0;
  renderedScore_ = 0xFFFFFFFFUL;
  renderedCoins_ = 0xFFFF;
  renderedLevel_ = 0xFF;
  drawGameFrame();
}

void RunnerGame::jump() {
  if (!onGround_) {
    return;
  }
  velocityY_ = -420.0f;
  onGround_ = false;
}

void RunnerGame::spawnCoin(float x, int16_t y) {
  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    if (!coins_[i].active) {
      coins_[i].active = true;
      coins_[i].x = x;
      coins_[i].y = y;
      coins_[i].drawn = false;
      return;
    }
  }
}

void RunnerGame::spawnObstacle(uint32_t now) {
  int8_t slot = -1;
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    if (!obstacles_[i].active) {
      slot = static_cast<int8_t>(i);
      break;
    }
  }

  if (slot >= 0) {
    Obstacle& obstacle = obstacles_[slot];
    obstacle.active = true;
    obstacle.drawn = false;
    obstacle.x = 326.0f;
    obstacle.passed = false;

    const long roll = random(0, 100);
    if (level_ >= 4 && roll < 18) {
      obstacle.type = 2;
      obstacle.width = 48;
      obstacle.height = 18;
    } else if (level_ >= 2 && roll < 48) {
      obstacle.type = 1;
      obstacle.width = 34;
      obstacle.height = 20;
    } else if (roll < 82) {
      obstacle.type = 0;
      obstacle.width = 18;
      obstacle.height = 23;
    } else {
      obstacle.type = 3;
      obstacle.width = 23;
      obstacle.height = 29;
    }

    if (random(0, 100) < 68) {
      const int16_t coinY = static_cast<int16_t>(
          std::max<int>(108, GROUND_Y - obstacle.height - random(34, 57)));
      spawnCoin(obstacle.x + obstacle.width / 2.0f + 5.0f, coinY);
      if (level_ >= 4 && random(0, 100) < 28) {
        spawnCoin(obstacle.x + obstacle.width + 34.0f,
                  static_cast<int16_t>(std::max<int>(106, coinY - 8)));
      }
    }
  }

  uint16_t baseGap = 1120;
  if (difficulty_ == Difficulty::Chill) {
    baseGap = 1360;
  } else if (difficulty_ == Difficulty::Turbo) {
    baseGap = 930;
  }
  const uint16_t reduction = static_cast<uint16_t>(std::min<int>(250, level_ * 22));
  nextObstacleAt_ = now + baseGap - reduction + static_cast<uint32_t>(random(0, 360));
}

void RunnerGame::updatePhysics(float dt) {
  velocityY_ += 960.0f * dt;
  playerY_ += velocityY_ * dt;

  const float groundTop = static_cast<float>(GROUND_Y - PLAYER_H);
  if (playerY_ >= groundTop) {
    playerY_ = groundTop;
    velocityY_ = 0.0f;
    onGround_ = true;
  }
  if (playerY_ < 88.0f) {
    playerY_ = 88.0f;
    velocityY_ = 30.0f;
  }
}

bool RunnerGame::intersectsPlayer(float x, int16_t y, int16_t width, int16_t height) const {
  const float playerLeft = static_cast<float>(PLAYER_X + 5);
  const float playerRight = static_cast<float>(PLAYER_X + PLAYER_W - 4);
  const float playerTop = playerY_ + 4.0f;
  const float playerBottom = playerY_ + PLAYER_H - 3.0f;

  return playerRight > x && playerLeft < x + width &&
         playerBottom > y && playerTop < y + height;
}

void RunnerGame::handleCollisions() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    Obstacle& obstacle = obstacles_[i];
    if (!obstacle.active) {
      continue;
    }
    const int16_t obstacleY = GROUND_Y - obstacle.height;
    if (intersectsPlayer(obstacle.x, obstacleY, obstacle.width, obstacle.height)) {
      endGame();
      return;
    }
  }

  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    Coin& coin = coins_[i];
    if (!coin.active) {
      continue;
    }
    if (intersectsPlayer(coin.x - 7.0f, coin.y - 7, 14, 14)) {
      coin.active = false;
      ++coinCount_;
    }
  }
}

void RunnerGame::updateObjects(float dt, uint32_t now) {
  float baseSpeed = 94.0f;
  if (difficulty_ == Difficulty::Chill) {
    baseSpeed = 78.0f;
  } else if (difficulty_ == Difficulty::Turbo) {
    baseSpeed = 112.0f;
  }

  distance_ += speed_ * dt;
  level_ = static_cast<uint8_t>(1 + std::min<int>(8, static_cast<int>(distance_ / 820.0f)));
  speed_ = baseSpeed + std::min<float>(78.0f, distance_ / 68.0f);

  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    Obstacle& obstacle = obstacles_[i];
    if (!obstacle.active) {
      continue;
    }
    obstacle.x -= speed_ * dt;
    if (!obstacle.passed && obstacle.x + obstacle.width < PLAYER_X) {
      obstacle.passed = true;
      ++dodged_;
    }
    if (obstacle.x + obstacle.width < -5.0f) {
      obstacle.active = false;
    }
  }

  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    Coin& coin = coins_[i];
    if (!coin.active) {
      continue;
    }
    coin.x -= speed_ * dt;
    if (coin.x < -12.0f) {
      coin.active = false;
    }
  }

  score_ = static_cast<uint32_t>(distance_ / 8.0f) +
           static_cast<uint32_t>(coinCount_) * 25U +
           static_cast<uint32_t>(dodged_) * 15U;

  if (static_cast<int32_t>(now - nextObstacleAt_) >= 0) {
    spawnObstacle(now);
  }
}

void RunnerGame::drawStaticPlayfield() {
  tft_.fillRect(0, PLAY_TOP, 320, 240 - PLAY_TOP, Theme::SKY_DARK);
  tft_.fillRect(0, GROUND_Y, 320, 240 - GROUND_Y, Theme::PANEL);
  tft_.drawFastHLine(0, GROUND_Y, 320, Theme::LIME);
  tft_.drawFastHLine(0, GROUND_Y + 2, 320, Theme::CYAN_DARK);

  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::SKY_DARK);
  tft_.drawString("TAP = JUMP", 8, 77);

  tft_.fillTriangle(147, 82, 152, 72, 157, 82, Theme::PINK);
  tft_.setTextColor(Theme::PINK, Theme::SKY_DARK);
  tft_.drawString("AVOID", 163, 77);

  tft_.fillCircle(255, 77, 6, Theme::YELLOW);
  tft_.drawCircle(255, 77, 7, Theme::GOLD);
  tft_.setTextColor(Theme::YELLOW, Theme::SKY_DARK);
  tft_.drawString("COLLECT", 266, 77);
}

void RunnerGame::drawPauseButton() {
  const uint16_t fill = state_ == State::Paused ? Theme::CYAN_DARK : Theme::PANEL_2;
  tft_.fillRoundRect(PAUSE_BUTTON.x, PAUSE_BUTTON.y, PAUSE_BUTTON.w, PAUSE_BUTTON.h, 6, fill);
  tft_.drawRoundRect(PAUSE_BUTTON.x, PAUSE_BUTTON.y, PAUSE_BUTTON.w, PAUSE_BUTTON.h, 6,
                     Theme::BORDER);
  if (state_ == State::Paused) {
    tft_.fillTriangle(291, 13, 291, 29, 303, 21, Theme::WHITE);
  } else {
    tft_.fillRect(289, 13, 4, 16, Theme::TEXT);
    tft_.fillRect(299, 13, 4, 16, Theme::TEXT);
  }
}

void RunnerGame::drawHudFrame() {
  tft_.fillRect(60, 0, 218, 38, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  tft_.drawString("SCORE", 68, 10);
  tft_.drawString("COINS", 151, 10);
  tft_.drawString("LEVEL", 222, 10);
  updateHud(true);
  drawPauseButton();
}

void RunnerGame::updateHud(bool force) {
  char text[24];

  if (force || renderedScore_ != score_) {
    tft_.fillRect(68, 18, 73, 18, Theme::BG_2);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::TEXT, Theme::BG_2);
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
    tft_.drawString(text, 68, 27);
    renderedScore_ = score_;
  }

  if (force || renderedCoins_ != coinCount_) {
    tft_.fillRect(151, 18, 58, 18, Theme::BG_2);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::YELLOW, Theme::BG_2);
    std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(coinCount_));
    tft_.drawString(text, 151, 27);
    renderedCoins_ = coinCount_;
  }

  if (force || renderedLevel_ != level_) {
    tft_.fillRect(222, 18, 50, 18, Theme::BG_2);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::CYAN, Theme::BG_2);
    std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(level_));
    tft_.drawString(text, 222, 27);
    renderedLevel_ = level_;
  }
}

void RunnerGame::drawPlayerAt(int16_t y) {
  const int16_t cx = PLAYER_X + PLAYER_W / 2;
  const uint16_t body = onGround_ ? Theme::CYAN : Theme::WHITE;

  tft_.fillCircle(cx, y + 9, 10, body);
  tft_.fillRoundRect(PLAYER_X + 2, y + 8, PLAYER_W - 4, PLAYER_H - 8, 9, body);
  tft_.drawRoundRect(PLAYER_X + 2, y + 8, PLAYER_W - 4, PLAYER_H - 8, 9, Theme::CYAN_DARK);
  tft_.fillRoundRect(PLAYER_X + 6, y + 8, 13, 9, 4, Theme::BG_2);
  tft_.drawFastHLine(PLAYER_X + 8, y + 12, 9, Theme::LIME);
  tft_.fillCircle(cx + 5, y + 3, 2, Theme::YELLOW);
  tft_.drawLine(cx + 5, y + 1, cx + 9, y - 4, Theme::YELLOW);
  tft_.fillRect(PLAYER_X + 4, y + PLAYER_H - 2, 6, 3, Theme::PURPLE);
  tft_.fillRect(PLAYER_X + 15, y + PLAYER_H - 2, 6, 3, Theme::PURPLE);
}

void RunnerGame::drawObstacleAt(const Obstacle& obstacle, int16_t x) {
  const int16_t y = GROUND_Y - obstacle.height;

  if (obstacle.type <= 2) {
    const uint8_t spikes = static_cast<uint8_t>(obstacle.type + 1);
    const int16_t spikeWidth = obstacle.width / spikes;
    for (uint8_t i = 0; i < spikes; ++i) {
      const int16_t left = x + i * spikeWidth;
      tft_.fillTriangle(left, GROUND_Y, left + spikeWidth / 2, y,
                        left + spikeWidth, GROUND_Y, Theme::PINK);
      tft_.drawLine(left + spikeWidth / 2, y, left + spikeWidth, GROUND_Y,
                    Theme::WHITE);
    }
    tft_.drawFastHLine(x, GROUND_Y - 1, obstacle.width, Theme::RED);
    return;
  }

  tft_.fillRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::ORANGE);
  tft_.drawRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::WHITE);
  tft_.drawLine(x + 4, y + 5, x + obstacle.width - 5, y + obstacle.height - 5,
                Theme::RED);
  tft_.drawLine(x + obstacle.width - 5, y + 5, x + 4, y + obstacle.height - 5,
                Theme::RED);
}

void RunnerGame::drawCoinAt(int16_t x, int16_t y) {
  tft_.fillCircle(x, y, 7, Theme::YELLOW);
  tft_.drawCircle(x, y, 8, Theme::GOLD);
  tft_.drawCircle(x, y, 4, Theme::ORANGE);
  tft_.drawFastVLine(x, y - 3, 7, Theme::WHITE);
}

void RunnerGame::clearDynamicRect(int16_t x, int16_t y, int16_t width, int16_t height) {
  int16_t left = std::max<int16_t>(0, x);
  int16_t top = std::max<int16_t>(DYNAMIC_TOP, y);
  int16_t right = std::min<int16_t>(320, x + width);
  int16_t bottom = std::min<int16_t>(240, y + height);
  if (right <= left || bottom <= top) {
    return;
  }

  if (top < GROUND_Y) {
    const int16_t skyBottom = std::min<int16_t>(bottom, GROUND_Y);
    tft_.fillRect(left, top, right - left, skyBottom - top, Theme::SKY_DARK);
  }
  if (bottom > GROUND_Y) {
    const int16_t groundTop = std::max<int16_t>(top, GROUND_Y);
    tft_.fillRect(left, groundTop, right - left, bottom - groundTop, Theme::PANEL);
  }
  if (top <= GROUND_Y && bottom >= GROUND_Y) {
    tft_.drawFastHLine(left, GROUND_Y, right - left, Theme::LIME);
    if (bottom > GROUND_Y + 2) {
      tft_.drawFastHLine(left, GROUND_Y + 2, right - left, Theme::CYAN_DARK);
    }
  }
}

void RunnerGame::eraseRenderedObjects() {
  if (renderedPlayerValid_) {
    clearDynamicRect(PLAYER_X - 4, renderedPlayerY_ - 6, PLAYER_W + 8, PLAYER_H + 10);
  }

  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    Obstacle& obstacle = obstacles_[i];
    if (obstacle.drawn) {
      clearDynamicRect(obstacle.drawnX - 2, GROUND_Y - obstacle.height - 2,
                       obstacle.width + 4, obstacle.height + 5);
      obstacle.drawn = false;
    }
  }

  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    Coin& coin = coins_[i];
    if (coin.drawn) {
      clearDynamicRect(coin.drawnX - 10, coin.drawnY - 10, 21, 21);
      coin.drawn = false;
    }
  }
}

void RunnerGame::drawDynamicObjects() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    Obstacle& obstacle = obstacles_[i];
    if (!obstacle.active) {
      continue;
    }
    const int16_t x = static_cast<int16_t>(obstacle.x);
    drawObstacleAt(obstacle, x);
    obstacle.drawn = true;
    obstacle.drawnX = x;
  }

  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    Coin& coin = coins_[i];
    if (!coin.active) {
      continue;
    }
    const int16_t x = static_cast<int16_t>(coin.x);
    drawCoinAt(x, coin.y);
    coin.drawn = true;
    coin.drawnX = x;
    coin.drawnY = coin.y;
  }

  const int16_t playerY = static_cast<int16_t>(playerY_);
  drawPlayerAt(playerY);
  renderedPlayerValid_ = true;
  renderedPlayerY_ = playerY;
}

void RunnerGame::renderDynamicFrame() {
  tft_.startWrite();
  eraseRenderedObjects();
  drawDynamicObjects();
  tft_.endWrite();
  updateHud(false);
}

void RunnerGame::drawGameFrame() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "STAR POD SPRINT");
  drawStaticPlayfield();
  drawHudFrame();

  renderedPlayerValid_ = false;
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    obstacles_[i].drawn = false;
  }
  for (uint8_t i = 0; i < MAX_COINS; ++i) {
    coins_[i].drawn = false;
  }
  drawDynamicObjects();
}

void RunnerGame::drawPauseOverlay() {
  tft_.fillRoundRect(70, 91, 180, 87, 11, Theme::BG);
  tft_.drawRoundRect(70, 91, 180, 87, 11, Theme::CYAN);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("PAUSED", 160, 117);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap pause to continue", 160, 151);
  drawPauseButton();
}

void RunnerGame::endGame() {
  if (state_ == State::GameOver) {
    return;
  }
  state_ = State::GameOver;
  if (score_ > highScore_) {
    highScore_ = score_;
  }
  drawGameOver();
}

void RunnerGame::drawGameOver() {
  tft_.fillRoundRect(48, 78, 224, 154, 12, Theme::BG);
  tft_.drawRoundRect(48, 78, 224, 154, 12, Theme::PINK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::PINK, Theme::BG);
  tft_.drawString("RUN OVER", 160, 103);

  char text[48];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "Score %lu   High %lu", static_cast<unsigned long>(score_),
                static_cast<unsigned long>(highScore_));
  tft_.drawString(text, 160, 136);
  std::snprintf(text, sizeof(text), "Coins %u   Dodged %u", static_cast<unsigned>(coinCount_),
                static_cast<unsigned>(dodged_));
  tft_.drawString(text, 160, 161);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString(difficultyLabel(), 160, 181);
  Ui::drawButton(tft_, AGAIN_BUTTON, "PLAY AGAIN", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void RunnerGame::update(const InputFrame& input, uint32_t now) {
  if (state_ == State::Intro) {
    if (!input.pressed) {
      return;
    }
    if (contains(CHILL_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Chill;
      drawDifficultyButtons();
    } else if (contains(ARCADE_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Arcade;
      drawDifficultyButtons();
    } else if (contains(TURBO_BUTTON, input.x, input.y)) {
      difficulty_ = Difficulty::Turbo;
      drawDifficultyButtons();
    } else if (contains(START_BUTTON, input.x, input.y)) {
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
      drawPauseOverlay();
    } else if (state_ == State::Paused) {
      state_ = State::Running;
      lastUpdateAt_ = now;
      drawGameFrame();
    }
    return;
  }

  if (state_ == State::Paused) {
    return;
  }

  if (input.pressed && input.y > PLAY_TOP) {
    jump();
  }

  const uint32_t elapsed = now - lastUpdateAt_;
  lastUpdateAt_ = now;
  const float dt = std::min<float>(0.10f, static_cast<float>(elapsed) / 1000.0f);
  updatePhysics(dt);
  updateObjects(dt, now);
  handleCollisions();
  if (state_ != State::Running) {
    return;
  }

  if (now - lastRenderAt_ >= 50) {
    lastRenderAt_ = now;
    renderDynamicFrame();
  }
}
