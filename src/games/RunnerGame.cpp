#include "games/RunnerGame.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Theme.h"
#include "Ui.h"

constexpr int16_t RunnerGame::PLAYER_X;
constexpr int16_t RunnerGame::PLAYER_W;
constexpr int16_t RunnerGame::PLAYER_H;
constexpr int16_t RunnerGame::GROUND_Y;
constexpr uint8_t RunnerGame::MAX_OBSTACLES;
constexpr uint8_t RunnerGame::MAX_COLLECTIBLES;
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
  Ui::drawSparkles(tft_, 20, 44, 228);
  Ui::drawRunnerIcon(tft_, 160, 86, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Tap to jump over the neon course", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap again in the air for one boost", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void RunnerGame::configureDifficulty() {
  switch (difficulty_) {
    case Difficulty::Chill:
      speed_ = 78.0f;
      break;
    case Difficulty::Turbo:
      speed_ = 118.0f;
      break;
    case Difficulty::Arcade:
    default:
      speed_ = 96.0f;
      break;
  }
}

void RunnerGame::startGame(uint32_t now) {
  configureDifficulty();
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    obstacles_[i].active = false;
  }
  for (uint8_t i = 0; i < MAX_COLLECTIBLES; ++i) {
    collectibles_[i].active = false;
  }

  state_ = State::Running;
  playerY_ = static_cast<float>(GROUND_Y - PLAYER_H);
  velocityY_ = 0.0f;
  distance_ = 0.0f;
  score_ = 0;
  sparks_ = 0;
  dodged_ = 0;
  level_ = 1;
  onGround_ = true;
  boostUsed_ = false;
  shield_ = false;
  nextObstacleAt_ = now + 1050;
  lastUpdateAt_ = now;
  lastRenderAt_ = 0;
  drawGameFrame();
}

void RunnerGame::jump() {
  if (onGround_) {
    velocityY_ = -410.0f;
    onGround_ = false;
    boostUsed_ = false;
  } else if (!boostUsed_) {
    velocityY_ = -300.0f;
    boostUsed_ = true;
  }
}

void RunnerGame::spawnCollectible(float x, int16_t y, uint8_t kind) {
  for (uint8_t i = 0; i < MAX_COLLECTIBLES; ++i) {
    if (!collectibles_[i].active) {
      collectibles_[i].active = true;
      collectibles_[i].x = x;
      collectibles_[i].y = y;
      collectibles_[i].kind = kind;
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
    obstacle.x = 326.0f;
    obstacle.passed = false;

    const long roll = random(0, 100);
    if (level_ >= 3 && roll < 18) {
      obstacle.type = 3;
      obstacle.width = 20;
      obstacle.height = 20;
    } else if (level_ >= 2 && roll < 40) {
      obstacle.type = 2;
      obstacle.width = 18;
      obstacle.height = 38;
    } else if (roll < 70) {
      obstacle.type = 1;
      obstacle.width = 29;
      obstacle.height = 17;
    } else {
      obstacle.type = 0;
      obstacle.width = 22;
      obstacle.height = 25;
    }

    if (random(0, 100) < 72) {
      const uint8_t kind = !shield_ && random(0, 100) < 9 ? 1 : 0;
      const int16_t heightAbove = static_cast<int16_t>(random(30, 59));
      spawnCollectible(obstacle.x + obstacle.width / 2.0f + 5.0f,
                       GROUND_Y - obstacle.height - heightAbove, kind);
      if (level_ >= 4 && random(0, 100) < 35) {
        spawnCollectible(obstacle.x + obstacle.width + 30.0f,
                         GROUND_Y - obstacle.height - heightAbove - 8, 0);
      }
    }
  }

  uint16_t baseGap = 1100;
  if (difficulty_ == Difficulty::Chill) {
    baseGap = 1320;
  } else if (difficulty_ == Difficulty::Turbo) {
    baseGap = 900;
  }
  const uint16_t reduction = static_cast<uint16_t>(std::min<int>(260, level_ * 24));
  nextObstacleAt_ = now + baseGap - reduction + static_cast<uint32_t>(random(0, 330));
}

void RunnerGame::updatePhysics(float dt) {
  velocityY_ += 850.0f * dt;
  playerY_ += velocityY_ * dt;

  const float groundTop = static_cast<float>(GROUND_Y - PLAYER_H);
  if (playerY_ >= groundTop) {
    playerY_ = groundTop;
    velocityY_ = 0.0f;
    onGround_ = true;
    boostUsed_ = false;
  }
  if (playerY_ < 48.0f) {
    playerY_ = 48.0f;
    velocityY_ = 20.0f;
  }
}

bool RunnerGame::intersectsPlayer(float x, int16_t y, int16_t width, int16_t height) const {
  const float playerLeft = static_cast<float>(PLAYER_X + 5);
  const float playerRight = static_cast<float>(PLAYER_X + PLAYER_W - 4);
  const float playerTop = playerY_ + 4.0f;
  const float playerBottom = playerY_ + PLAYER_H - 5.0f;

  return playerRight > x && playerLeft < x + width && playerBottom > y &&
         playerTop < y + height;
}

void RunnerGame::flashShieldBreak() {
  const int16_t cx = PLAYER_X + PLAYER_W / 2;
  const int16_t cy = static_cast<int16_t>(playerY_) + PLAYER_H / 2;
  for (uint8_t i = 0; i < 4; ++i) {
    tft_.drawCircle(cx, cy, 17 + i * 5, i % 2 == 0 ? Theme::CYAN : Theme::WHITE);
    delay(24);
  }
}

void RunnerGame::handleCollisions() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    Obstacle& obstacle = obstacles_[i];
    if (!obstacle.active) {
      continue;
    }
    const int16_t obstacleY = GROUND_Y - obstacle.height;
    if (intersectsPlayer(obstacle.x, obstacleY, obstacle.width, obstacle.height)) {
      if (shield_) {
        shield_ = false;
        obstacle.active = false;
        flashShieldBreak();
      } else {
        endGame();
      }
      return;
    }
  }

  for (uint8_t i = 0; i < MAX_COLLECTIBLES; ++i) {
    Collectible& collectible = collectibles_[i];
    if (!collectible.active) {
      continue;
    }
    if (intersectsPlayer(collectible.x - 7.0f, collectible.y - 7, 14, 14)) {
      collectible.active = false;
      if (collectible.kind == 1) {
        shield_ = true;
      } else {
        ++sparks_;
      }
    }
  }
}

void RunnerGame::updateObjects(float dt, uint32_t now) {
  float baseSpeed = 96.0f;
  if (difficulty_ == Difficulty::Chill) {
    baseSpeed = 78.0f;
  } else if (difficulty_ == Difficulty::Turbo) {
    baseSpeed = 118.0f;
  }

  distance_ += speed_ * dt;
  level_ = static_cast<uint8_t>(1 + std::min<int>(8, static_cast<int>(distance_ / 760.0f)));
  speed_ = baseSpeed + std::min<float>(92.0f, distance_ / 55.0f);

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

  for (uint8_t i = 0; i < MAX_COLLECTIBLES; ++i) {
    Collectible& collectible = collectibles_[i];
    if (!collectible.active) {
      continue;
    }
    collectible.x -= speed_ * dt;
    if (collectible.x < -12.0f) {
      collectible.active = false;
    }
  }

  score_ = static_cast<uint32_t>(distance_ / 7.0f) + static_cast<uint32_t>(sparks_) * 30U +
           static_cast<uint32_t>(dodged_) * 18U;

  if (static_cast<int32_t>(now - nextObstacleAt_) >= 0) {
    spawnObstacle(now);
  }
}

void RunnerGame::drawBackground() {
  const int32_t starShift = static_cast<int32_t>(distance_ * 0.12f);
  for (uint8_t i = 0; i < 20; ++i) {
    int16_t x = static_cast<int16_t>((i * 53 - starShift) % 340);
    if (x < 0) {
      x += 340;
    }
    const int16_t y = static_cast<int16_t>(51 + (i * 29) % 108);
    const uint16_t color = i % 4 == 0 ? Theme::CYAN : Theme::GRID;
    tft_.drawPixel(x, y, color);
    if (i % 5 == 0) {
      tft_.drawPixel(x + 1, y, color);
    }
  }

  const int32_t hillShift = static_cast<int32_t>(distance_ * 0.22f) % 96;
  for (int16_t base = -96 - hillShift; base < 350; base += 96) {
    tft_.fillTriangle(base, GROUND_Y, base + 45, 150, base + 92, GROUND_Y, Theme::BG_2);
    tft_.drawLine(base + 45, 150, base + 92, GROUND_Y, Theme::CYAN_DARK);
  }
}

void RunnerGame::drawGround() {
  tft_.fillRect(0, GROUND_Y, 320, 240 - GROUND_Y, Theme::PANEL);
  tft_.drawFastHLine(0, GROUND_Y, 320, Theme::LIME);
  tft_.drawFastHLine(0, GROUND_Y + 3, 320, Theme::CYAN_DARK);

  const int16_t shift = static_cast<int16_t>(static_cast<int32_t>(distance_) % 32);
  for (int16_t x = -32 - shift; x < 340; x += 32) {
    tft_.fillRoundRect(x, GROUND_Y + 16, 18, 4, 2, Theme::GRID);
  }
}

void RunnerGame::drawPlayer() {
  const int16_t y = static_cast<int16_t>(playerY_);
  const int16_t cx = PLAYER_X + PLAYER_W / 2;
  const uint16_t body = onGround_ ? Theme::CYAN : Theme::WHITE;

  if (shield_) {
    tft_.drawCircle(cx, y + PLAYER_H / 2, 20, Theme::CYAN);
    tft_.drawCircle(cx, y + PLAYER_H / 2, 22, Theme::CYAN_DARK);
  }

  tft_.fillCircle(cx, y + 9, 10, body);
  tft_.fillRoundRect(PLAYER_X + 2, y + 8, PLAYER_W - 4, PLAYER_H - 8, 9, body);
  tft_.fillRoundRect(PLAYER_X + 6, y + 8, 13, 9, 4, Theme::BG_2);
  tft_.drawFastHLine(PLAYER_X + 8, y + 12, 9, Theme::LIME);
  tft_.fillCircle(cx + 5, y + 3, 2, Theme::YELLOW);
  tft_.drawLine(cx + 5, y + 1, cx + 9, y - 4, Theme::YELLOW);
  tft_.fillRect(PLAYER_X + 5, y + PLAYER_H - 2, 5, 3, Theme::PURPLE);
  tft_.fillRect(PLAYER_X + 15, y + PLAYER_H - 2, 5, 3, Theme::PURPLE);
}

void RunnerGame::drawObstacle(const Obstacle& obstacle) {
  const int16_t x = static_cast<int16_t>(obstacle.x);
  const int16_t y = GROUND_Y - obstacle.height;

  switch (obstacle.type) {
    case 1:
      for (int16_t offset = 0; offset < obstacle.width; offset += 10) {
        tft_.fillTriangle(x + offset, GROUND_Y, x + offset + 5, y,
                          x + offset + 10, GROUND_Y, Theme::PINK);
      }
      tft_.drawFastHLine(x, GROUND_Y - 1, obstacle.width, Theme::WHITE);
      break;
    case 2:
      tft_.fillRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::PURPLE);
      tft_.drawRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::PINK);
      tft_.fillRect(x + 4, y + 7, obstacle.width - 8, 5, Theme::BG_2);
      tft_.fillCircle(x + obstacle.width / 2, y + 4, 2, Theme::RED);
      break;
    case 3:
      tft_.fillCircle(x + obstacle.width / 2, y + obstacle.height / 2,
                      obstacle.width / 2, Theme::ORANGE);
      tft_.drawCircle(x + obstacle.width / 2, y + obstacle.height / 2,
                      obstacle.width / 2, Theme::YELLOW);
      tft_.drawLine(x + 4, y + 4, x + obstacle.width - 4, y + obstacle.height - 4,
                    Theme::BG_2);
      tft_.drawLine(x + obstacle.width - 4, y + 4, x + 4, y + obstacle.height - 4,
                    Theme::BG_2);
      break;
    case 0:
    default:
      tft_.fillRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::ORANGE);
      tft_.drawRoundRect(x, y, obstacle.width, obstacle.height, 4, Theme::YELLOW);
      tft_.drawLine(x + 4, y + 4, x + obstacle.width - 4, y + obstacle.height - 4,
                    Theme::BG_2);
      tft_.drawLine(x + obstacle.width - 4, y + 4, x + 4, y + obstacle.height - 4,
                    Theme::BG_2);
      break;
  }
}

void RunnerGame::drawCollectible(const Collectible& collectible) {
  const int16_t x = static_cast<int16_t>(collectible.x);
  const int16_t y = collectible.y;
  if (collectible.kind == 1) {
    tft_.drawCircle(x, y, 8, Theme::CYAN);
    tft_.drawCircle(x, y, 5, Theme::WHITE);
    tft_.fillCircle(x, y, 2, Theme::LIME);
  } else {
    tft_.fillCircle(x, y, 6, Theme::YELLOW);
    tft_.drawFastHLine(x - 9, y, 19, Theme::ORANGE);
    tft_.drawFastVLine(x, y - 9, 19, Theme::ORANGE);
    tft_.fillCircle(x, y, 2, Theme::WHITE);
  }
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

void RunnerGame::drawHud() {
  tft_.fillRect(60, 0, 218, 38, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  tft_.drawString("SCORE", 68, 10);
  tft_.drawString("SPARKS", 151, 10);
  tft_.drawString("LEVEL", 222, 10);

  char text[24];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
  tft_.drawString(text, 68, 26);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(sparks_));
  tft_.setTextColor(Theme::YELLOW, Theme::BG_2);
  tft_.drawString(text, 151, 26);
  std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(level_));
  tft_.setTextColor(Theme::CYAN, Theme::BG_2);
  tft_.drawString(text, 222, 26);
  drawPauseButton();
}

void RunnerGame::renderFrame() {
  tft_.fillRect(0, 40, 320, 200, Theme::SKY_DARK);
  drawBackground();
  drawGround();

  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) {
    if (obstacles_[i].active) {
      drawObstacle(obstacles_[i]);
    }
  }
  for (uint8_t i = 0; i < MAX_COLLECTIBLES; ++i) {
    if (collectibles_[i].active) {
      drawCollectible(collectibles_[i]);
    }
  }
  drawPlayer();
  drawHud();
}

void RunnerGame::drawGameFrame() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "STAR POD SPRINT");
  renderFrame();
}

void RunnerGame::drawPauseOverlay() {
  tft_.fillRoundRect(70, 85, 180, 91, 11, Theme::BG);
  tft_.drawRoundRect(70, 85, 180, 91, 11, Theme::CYAN);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("PAUSED", 160, 112);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Tap pause to continue", 160, 149);
  drawHud();
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
  tft_.fillRoundRect(48, 73, 224, 159, 12, Theme::BG);
  tft_.drawRoundRect(48, 73, 224, 159, 12, Theme::PINK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::PINK, Theme::BG);
  tft_.drawString("POD DOWN", 160, 99);

  char text[48];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "Score %lu   High %lu", static_cast<unsigned long>(score_),
                static_cast<unsigned long>(highScore_));
  tft_.drawString(text, 160, 132);
  std::snprintf(text, sizeof(text), "Sparks %u   Dodged %u", static_cast<unsigned>(sparks_),
                static_cast<unsigned>(dodged_));
  tft_.drawString(text, 160, 158);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString(difficultyLabel(), 160, 178);
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
      renderFrame();
    }
    return;
  }

  if (state_ == State::Paused) {
    return;
  }

  if (input.pressed && input.y > 40) {
    jump();
  }

  const uint32_t elapsed = now - lastUpdateAt_;
  lastUpdateAt_ = now;
  const float dt = std::min<float>(0.12f, static_cast<float>(elapsed) / 1000.0f);
  updatePhysics(dt);
  updateObjects(dt, now);
  handleCollisions();
  if (state_ != State::Running) {
    return;
  }

  if (now - lastRenderAt_ >= 60) {
    lastRenderAt_ = now;
    renderFrame();
  }
}
