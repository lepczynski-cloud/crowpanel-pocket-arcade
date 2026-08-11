#include "games/ReactionGame.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t ReactionGame::MAX_TARGETS;
constexpr Rect ReactionGame::EASY_BUTTON;
constexpr Rect ReactionGame::NORMAL_BUTTON;
constexpr Rect ReactionGame::HARD_BUTTON;
constexpr Rect ReactionGame::START_BUTTON;
constexpr Rect ReactionGame::AGAIN_BUTTON;

ReactionGame::ReactionGame(TFT_eSPI& display) : tft_(display) {}

void ReactionGame::enter() {
  phase_ = Phase::Intro;
  drawIntro();
}

const char* ReactionGame::difficultyLabel() const {
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

void ReactionGame::drawDifficultyButtons() {
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

void ReactionGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "BURST HUNT");
  Ui::drawSparkles(tft_, 18, 44, 228);

  Target berry{true, false, ObjectKind::Berry, 91, 88, 20, 90, 0};
  Target citrus{true, false, ObjectKind::Citrus, 160, 82, 21, 100, 0};
  Target mine{true, true, ObjectKind::Mine, 231, 89, 20, 0, 0};
  drawBerry(berry);
  drawCitrus(citrus);
  drawMine(mine);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Smash bright loot. Avoid glitch mines.", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Targets move faster as difficulty rises", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void ReactionGame::configureDifficulty() {
  switch (difficulty_) {
    case Difficulty::Easy:
      sessionDuration_ = 40000;
      spawnInterval_ = 780;
      targetLifetime_ = 1650;
      maxTargets_ = 2;
      hazardChance_ = 18;
      break;
    case Difficulty::Hard:
      sessionDuration_ = 30000;
      spawnInterval_ = 430;
      targetLifetime_ = 900;
      maxTargets_ = 4;
      hazardChance_ = 38;
      break;
    case Difficulty::Normal:
    default:
      sessionDuration_ = 35000;
      spawnInterval_ = 610;
      targetLifetime_ = 1250;
      maxTargets_ = 3;
      hazardChance_ = 28;
      break;
  }
}

void ReactionGame::startSession(uint32_t now) {
  configureDifficulty();
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    targets_[i].active = false;
  }

  score_ = 0;
  hits_ = 0;
  misses_ = 0;
  minesHit_ = 0;
  combo_ = 0;
  lives_ = 3;
  sessionStartedAt_ = now;
  sessionEndsAt_ = now + sessionDuration_;
  nextSpawnAt_ = now + 420;
  lastTimerDraw_ = 0;
  phase_ = Phase::Playing;

  spawnTarget(now, true);
  if (difficulty_ != Difficulty::Easy) {
    spawnTarget(now, true);
  }
  drawPlayfield(now);
}

uint8_t ReactionGame::activeTargetCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    if (targets_[i].active) {
      ++count;
    }
  }
  return count;
}

uint8_t ReactionGame::activeGoodTargetCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    if (targets_[i].active && !targets_[i].hazard) {
      ++count;
    }
  }
  return count;
}

bool ReactionGame::positionAvailable(int16_t x, int16_t y, int16_t radius) const {
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    if (!targets_[i].active) {
      continue;
    }
    const int32_t dx = static_cast<int32_t>(x) - targets_[i].x;
    const int32_t dy = static_cast<int32_t>(y) - targets_[i].y;
    const int32_t spacing = radius + targets_[i].radius + 12;
    if (dx * dx + dy * dy < spacing * spacing) {
      return false;
    }
  }
  return true;
}

void ReactionGame::spawnTarget(uint32_t now, bool forceGood) {
  int8_t slot = -1;
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    if (!targets_[i].active) {
      slot = static_cast<int8_t>(i);
      break;
    }
  }
  if (slot < 0) {
    return;
  }

  const uint8_t progress = sessionProgress(now);
  const uint8_t dynamicHazardChance = static_cast<uint8_t>(
      std::min<int>(55, hazardChance_ + progress / 10));
  const bool hazard = !forceGood && activeGoodTargetCount() > 0 &&
                      random(0, 100) < dynamicHazardChance;
  int16_t radius = 18;
  if (difficulty_ == Difficulty::Easy) {
    radius = static_cast<int16_t>(random(20, 25));
  } else if (difficulty_ == Difficulty::Hard) {
    radius = static_cast<int16_t>(random(13, 19));
  } else {
    radius = static_cast<int16_t>(random(16, 22));
  }
  if (difficulty_ != Difficulty::Easy) {
    radius = std::max<int16_t>(12, radius - static_cast<int16_t>(progress / 35));
  }

  int16_t x = 160;
  int16_t y = 135;
  bool positionFound = false;
  for (uint8_t attempt = 0; attempt < 70; ++attempt) {
    const int16_t candidateX = static_cast<int16_t>(random(radius + 12, 308 - radius));
    const int16_t candidateY = static_cast<int16_t>(random(82 + radius, 214 - radius));
    if (positionAvailable(candidateX, candidateY, radius)) {
      x = candidateX;
      y = candidateY;
      positionFound = true;
      break;
    }
  }
  if (!positionFound) {
    return;
  }

  Target& target = targets_[slot];
  target.active = true;
  target.hazard = hazard;
  target.kind = hazard ? ObjectKind::Mine : static_cast<ObjectKind>(random(0, 4));
  target.x = x;
  target.y = y;
  target.radius = radius;
  target.value = 90;
  if (!hazard) {
    switch (target.kind) {
      case ObjectKind::Citrus:
        target.value = 105;
        break;
      case ObjectKind::Crystal:
        target.value = 120;
        break;
      case ObjectKind::Star:
        target.value = 140;
        break;
      case ObjectKind::Berry:
      default:
        target.value = 90;
        break;
    }
    if (difficulty_ == Difficulty::Hard) {
      target.value += 25;
    }
  } else {
    target.value = 0;
  }

  const int32_t lifetimeJitter = static_cast<int32_t>(random(-130, 260));
  const int32_t paceReduction = difficulty_ == Difficulty::Easy ? progress : progress * 3;
  target.expiresAt = now + static_cast<uint32_t>(
                               std::max<int32_t>(560, targetLifetime_ + lifetimeJitter - paceReduction));
}

uint16_t ReactionGame::targetColor(const Target& target) const {
  switch (target.kind) {
    case ObjectKind::Berry:
      return Theme::PINK;
    case ObjectKind::Citrus:
      return Theme::ORANGE;
    case ObjectKind::Crystal:
      return Theme::CYAN;
    case ObjectKind::Star:
      return Theme::YELLOW;
    case ObjectKind::Mine:
    default:
      return Theme::RED;
  }
}

void ReactionGame::drawBerry(const Target& target) {
  const int16_t r = std::max<int16_t>(4, target.radius / 3);
  tft_.fillCircle(target.x - r, target.y, r + 1, Theme::PINK);
  tft_.fillCircle(target.x + r, target.y, r + 1, Theme::PURPLE);
  tft_.fillCircle(target.x, target.y - r, r + 1, Theme::PINK);
  tft_.fillCircle(target.x, target.y + r, r + 1, Theme::PURPLE);
  tft_.fillCircle(target.x, target.y, r + 2, Theme::WHITE);
  tft_.fillTriangle(target.x - 3, target.y - target.radius + 3, target.x + 2,
                    target.y - target.radius - 5, target.x + 8, target.y - target.radius + 3,
                    Theme::LIME);
  tft_.drawCircle(target.x, target.y, target.radius + 2, Theme::PINK);
}

void ReactionGame::drawCitrus(const Target& target) {
  tft_.fillCircle(target.x, target.y, target.radius, Theme::ORANGE);
  tft_.drawCircle(target.x, target.y, target.radius + 2, Theme::YELLOW);
  tft_.fillCircle(target.x, target.y, std::max<int16_t>(3, target.radius / 5), Theme::YELLOW);
  tft_.drawFastHLine(target.x - target.radius + 4, target.y, target.radius * 2 - 8,
                     Theme::YELLOW);
  tft_.drawFastVLine(target.x, target.y - target.radius + 4, target.radius * 2 - 8,
                     Theme::YELLOW);
  tft_.drawLine(target.x - target.radius / 2, target.y - target.radius / 2,
                target.x + target.radius / 2, target.y + target.radius / 2, Theme::GOLD);
  tft_.drawLine(target.x + target.radius / 2, target.y - target.radius / 2,
                target.x - target.radius / 2, target.y + target.radius / 2, Theme::GOLD);
}

void ReactionGame::drawCrystal(const Target& target) {
  const int16_t r = target.radius;
  tft_.fillTriangle(target.x, target.y - r, target.x - r, target.y,
                    target.x + r, target.y, Theme::CYAN);
  tft_.fillTriangle(target.x, target.y + r, target.x - r, target.y,
                    target.x + r, target.y, Theme::PURPLE);
  tft_.drawLine(target.x, target.y - r, target.x, target.y + r, Theme::WHITE);
  tft_.drawLine(target.x - r, target.y, target.x + r, target.y, Theme::WHITE);
}

void ReactionGame::drawStar(const Target& target) {
  const int16_t r = target.radius;
  const int16_t inner = std::max<int16_t>(5, r / 2);
  tft_.fillTriangle(target.x, target.y - r, target.x - inner, target.y - 2,
                    target.x + inner, target.y - 2, Theme::YELLOW);
  tft_.fillTriangle(target.x, target.y + r, target.x - inner, target.y + 2,
                    target.x + inner, target.y + 2, Theme::GOLD);
  tft_.fillTriangle(target.x - r, target.y, target.x - 2, target.y - inner,
                    target.x - 2, target.y + inner, Theme::YELLOW);
  tft_.fillTriangle(target.x + r, target.y, target.x + 2, target.y - inner,
                    target.x + 2, target.y + inner, Theme::GOLD);
  tft_.fillCircle(target.x, target.y, inner, Theme::WHITE);
}

void ReactionGame::drawMine(const Target& target) {
  const int16_t r = target.radius;
  tft_.fillTriangle(target.x, target.y - r - 5, target.x - 5, target.y - r + 3,
                    target.x + 5, target.y - r + 3, Theme::RED);
  tft_.fillTriangle(target.x, target.y + r + 5, target.x - 5, target.y + r - 3,
                    target.x + 5, target.y + r - 3, Theme::RED);
  tft_.fillTriangle(target.x - r - 5, target.y, target.x - r + 3, target.y - 5,
                    target.x - r + 3, target.y + 5, Theme::RED);
  tft_.fillTriangle(target.x + r + 5, target.y, target.x + r - 3, target.y - 5,
                    target.x + r - 3, target.y + 5, Theme::RED);
  tft_.fillCircle(target.x, target.y, r, Theme::PURPLE);
  tft_.drawCircle(target.x, target.y, r + 2, Theme::PINK);
  tft_.drawLine(target.x - r / 2, target.y - r / 2, target.x + r / 2,
                target.y + r / 2, Theme::BG);
  tft_.drawLine(target.x + r / 2, target.y - r / 2, target.x - r / 2,
                target.y + r / 2, Theme::BG);
  tft_.fillCircle(target.x, target.y, 3, Theme::WHITE);
}

void ReactionGame::drawTarget(const Target& target) {
  if (!target.active) {
    return;
  }
  switch (target.kind) {
    case ObjectKind::Berry:
      drawBerry(target);
      break;
    case ObjectKind::Citrus:
      drawCitrus(target);
      break;
    case ObjectKind::Crystal:
      drawCrystal(target);
      break;
    case ObjectKind::Star:
      drawStar(target);
      break;
    case ObjectKind::Mine:
      drawMine(target);
      break;
  }
}

uint32_t ReactionGame::remainingTime(uint32_t now) const {
  const int32_t remaining = static_cast<int32_t>(sessionEndsAt_ - now);
  return remaining > 0 ? static_cast<uint32_t>(remaining) : 0;
}

uint8_t ReactionGame::sessionProgress(uint32_t now) const {
  if (sessionDuration_ == 0) {
    return 100;
  }
  const uint32_t elapsed = now - sessionStartedAt_;
  if (elapsed >= sessionDuration_) {
    return 100;
  }
  return static_cast<uint8_t>((elapsed * 100UL) / sessionDuration_);
}

void ReactionGame::drawHud(uint32_t now) {
  tft_.fillRect(0, 40, 320, 31, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  tft_.drawString("SCORE", 9, 49);
  tft_.drawString("COMBO", 111, 49);
  tft_.drawString("LIVES", 206, 49);
  tft_.setTextDatum(MR_DATUM);
  tft_.drawString(difficultyLabel(), 310, 49);
  tft_.setTextDatum(ML_DATUM);

  char text[24];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
  tft_.drawString(text, 9, 63);
  std::snprintf(text, sizeof(text), "x%u", static_cast<unsigned>(combo_));
  tft_.setTextColor(combo_ >= 4 ? Theme::YELLOW : Theme::TEXT, Theme::BG_2);
  tft_.drawString(text, 111, 63);

  for (uint8_t i = 0; i < 3; ++i) {
    const uint16_t color = i < lives_ ? Theme::PINK : Theme::PANEL;
    tft_.fillCircle(217 + i * 17, 61, 6, color);
  }

  const uint32_t remaining = remainingTime(now);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextColor(remaining < 6000 ? Theme::PINK : Theme::CYAN, Theme::BG_2);
  std::snprintf(text, sizeof(text), "%lus", static_cast<unsigned long>((remaining + 999) / 1000));
  tft_.drawString(text, 310, 63);
}

void ReactionGame::drawTimer(uint32_t now) {
  const uint32_t remaining = remainingTime(now);
  const int16_t width = static_cast<int16_t>((remaining * 288UL) / sessionDuration_);
  tft_.fillRoundRect(16, 229, 288, 7, 4, Theme::PANEL);
  if (width > 0) {
    const uint16_t color = remaining < 6000 ? Theme::PINK : Theme::LIME;
    tft_.fillRoundRect(16, 229, width, 7, 4, color);
  }
}

void ReactionGame::drawPlayfield(uint32_t now) {
  tft_.fillRect(0, 40, 320, 200, Theme::BG);
  tft_.fillRoundRect(6, 76, 308, 145, 10, Theme::BG_2);
  tft_.drawRoundRect(6, 76, 308, 145, 10, Theme::BORDER);

  for (int16_t x = 22; x < 310; x += 47) {
    tft_.drawPixel(x, 86 + (x % 23), Theme::GRID);
    tft_.drawPixel(x + 8, 201 - (x % 31), Theme::CYAN_DARK);
  }

  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    drawTarget(targets_[i]);
  }
  drawHud(now);
  drawTimer(now);
}

bool ReactionGame::expireTargets(uint32_t now) {
  bool changed = false;
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    Target& target = targets_[i];
    if (!target.active || static_cast<int32_t>(now - target.expiresAt) < 0) {
      continue;
    }
    target.active = false;
    changed = true;
    if (!target.hazard) {
      ++misses_;
      combo_ = 0;
      score_ = score_ >= 10 ? score_ - 10 : 0;
      if (difficulty_ == Difficulty::Hard && misses_ % 4 == 0 && lives_ > 0) {
        --lives_;
      }
    }
  }
  return changed;
}

int8_t ReactionGame::targetAt(uint16_t x, uint16_t y) const {
  for (int8_t i = static_cast<int8_t>(MAX_TARGETS) - 1; i >= 0; --i) {
    const Target& target = targets_[i];
    if (!target.active) {
      continue;
    }
    const int32_t dx = static_cast<int32_t>(x) - target.x;
    const int32_t dy = static_cast<int32_t>(y) - target.y;
    const int32_t hitRadius = target.radius + 6;
    if (dx * dx + dy * dy <= hitRadius * hitRadius) {
      return i;
    }
  }
  return -1;
}

void ReactionGame::animateBurst(int16_t x, int16_t y, uint16_t color, bool hazard) {
  static const int8_t directions[8][2] = {
      {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1},
  };

  for (uint8_t frame = 1; frame <= 5; ++frame) {
    const int16_t radius = 5 + frame * 6;
    tft_.drawCircle(x, y, radius, color);
    for (uint8_t i = 0; i < 8; ++i) {
      const int16_t x1 = x + directions[i][0] * (radius - 4);
      const int16_t y1 = y + directions[i][1] * (radius - 4);
      const int16_t x2 = x + directions[i][0] * (radius + 5);
      const int16_t y2 = y + directions[i][1] * (radius + 5);
      tft_.drawLine(x1, y1, x2, y2, i % 2 == 0 ? Theme::WHITE : color);
    }
    if (hazard) {
      tft_.drawLine(x - radius, y - radius, x + radius, y + radius, Theme::RED);
      tft_.drawLine(x + radius, y - radius, x - radius, y + radius, Theme::RED);
    }
    delay(18);
  }
}

void ReactionGame::handleTargetHit(uint8_t index, uint32_t now) {
  Target target = targets_[index];
  targets_[index].active = false;

  if (target.hazard) {
    ++minesHit_;
    combo_ = 0;
    score_ = score_ >= 120 ? score_ - 120 : 0;
    if (lives_ > 0) {
      --lives_;
    }
    animateBurst(target.x, target.y, Theme::RED, true);
  } else {
    ++hits_;
    combo_ = static_cast<uint8_t>(std::min<int>(12, combo_ + 1));
    score_ += target.value + static_cast<uint32_t>(combo_) * 18U;
    animateBurst(target.x, target.y, targetColor(target), false);
  }

  if (lives_ == 0) {
    finishSession();
    return;
  }

  drawPlayfield(now);
}

void ReactionGame::finishSession() {
  phase_ = Phase::Results;
  if (score_ > bestScore_) {
    bestScore_ = score_;
  }
  drawResults();
}

void ReactionGame::drawResults() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "BURST HUNT");
  Ui::drawSparkles(tft_, 18, 44, 228);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("HUNT COMPLETE", 160, 70);

  char text[56];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(text, sizeof(text), "Score %lu   Best %lu", static_cast<unsigned long>(score_),
                static_cast<unsigned long>(bestScore_));
  tft_.drawString(text, 160, 108);
  std::snprintf(text, sizeof(text), "Hits %u   Misses %u", static_cast<unsigned>(hits_),
                static_cast<unsigned>(misses_));
  tft_.drawString(text, 160, 135);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  std::snprintf(text, sizeof(text), "Mines hit %u   Difficulty %s",
                static_cast<unsigned>(minesHit_), difficultyLabel());
  tft_.drawString(text, 160, 160);

  Ui::drawButton(tft_, AGAIN_BUTTON, "PLAY AGAIN", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void ReactionGame::update(const InputFrame& input, uint32_t now) {
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
      startSession(now);
    }
    return;
  }

  if (phase_ == Phase::Results) {
    if (input.pressed && contains(AGAIN_BUTTON, input.x, input.y)) {
      startSession(now);
    }
    return;
  }

  if (remainingTime(now) == 0 || lives_ == 0) {
    finishSession();
    return;
  }

  bool changed = expireTargets(now);
  if (lives_ == 0) {
    finishSession();
    return;
  }
  if (changed) {
    drawPlayfield(now);
  }

  const uint8_t progress = sessionProgress(now);
  const uint8_t targetLimit = static_cast<uint8_t>(
      std::min<int>(MAX_TARGETS, maxTargets_ + (progress >= 55 ? 1 : 0)));
  if (static_cast<int32_t>(now - nextSpawnAt_) >= 0 && activeTargetCount() < targetLimit) {
    spawnTarget(now);
    const uint16_t reduction = static_cast<uint16_t>((spawnInterval_ * progress) / 320U);
    const uint16_t pacedInterval = std::max<uint16_t>(280, spawnInterval_ - reduction);
    nextSpawnAt_ = now + pacedInterval +
                   static_cast<uint32_t>(random(0, pacedInterval / 3 + 1));
    drawPlayfield(now);
  }

  if (input.pressed && input.y > 72) {
    const int8_t index = targetAt(input.x, input.y);
    if (index >= 0) {
      handleTargetHit(static_cast<uint8_t>(index), now);
      if (phase_ != Phase::Playing) {
        return;
      }
    } else {
      combo_ = 0;
      if (difficulty_ != Difficulty::Easy) {
        score_ = score_ >= 20 ? score_ - 20 : 0;
      }
      drawHud(now);
    }
  }

  if (now - lastTimerDraw_ >= 90) {
    lastTimerDraw_ = now;
    drawHud(now);
    drawTimer(now);
  }
}
