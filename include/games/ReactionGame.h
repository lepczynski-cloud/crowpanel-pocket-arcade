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

  struct Burst {
    bool active;
    bool hazard;
    int16_t x;
    int16_t y;
    uint16_t color;
    uint32_t expiresAt;
  };

  TFT_eSPI& tft_;
  Phase phase_ = Phase::Intro;
  Difficulty difficulty_ = Difficulty::Normal;
  Target targets_[5]{};
  Burst burst_{};

  uint32_t score_ = 0;
  uint32_t bestScore_ = 0;
  uint32_t sessionStartedAt_ = 0;
  uint32_t sessionEndsAt_ = 0;
  uint32_t nextSpawnAt_ = 0;
  uint32_t lastHudUpdateAt_ = 0;
  uint16_t hits_ = 0;
  uint16_t misses_ = 0;
  uint16_t minesHit_ = 0;
  uint8_t combo_ = 0;
  uint8_t lives_ = 3;
  uint8_t maxTargets_ = 3;
  uint8_t hazardChance_ = 28;
  uint16_t spawnInterval_ = 650;
  uint16_t targetLifetime_ = 1350;
  uint32_t sessionDuration_ = 35000;

  uint32_t renderedScore_ = 0xFFFFFFFFUL;
  uint8_t renderedCombo_ = 0xFF;
  uint8_t renderedLives_ = 0xFF;
  uint16_t renderedSeconds_ = 0xFFFF;
  int16_t renderedTimerWidth_ = -1;

  static constexpr uint8_t MAX_TARGETS = 5;
  static constexpr int16_t PLAY_X = 7;
  static constexpr int16_t PLAY_Y = 100;
  static constexpr int16_t PLAY_W = 306;
  static constexpr int16_t PLAY_H = 122;
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
  void drawPlayfieldFrame(uint32_t now);
  void drawLegend();
  void drawHudFrame(uint32_t now);
  void updateHud(uint32_t now, bool force = false);
  void updateTimer(uint32_t now, bool force = false);
  uint32_t remainingTime(uint32_t now) const;
  uint8_t sessionProgress(uint32_t now) const;

  void drawTarget(const Target& target);
  void drawBerry(const Target& target);
  void drawCitrus(const Target& target);
  void drawCrystal(const Target& target);
  void drawStar(const Target& target);
  void drawMine(const Target& target);
  uint16_t targetColor(const Target& target) const;

  void spawnTarget(uint32_t now, bool forceGood = false);
  bool positionAvailable(int16_t x, int16_t y, int16_t radius) const;
  uint8_t activeTargetCount() const;
  uint8_t activeGoodTargetCount() const;
  bool expireTargets(uint32_t now);
  int8_t targetAt(uint16_t x, uint16_t y) const;
  void handleTargetHit(uint8_t index, uint32_t now);

  Rect targetBounds(const Target& target) const;
  Rect burstBounds() const;
  bool intersects(const Rect& a, const Rect& b) const;
  void clearPlayRect(const Rect& rect);
  void redrawTargetsInRect(const Rect& rect);
  void eraseTarget(const Target& target);
  void startBurst(const Target& target, uint32_t now);
  void drawBurst();
  void clearBurst();
  void expireBurst(uint32_t now);

  const char* difficultyLabel() const;
  void drawResults();
};
