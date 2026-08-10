#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class ReactionGame {
 public:
  explicit ReactionGame(TFT_eSPI& display);

  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class Phase {
    Intro,
    Waiting,
    Active,
    Feedback,
    Results,
  };

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  uint8_t round_ = 0;
  uint8_t combo_ = 0;
  uint8_t falseStarts_ = 0;
  uint32_t score_ = 0;
  uint32_t bestScore_ = 0;
  uint32_t totalReaction_ = 0;
  uint16_t bestReaction_ = 0;
  uint16_t lastReaction_ = 0;
  uint32_t phaseUntil_ = 0;
  uint32_t targetShownAt_ = 0;
  uint32_t activeTimeout_ = 0;
  uint32_t lastProgressDraw_ = 0;
  int16_t targetX_ = 160;
  int16_t targetY_ = 135;
  int16_t targetRadius_ = 28;
  bool feedbackWasHit_ = false;
  bool feedbackWasTimeout_ = false;

  static constexpr uint8_t TOTAL_ROUNDS = 10;
  static constexpr Rect START_BUTTON{86, 177, 148, 42};
  static constexpr Rect AGAIN_BUTTON{86, 183, 148, 38};

  void drawIntro();
  void startSession(uint32_t now);
  void queueRound(uint32_t now);
  void activateTarget(uint32_t now);
  void finishRound(uint32_t now, bool hit, bool timeout);
  void finishSession();
  void drawHud();
  void drawWaiting();
  void drawTarget();
  void drawProgress(uint32_t now);
  void drawFeedback();
  void drawResults();
};
