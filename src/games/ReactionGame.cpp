#include "games/ReactionGame.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t ReactionGame::MAX_TARGETS;
constexpr int16_t ReactionGame::PLAY_X;
constexpr int16_t ReactionGame::PLAY_Y;
constexpr int16_t ReactionGame::PLAY_W;
constexpr int16_t ReactionGame::PLAY_H;
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

  Target berry{true, false, ObjectKind::Berry, 91, 86, 20, 90, 0};
  Target citrus{true, false, ObjectKind::Citrus, 160, 84, 20, 100, 0};
  Target mine{true, true, ObjectKind::Mine, 232, 86, 20, 0, 0};
  drawBerry(berry);
  tft_.drawCircle(berry.x, berry.y, berry.radius + 3, Theme::LIME);
  drawCitrus(citrus);
  tft_.drawCircle(citrus.x, citrus.y, citrus.radius + 3, Theme::LIME);
  drawMine(mine);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("Tap bright targets. Avoid the red X.", 160, 122);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Build combos before the timer runs out", 160, 140);

  drawDifficultyButtons();
  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void ReactionGame::configureDifficulty() {
  switch (difficulty_) {
    case Difficulty::Easy:
      sessionDuration_ = 40000;
      spawnInterval_ = 820;
      targetLifetime_ = 1750;
      maxTargets_ = 2;
      hazardChance_ = 16;
      break;
    case Difficulty::Hard:
      sessionDuration_ = 30000;
      spawnInterval_ = 470;
      targetLifetime_ = 980;
      maxTargets_ = 4;
      hazardChance_ = 38;
      break;
    case Difficulty::Normal:
    default:
      sessionDuration_ = 35000;
      spawnInterval_ = 650;
      targetLifetime_ = 1350;
      maxTargets_ = 3;
      hazardChance_ = 27;
      break;
  }
}

void ReactionGame::startSession(uint32_t now) {
  configureDifficulty();
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    targets_[i].active = false;
  }
  burst_.active = false;

  score_ = 0;
  hits_ = 0;
  misses_ = 0;
  minesHit_ = 0;
  combo_ = 0;
  lives_ = 3;
  sessionStartedAt_ = now;
  sessionEndsAt_ = now + sessionDuration_;
  nextSpawnAt_ = now + 500;
  lastHudUpdateAt_ = 0;
  renderedScore_ = 0xFFFFFFFFUL;
  renderedCombo_ = 0xFF;
  renderedLives_ = 0xFF;
  renderedSeconds_ = 0xFFFF;
  renderedTimerWidth_ = -1;
  phase_ = Phase::Playing;

  drawPlayfieldFrame(now);
  spawnTarget(now, true);
  if (difficulty_ != Difficulty::Easy) {
    spawnTarget(now, true);
  }
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
    const int32_t spacing = radius + targets_[i].radius + 14;
    if (dx * dx + dy * dy < spacing * spacing) {
      return false;
    }
  }

  if (burst_.active) {
    const int32_t dx = static_cast<int32_t>(x) - burst_.x;
    const int32_t dy = static_cast<int32_t>(y) - burst_.y;
    const int32_t spacing = radius + 24;
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
      std::min<int>(48, hazardChance_ + progress / 12));
  const bool hazard = !forceGood && activeGoodTargetCount() > 0 &&
                      random(0, 100) < dynamicHazardChance;

  int16_t radius = 18;
  if (difficulty_ == Difficulty::Easy) {
    radius = static_cast<int16_t>(random(20, 25));
  } else if (difficulty_ == Difficulty::Hard) {
    radius = static_cast<int16_t>(random(14, 19));
  } else {
    radius = static_cast<int16_t>(random(17, 22));
  }
  if (hazard) {
    radius = std::max<int16_t>(19, radius + 2);
  }

  const int16_t visualMargin = hazard ? 12 : 8;
  int16_t x = 160;
  int16_t y = 160;
  bool positionFound = false;
  for (uint8_t attempt = 0; attempt < 80; ++attempt) {
    const int16_t candidateX = static_cast<int16_t>(
        random(PLAY_X + radius + visualMargin,
               PLAY_X + PLAY_W - radius - visualMargin));
    const int16_t candidateY = static_cast<int16_t>(
        random(PLAY_Y + radius + visualMargin,
               PLAY_Y + PLAY_H - radius - visualMargin));
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

  const int32_t lifetimeJitter = static_cast<int32_t>(random(-120, 230));
  const int32_t paceReduction = difficulty_ == Difficulty::Easy ? progress : progress * 2;
  target.expiresAt = now + static_cast<uint32_t>(
                               std::max<int32_t>(620, targetLifetime_ + lifetimeJitter - paceReduction));
  drawTarget(target);
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
                    target.y - target.radius - 5, target.x + 8,
                    target.y - target.radius + 3, Theme::LIME);
}

void ReactionGame::drawCitrus(const Target& target) {
  tft_.fillCircle(target.x, target.y, target.radius, Theme::ORANGE);
  tft_.drawCircle(target.x, target.y, target.radius - 4, Theme::YELLOW);
  tft_.fillCircle(target.x, target.y, std::max<int16_t>(3, target.radius / 5), Theme::YELLOW);
  tft_.drawFastHLine(target.x - target.radius + 5, target.y,
                     target.radius * 2 - 10, Theme::YELLOW);
  tft_.drawFastVLine(target.x, target.y - target.radius + 5,
                     target.radius * 2 - 10, Theme::YELLOW);
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
  tft_.fillCircle(target.x, target.y, r + 5, Theme::RED);
  tft_.fillCircle(target.x, target.y, r + 1, Theme::BLACK);
  tft_.drawCircle(target.x, target.y, r + 4, Theme::WHITE);

  const int16_t arm = std::max<int16_t>(8, r - 5);
  for (int8_t offset = -2; offset <= 2; ++offset) {
    tft_.drawLine(target.x - arm, target.y - arm + offset,
                  target.x + arm, target.y + arm + offset, Theme::WHITE);
    tft_.drawLine(target.x + arm, target.y - arm + offset,
                  target.x - arm, target.y + arm + offset, Theme::WHITE);
  }

  tft_.fillTriangle(target.x, target.y - r - 9, target.x - 5, target.y - r - 2,
                    target.x + 5, target.y - r - 2, Theme::RED);
  tft_.fillTriangle(target.x, target.y + r + 9, target.x - 5, target.y + r + 2,
                    target.x + 5, target.y + r + 2, Theme::RED);
  tft_.fillTriangle(target.x - r - 9, target.y, target.x - r - 2, target.y - 5,
                    target.x - r - 2, target.y + 5, Theme::RED);
  tft_.fillTriangle(target.x + r + 9, target.y, target.x + r + 2, target.y - 5,
                    target.x + r + 2, target.y + 5, Theme::RED);
}

void ReactionGame::drawTarget(const Target& target) {
  if (!target.active) {
    return;
  }

  if (!target.hazard) {
    tft_.drawCircle(target.x, target.y, target.radius + 4, Theme::LIME);
    tft_.drawCircle(target.x, target.y, target.radius + 2, Theme::WHITE);
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

void ReactionGame::drawLegend() {
  tft_.fillRoundRect(8, 72, 146, 24, 7, Theme::CYAN_DARK);
  tft_.drawRoundRect(8, 72, 146, 24, 7, Theme::LIME);
  tft_.fillCircle(22, 84, 7, Theme::LIME);
  tft_.fillCircle(22, 84, 3, Theme::WHITE);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::WHITE, Theme::CYAN_DARK);
  tft_.drawString("TAP BRIGHT TARGETS", 35, 84);

  tft_.fillRoundRect(166, 72, 146, 24, 7, Theme::PANEL);
  tft_.drawRoundRect(166, 72, 146, 24, 7, Theme::RED);
  tft_.fillCircle(181, 84, 8, Theme::RED);
  tft_.fillCircle(181, 84, 5, Theme::BLACK);
  tft_.drawLine(177, 80, 185, 88, Theme::WHITE);
  tft_.drawLine(185, 80, 177, 88, Theme::WHITE);
  tft_.setTextColor(Theme::WHITE, Theme::PANEL);
  tft_.drawString("AVOID THE RED X", 195, 84);
}

void ReactionGame::drawHudFrame(uint32_t now) {
  tft_.fillRect(0, 40, 320, 29, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  tft_.drawString("SCORE", 8, 48);
  tft_.drawString("COMBO", 90, 48);
  tft_.drawString("LIVES", 171, 48);
  tft_.drawString("TIME", 262, 48);
  updateHud(now, true);
}

void ReactionGame::updateHud(uint32_t now, bool force) {
  char text[24];

  if (force || renderedScore_ != score_) {
    tft_.fillRect(8, 54, 72, 14, Theme::BG_2);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::TEXT, Theme::BG_2);
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
    tft_.drawString(text, 8, 62);
    renderedScore_ = score_;
  }

  if (force || renderedCombo_ != combo_) {
    tft_.fillRect(90, 54, 66, 14, Theme::BG_2);
    tft_.setTextDatum(ML_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(combo_ >= 4 ? Theme::YELLOW : Theme::TEXT, Theme::BG_2);
    std::snprintf(text, sizeof(text), "x%u", static_cast<unsigned>(combo_));
    tft_.drawString(text, 90, 62);
    renderedCombo_ = combo_;
  }

  if (force || renderedLives_ != lives_) {
    tft_.fillRect(171, 54, 72, 14, Theme::BG_2);
    for (uint8_t i = 0; i < 3; ++i) {
      const uint16_t color = i < lives_ ? Theme::PINK : Theme::PANEL;
      tft_.fillCircle(181 + i * 19, 61, 6, color);
      tft_.drawCircle(181 + i * 19, 61, 6, i < lives_ ? Theme::WHITE : Theme::BORDER);
    }
    renderedLives_ = lives_;
  }

  const uint16_t seconds = static_cast<uint16_t>((remainingTime(now) + 999UL) / 1000UL);
  if (force || renderedSeconds_ != seconds) {
    tft_.fillRect(260, 54, 58, 14, Theme::BG_2);
    tft_.setTextDatum(MR_DATUM);
    tft_.setTextFont(2);
    tft_.setTextColor(seconds <= 5 ? Theme::PINK : Theme::CYAN, Theme::BG_2);
    std::snprintf(text, sizeof(text), "%us", static_cast<unsigned>(seconds));
    tft_.drawString(text, 311, 62);
    renderedSeconds_ = seconds;
  }

  updateTimer(now, force);
}

void ReactionGame::updateTimer(uint32_t now, bool force) {
  const uint32_t remaining = remainingTime(now);
  const int16_t width = static_cast<int16_t>((remaining * 296UL) / sessionDuration_);
  if (!force && std::abs(width - renderedTimerWidth_) < 2) {
    return;
  }

  tft_.fillRoundRect(12, 229, 296, 7, 4, Theme::PANEL);
  if (width > 0) {
    const uint16_t color = remaining < 6000 ? Theme::PINK : Theme::LIME;
    tft_.fillRoundRect(12, 229, width, 7, 4, color);
  }
  renderedTimerWidth_ = width;
}

void ReactionGame::drawPlayfieldFrame(uint32_t now) {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "BURST HUNT");
  drawHudFrame(now);
  drawLegend();
  tft_.fillRoundRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 10, Theme::BG_2);
  tft_.drawRoundRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 10, Theme::BORDER);
}

Rect ReactionGame::targetBounds(const Target& target) const {
  const int16_t margin = target.hazard ? 11 : 7;
  const int16_t radius = target.radius + margin;
  return Rect{static_cast<int16_t>(target.x - radius),
              static_cast<int16_t>(target.y - radius),
              static_cast<int16_t>(radius * 2 + 1),
              static_cast<int16_t>(radius * 2 + 1)};
}

Rect ReactionGame::burstBounds() const {
  return Rect{static_cast<int16_t>(burst_.x - 24), static_cast<int16_t>(burst_.y - 24), 49, 49};
}

bool ReactionGame::intersects(const Rect& a, const Rect& b) const {
  return a.x < b.x + b.w && a.x + a.w > b.x &&
         a.y < b.y + b.h && a.y + a.h > b.y;
}

void ReactionGame::clearPlayRect(const Rect& rect) {
  const int16_t left = std::max<int16_t>(PLAY_X + 1, rect.x);
  const int16_t top = std::max<int16_t>(PLAY_Y + 1, rect.y);
  const int16_t right = std::min<int16_t>(PLAY_X + PLAY_W - 1, rect.x + rect.w);
  const int16_t bottom = std::min<int16_t>(PLAY_Y + PLAY_H - 1, rect.y + rect.h);
  if (right <= left || bottom <= top) {
    return;
  }
  tft_.fillRect(left, top, right - left, bottom - top, Theme::BG_2);
}

void ReactionGame::redrawTargetsInRect(const Rect& rect) {
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    if (targets_[i].active && intersects(targetBounds(targets_[i]), rect)) {
      drawTarget(targets_[i]);
    }
  }
}

void ReactionGame::eraseTarget(const Target& target) {
  const Rect area = targetBounds(target);
  clearPlayRect(area);
  redrawTargetsInRect(area);
}

void ReactionGame::startBurst(const Target& target, uint32_t now) {
  if (burst_.active) {
    clearBurst();
  }
  burst_.active = true;
  burst_.hazard = target.hazard;
  burst_.x = target.x;
  burst_.y = target.y;
  burst_.color = target.hazard ? Theme::RED : targetColor(target);
  burst_.expiresAt = now + 130;
  drawBurst();
}

void ReactionGame::drawBurst() {
  if (!burst_.active) {
    return;
  }

  static const int8_t directions[8][2] = {
      {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1},
  };
  tft_.drawCircle(burst_.x, burst_.y, 12, burst_.color);
  for (uint8_t i = 0; i < 8; ++i) {
    const int16_t x1 = burst_.x + directions[i][0] * 9;
    const int16_t y1 = burst_.y + directions[i][1] * 9;
    const int16_t x2 = burst_.x + directions[i][0] * 20;
    const int16_t y2 = burst_.y + directions[i][1] * 20;
    tft_.drawLine(x1, y1, x2, y2, i % 2 == 0 ? Theme::WHITE : burst_.color);
  }
  if (burst_.hazard) {
    tft_.drawLine(burst_.x - 15, burst_.y - 15, burst_.x + 15, burst_.y + 15, Theme::RED);
    tft_.drawLine(burst_.x + 15, burst_.y - 15, burst_.x - 15, burst_.y + 15, Theme::RED);
  }
}

void ReactionGame::clearBurst() {
  if (!burst_.active) {
    return;
  }
  const Rect area = burstBounds();
  burst_.active = false;
  clearPlayRect(area);
  redrawTargetsInRect(area);
}

void ReactionGame::expireBurst(uint32_t now) {
  if (burst_.active && static_cast<int32_t>(now - burst_.expiresAt) >= 0) {
    clearBurst();
  }
}

bool ReactionGame::expireTargets(uint32_t now) {
  bool scoreChanged = false;
  for (uint8_t i = 0; i < MAX_TARGETS; ++i) {
    Target& target = targets_[i];
    if (!target.active || static_cast<int32_t>(now - target.expiresAt) < 0) {
      continue;
    }

    const Target expired = target;
    target.active = false;
    eraseTarget(expired);
    if (!expired.hazard) {
      ++misses_;
      combo_ = 0;
      score_ = score_ >= 10 ? score_ - 10 : 0;
      if (difficulty_ == Difficulty::Hard && misses_ % 4 == 0 && lives_ > 0) {
        --lives_;
      }
      scoreChanged = true;
    }
  }
  return scoreChanged;
}

int8_t ReactionGame::targetAt(uint16_t x, uint16_t y) const {
  for (int8_t i = static_cast<int8_t>(MAX_TARGETS) - 1; i >= 0; --i) {
    const Target& target = targets_[i];
    if (!target.active) {
      continue;
    }
    const int32_t dx = static_cast<int32_t>(x) - target.x;
    const int32_t dy = static_cast<int32_t>(y) - target.y;
    const int32_t hitRadius = target.radius + 7;
    if (dx * dx + dy * dy <= hitRadius * hitRadius) {
      return i;
    }
  }
  return -1;
}

void ReactionGame::handleTargetHit(uint8_t index, uint32_t now) {
  const Target target = targets_[index];
  targets_[index].active = false;
  eraseTarget(target);

  if (target.hazard) {
    ++minesHit_;
    combo_ = 0;
    score_ = score_ >= 120 ? score_ - 120 : 0;
    if (lives_ > 0) {
      --lives_;
    }
  } else {
    ++hits_;
    combo_ = static_cast<uint8_t>(std::min<int>(12, combo_ + 1));
    score_ += target.value + static_cast<uint32_t>(combo_) * 18U;
  }

  startBurst(target, now);
  updateHud(now, true);

  if (lives_ == 0) {
    finishSession();
  }
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
  std::snprintf(text, sizeof(text), "Red X hits %u   Difficulty %s",
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

  expireBurst(now);
  const bool hudChanged = expireTargets(now);
  if (lives_ == 0) {
    finishSession();
    return;
  }
  if (hudChanged) {
    updateHud(now, true);
  }

  const uint8_t progress = sessionProgress(now);
  const uint8_t targetLimit = static_cast<uint8_t>(
      std::min<int>(MAX_TARGETS, maxTargets_ + (progress >= 60 ? 1 : 0)));
  if (static_cast<int32_t>(now - nextSpawnAt_) >= 0 && activeTargetCount() < targetLimit) {
    spawnTarget(now);
    const uint16_t reduction = static_cast<uint16_t>((spawnInterval_ * progress) / 360U);
    const uint16_t pacedInterval = std::max<uint16_t>(320, spawnInterval_ - reduction);
    nextSpawnAt_ = now + pacedInterval + static_cast<uint32_t>(random(0, pacedInterval / 3 + 1));
  }

  if (input.pressed && input.y >= PLAY_Y && input.y < PLAY_Y + PLAY_H) {
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
      updateHud(now, true);
    }
  }

  if (now - lastHudUpdateAt_ >= 120) {
    lastHudUpdateAt_ = now;
    updateHud(now, false);
  }
}
