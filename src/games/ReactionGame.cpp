#include "games/ReactionGame.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

#include "Theme.h"
#include "Ui.h"

constexpr uint8_t ReactionGame::TOTAL_ROUNDS;
constexpr Rect ReactionGame::START_BUTTON;
constexpr Rect ReactionGame::AGAIN_BUTTON;

ReactionGame::ReactionGame(TFT_eSPI& display) : tft_(display) {}

void ReactionGame::enter() {
  phase_ = Phase::Intro;
  drawIntro();
}

void ReactionGame::drawIntro() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "REFLEX BEACON");
  Ui::drawSparkles(tft_, 24, 45, 228);

  Ui::drawTargetIcon(tft_, 160, 92, Theme::CYAN);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("React when the pulse appears", 160, 132);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("10 rounds  |  early taps lose points", 160, 153);

  Ui::drawButton(tft_, START_BUTTON, "START", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void ReactionGame::startSession(uint32_t now) {
  round_ = 0;
  combo_ = 0;
  falseStarts_ = 0;
  score_ = 0;
  totalReaction_ = 0;
  bestReaction_ = 0;
  lastReaction_ = 0;
  queueRound(now);
}

void ReactionGame::queueRound(uint32_t now) {
  phase_ = Phase::Waiting;
  phaseUntil_ = now + static_cast<uint32_t>(random(750, 2200));
  drawWaiting();
}

void ReactionGame::activateTarget(uint32_t now) {
  phase_ = Phase::Active;
  targetShownAt_ = now;
  lastProgressDraw_ = 0;

  const int16_t difficulty = static_cast<int16_t>(round_ / 2);
  targetRadius_ = std::max<int16_t>(21, 31 - difficulty);
  activeTimeout_ = std::max<uint32_t>(850, 1450 - static_cast<uint32_t>(round_) * 55);

  targetX_ = static_cast<int16_t>(random(42 + targetRadius_, 278 - targetRadius_));
  targetY_ = static_cast<int16_t>(random(68 + targetRadius_, 205 - targetRadius_));

  tft_.fillRect(0, 40, 320, 200, Theme::BG);
  Ui::drawSparkles(tft_, 12, 45, 227);
  drawHud();
  drawTarget();
  drawProgress(now);
}

void ReactionGame::finishRound(uint32_t now, bool hit, bool timeout) {
  feedbackWasHit_ = hit;
  feedbackWasTimeout_ = timeout;
  phase_ = Phase::Feedback;
  phaseUntil_ = now + 620;

  if (hit) {
    ++round_;
  } else if (timeout) {
    ++round_;
    combo_ = 0;
  }

  drawFeedback();
}

void ReactionGame::finishSession() {
  phase_ = Phase::Results;
  if (score_ > bestScore_) {
    bestScore_ = score_;
  }
  drawResults();
}

void ReactionGame::drawHud() {
  tft_.fillRect(60, 0, 260, 38, Theme::BG_2);
  tft_.setTextFont(1);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);

  char buffer[40];
  const uint8_t shownRound = std::min<uint8_t>(TOTAL_ROUNDS, static_cast<uint8_t>(round_ + 1));
  std::snprintf(buffer, sizeof(buffer), "ROUND %u/%u", static_cast<unsigned>(shownRound),
                static_cast<unsigned>(TOTAL_ROUNDS));
  tft_.drawString(buffer, 70, 12);

  std::snprintf(buffer, sizeof(buffer), "SCORE %lu", static_cast<unsigned long>(score_));
  tft_.drawString(buffer, 70, 28);

  tft_.setTextDatum(MR_DATUM);
  std::snprintf(buffer, sizeof(buffer), "COMBO x%u", static_cast<unsigned>(combo_));
  tft_.setTextColor(combo_ >= 2 ? Theme::YELLOW : Theme::MUTED, Theme::BG_2);
  tft_.drawString(buffer, 308, 20);
}

void ReactionGame::drawWaiting() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "REFLEX BEACON");
  drawHud();
  Ui::drawSparkles(tft_, 18, 50, 226);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::PINK, Theme::BG);
  tft_.drawString("WAIT...", 160, 115);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("Do not tap before the target appears", 160, 150);
}

void ReactionGame::drawTarget() {
  tft_.fillCircle(targetX_, targetY_, targetRadius_ + 8, Theme::CYAN_DARK);
  tft_.drawCircle(targetX_, targetY_, targetRadius_ + 12, Theme::CYAN);
  tft_.drawCircle(targetX_, targetY_, targetRadius_ + 16, Theme::GRID);
  tft_.fillCircle(targetX_, targetY_, targetRadius_, Theme::CYAN);
  tft_.fillCircle(targetX_, targetY_, std::max<int16_t>(5, targetRadius_ / 3), Theme::WHITE);
}

void ReactionGame::drawProgress(uint32_t now) {
  const uint32_t elapsed = now - targetShownAt_;
  const uint32_t remaining = elapsed >= activeTimeout_ ? 0 : activeTimeout_ - elapsed;
  const int16_t width = static_cast<int16_t>((remaining * 288UL) / activeTimeout_);

  tft_.fillRoundRect(16, 220, 288, 8, 4, Theme::PANEL);
  if (width > 0) {
    const uint16_t color = remaining < activeTimeout_ / 3 ? Theme::PINK : Theme::LIME;
    tft_.fillRoundRect(16, 220, width, 8, 4, color);
  }
}

void ReactionGame::drawFeedback() {
  tft_.fillRect(0, 40, 320, 200, Theme::BG);
  Ui::drawSparkles(tft_, 18, 50, 226);
  drawHud();

  tft_.setTextDatum(MC_DATUM);
  if (feedbackWasHit_) {
    char buffer[32];
    tft_.setTextFont(4);
    tft_.setTextColor(lastReaction_ < 300 ? Theme::LIME : Theme::CYAN, Theme::BG);
    std::snprintf(buffer, sizeof(buffer), "%u ms", static_cast<unsigned>(lastReaction_));
    tft_.drawString(buffer, 160, 108);
    tft_.setTextFont(2);
    tft_.setTextColor(Theme::TEXT, Theme::BG);
    tft_.drawString(lastReaction_ < 300 ? "LIGHTNING HIT" : "NICE HIT", 160, 145);
  } else if (feedbackWasTimeout_) {
    tft_.setTextFont(4);
    tft_.setTextColor(Theme::RED, Theme::BG);
    tft_.drawString("MISSED", 160, 112);
    tft_.setTextFont(1);
    tft_.setTextColor(Theme::MUTED, Theme::BG);
    tft_.drawString("The pulse faded out", 160, 148);
  } else {
    tft_.setTextFont(4);
    tft_.setTextColor(Theme::PINK, Theme::BG);
    tft_.drawString("TOO SOON", 160, 112);
    tft_.setTextFont(1);
    tft_.setTextColor(Theme::MUTED, Theme::BG);
    tft_.drawString("-75 points  |  wait for the pulse", 160, 148);
  }
}

void ReactionGame::drawResults() {
  Ui::fillBackground(tft_);
  Ui::drawTopBar(tft_, "REFLEX BEACON");
  Ui::drawSparkles(tft_, 20, 45, 225);

  const uint32_t average = TOTAL_ROUNDS > 0 ? totalReaction_ / TOTAL_ROUNDS : 0;
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("RUN COMPLETE", 160, 74);

  char line[48];
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  std::snprintf(line, sizeof(line), "Score %lu", static_cast<unsigned long>(score_));
  tft_.drawString(line, 160, 111);
  std::snprintf(line, sizeof(line), "Average %lu ms   Best %u ms", static_cast<unsigned long>(average),
                static_cast<unsigned>(bestReaction_));
  tft_.drawString(line, 160, 137);

  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  std::snprintf(line, sizeof(line), "Early taps %u   Session high %lu",
                static_cast<unsigned>(falseStarts_), static_cast<unsigned long>(bestScore_));
  tft_.drawString(line, 160, 160);

  Ui::drawButton(tft_, AGAIN_BUTTON, "PLAY AGAIN", Theme::CYAN_DARK, Theme::WHITE, Theme::CYAN);
}

void ReactionGame::update(const InputFrame& input, uint32_t now) {
  if (phase_ == Phase::Intro) {
    if (input.pressed && contains(START_BUTTON, input.x, input.y)) {
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

  if (phase_ == Phase::Waiting) {
    if (input.pressed) {
      falseStarts_ = static_cast<uint8_t>(std::min<int>(99, falseStarts_ + 1));
      score_ = score_ >= 75 ? score_ - 75 : 0;
      combo_ = 0;
      feedbackWasHit_ = false;
      feedbackWasTimeout_ = false;
      phase_ = Phase::Feedback;
      phaseUntil_ = now + 700;
      drawFeedback();
      return;
    }
    if (static_cast<int32_t>(now - phaseUntil_) >= 0) {
      activateTarget(now);
    }
    return;
  }

  if (phase_ == Phase::Active) {
    if (input.pressed) {
      const int32_t dx = static_cast<int32_t>(input.x) - targetX_;
      const int32_t dy = static_cast<int32_t>(input.y) - targetY_;
      const int32_t hitRadius = targetRadius_ + 7;
      if (dx * dx + dy * dy <= hitRadius * hitRadius) {
        lastReaction_ = static_cast<uint16_t>(std::min<uint32_t>(65535, now - targetShownAt_));
        totalReaction_ += lastReaction_;
        if (bestReaction_ == 0 || lastReaction_ < bestReaction_) {
          bestReaction_ = lastReaction_;
        }
        if (lastReaction_ < 420) {
          combo_ = static_cast<uint8_t>(std::min<int>(9, combo_ + 1));
        } else {
          combo_ = 0;
        }
        const uint32_t speedPoints = lastReaction_ >= 650 ? 50 : 700 - lastReaction_;
        score_ += speedPoints + static_cast<uint32_t>(combo_) * 25U;
        finishRound(now, true, false);
      } else {
        score_ = score_ >= 25 ? score_ - 25 : 0;
        combo_ = 0;
      }
      return;
    }

    if (now - targetShownAt_ >= activeTimeout_) {
      totalReaction_ += activeTimeout_;
      finishRound(now, false, true);
      return;
    }

    if (now - lastProgressDraw_ >= 45) {
      lastProgressDraw_ = now;
      drawProgress(now);
    }
    return;
  }

  if (phase_ == Phase::Feedback && static_cast<int32_t>(now - phaseUntil_) >= 0) {
    if (round_ >= TOTAL_ROUNDS) {
      finishSession();
    } else {
      queueRound(now);
    }
  }
}
