#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

class RunnerGame {
 public:
  explicit RunnerGame(TFT_eSPI& display);

  void enter();
  void update(const InputFrame& input, uint32_t now);

 private:
  enum class State {
    Intro,
    Running,
    Paused,
    GameOver,
  };

  enum class Difficulty : uint8_t {
    Chill,
    Arcade,
    Turbo,
  };

  struct Obstacle {
    bool active;
    float x;
    int16_t width;
    int16_t height;
    uint8_t type;
    bool passed;
    bool drawn;
    int16_t drawnX;
  };

  struct Coin {
    bool active;
    float x;
    int16_t y;
    bool drawn;
    int16_t drawnX;
    int16_t drawnY;
  };

  TFT_eSPI& tft_;
  State state_ = State::Intro;
  Difficulty difficulty_ = Difficulty::Arcade;
  Obstacle obstacles_[5]{};
  Coin coins_[6]{};

  float playerY_ = 177.0f;
  float velocityY_ = 0.0f;
  float speed_ = 94.0f;
  float distance_ = 0.0f;
  uint32_t score_ = 0;
  uint32_t highScore_ = 0;
  uint16_t coinCount_ = 0;
  uint16_t dodged_ = 0;
  uint8_t level_ = 1;
  bool onGround_ = true;
  bool renderedPlayerValid_ = false;
  int16_t renderedPlayerY_ = 0;
  uint32_t nextObstacleAt_ = 0;
  uint32_t lastUpdateAt_ = 0;
  uint32_t lastRenderAt_ = 0;
  uint32_t renderedScore_ = 0xFFFFFFFFUL;
  uint16_t renderedCoins_ = 0xFFFF;
  uint8_t renderedLevel_ = 0xFF;

  static constexpr int16_t PLAYER_X = 47;
  static constexpr int16_t PLAYER_W = 24;
  static constexpr int16_t PLAYER_H = 28;
  static constexpr int16_t PLAY_TOP = 70;
  static constexpr int16_t DYNAMIC_TOP = 84;
  static constexpr int16_t GROUND_Y = 205;
  static constexpr uint8_t MAX_OBSTACLES = 5;
  static constexpr uint8_t MAX_COINS = 6;
  static constexpr Rect CHILL_BUTTON{34, 151, 76, 30};
  static constexpr Rect ARCADE_BUTTON{122, 151, 76, 30};
  static constexpr Rect TURBO_BUTTON{210, 151, 76, 30};
  static constexpr Rect START_BUTTON{86, 194, 148, 35};
  static constexpr Rect PAUSE_BUTTON{278, 7, 35, 27};
  static constexpr Rect AGAIN_BUTTON{89, 191, 142, 36};

  void drawIntro();
  void drawDifficultyButtons();
  void configureDifficulty();
  const char* difficultyLabel() const;
  void startGame(uint32_t now);
  void jump();
  void spawnObstacle(uint32_t now);
  void spawnCoin(float x, int16_t y);
  void updatePhysics(float dt);
  void updateObjects(float dt, uint32_t now);
  void handleCollisions();
  bool intersectsPlayer(float x, int16_t y, int16_t width, int16_t height) const;
  void endGame();

  void drawGameFrame();
  void drawStaticPlayfield();
  void drawHudFrame();
  void updateHud(bool force = false);
  void drawPauseButton();
  void renderDynamicFrame();
  void eraseRenderedObjects();
  void drawDynamicObjects();
  void clearDynamicRect(int16_t x, int16_t y, int16_t width, int16_t height);
  void drawPlayerAt(int16_t y);
  void drawObstacleAt(const Obstacle& obstacle, int16_t x);
  void drawCoinAt(int16_t x, int16_t y);
  void drawPauseOverlay();
  void drawGameOver();
};
