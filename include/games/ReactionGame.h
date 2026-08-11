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
    Playing,
    Results,
  };

  enum class Difficulty : uint8_t {
    Easy,
    Normal,
    Hard,
  };

  enum class ObjectKind : uint8_t {
    Berry,
    Citrus,
    Crystal,
    Star,
    Mine,
  };

  struct Target {
    bool active;
    bool hazard;
    ObjectKind kind;
    int16_t x;
    int16_t y;
    int16_t radius;
    uint16_t value;
    uint32_t expiresAt;
  };

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  Difficulty difficulty_ = Difficulty::Normal;
  Target targets_[5]{};

  uint32_t score_ = 0;
  uint32_t bestScore_ = 0;
  uint32_t sessionStartedAt_ = 0;
  uint32_t sessionEndsAt_ = 0;
  uint32_t nextSpawnAt_ = 0;
  uint32_t lastTimerDraw_ = 0;
  uint16_t hits_ = 0;
  uint16_t misses_ = 0;
  uint16_t minesHit_ = 0;
  uint8_t combo_ = 0;
  uint8_t lives_ = 3;
  uint8_t maxTargets_ = 3;
  uint8_t hazardChance_ = 28;
  uint16_t spawnInterval_ = 620;
  uint16_t targetLifetime_ = 1300;
  uint32_t sessionDuration_ = 35000;

  static constexpr uint8_t MAX_TARGETS = 5;
  static constexpr Rect EASY_BUTTON{34, 151, 76, 30};
  static constexpr Rect NORMAL_BUTTON{122, 151, 76, 30};
  static constexpr Rect HARD_BUTTON{210, 151, 76, 30};
  static constexpr Rect START_BUTTON{86, 194, 148, 35};
  static constexpr Rect AGAIN_BUTTON{86, 193, 148, 36};

  void drawIntro();
  void drawDifficultyButtons();
  void configureDifficulty();
  void startSession(uint32_t now);
  void finishSession();
  void drawPlayfield(uint32_t now);
  void drawHud(uint32_t now);
  void drawTimer(uint32_t now);
  uint32_t remainingTime(uint32_t now) const;
  uint8_t sessionProgress(uint32_t now) const;
  void drawTarget(const Target& target);
  void drawBerry(const Target& target);
  void drawCitrus(const Target& target);
  void drawCrystal(const Target& target);
  void drawStar(const Target& target);
  void drawMine(const Target& target);
  void spawnTarget(uint32_t now, bool forceGood = false);
  bool positionAvailable(int16_t x, int16_t y, int16_t radius) const;
  uint8_t activeTargetCount() const;
  uint8_t activeGoodTargetCount() const;
  bool expireTargets(uint32_t now);
  int8_t targetAt(uint16_t x, uint16_t y) const;
  void handleTargetHit(uint8_t index, uint32_t now);
  void animateBurst(int16_t x, int16_t y, uint16_t color, bool hazard);
  uint16_t targetColor(const Target& target) const;
  const char* difficultyLabel() const;
  void drawResults();
};
