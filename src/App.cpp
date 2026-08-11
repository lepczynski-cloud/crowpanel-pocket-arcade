#include "App.h"

#include <Arduino.h>
#include <cstdio>

#include "AppConfig.h"
#include "Theme.h"
#include "Ui.h"

constexpr Rect App::HOME_BUTTON;
constexpr Rect App::REACTION_CARD;
constexpr Rect App::SLIDING_CARD;
constexpr Rect App::RUNNER_CARD;

App::App(TFT_eSPI& display)
    : tft_(display), reaction_(display), sliding_(display), runner_(display) {}

void App::begin() {
  showSplash();
  drawHome();
}

void App::showSplash() {
  Ui::fillBackground(tft_);
  Ui::drawSparkles(tft_, 38, 8, 232);

  const int16_t cx = 160;
  const int16_t cy = 83;
  for (int16_t radius = 10; radius <= 42; radius += 8) {
    tft_.drawCircle(cx, cy, radius, radius % 16 == 10 ? Theme::CYAN : Theme::PURPLE);
    delay(38);
  }
  tft_.fillCircle(cx, cy, 13, Theme::CYAN);
  tft_.fillCircle(cx, cy, 6, Theme::WHITE);
  tft_.fillTriangle(cx + 42, cy + 11, cx + 55, cy - 7, cx + 61, cy + 15, Theme::PINK);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("POCKET ARCADE", 160, 145);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("ORIGINAL  |  TOUCH  |  OFFLINE", 160, 172);

  tft_.fillRoundRect(58, 198, 204, 7, 4, Theme::PANEL);
  for (int16_t width = 4; width <= 200; width += 14) {
    tft_.fillRoundRect(60, 200, width, 3, 2, Theme::CYAN);
    delay(20);
  }
  delay(180);
}

void App::drawGameCard(const Rect& rect, const char* title, const char* subtitle, uint8_t icon) {
  tft_.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 11, Theme::PANEL);
  tft_.drawRoundRect(rect.x, rect.y, rect.w, rect.h, 11, Theme::BORDER);
  tft_.fillRoundRect(rect.x + 7, rect.y + 8, rect.w - 14, 78, 9, Theme::BG_2);

  const int16_t cx = rect.x + rect.w / 2;
  const int16_t cy = rect.y + 47;
  if (icon == 0) {
    Ui::drawTargetIcon(tft_, cx, cy, Theme::CYAN);
  } else if (icon == 1) {
    Ui::drawBlocksIcon(tft_, cx - 4, cy, Theme::CYAN);
  } else {
    Ui::drawRunnerIcon(tft_, cx, cy, Theme::CYAN);
  }

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::PANEL);
  tft_.drawString(title, cx, rect.y + 104);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString(subtitle, cx, rect.y + 127);

  tft_.fillRoundRect(rect.x + 13, rect.y + 139, rect.w - 26, 13, 6, Theme::CYAN_DARK);
  tft_.setTextColor(Theme::CYAN, Theme::CYAN_DARK);
  tft_.drawString("PLAY", cx, rect.y + 146);
}

void App::drawHome() {
  screen_ = Screen::Home;
  Ui::fillBackground(tft_);
  Ui::drawSparkles(tft_, 22, 5, 232);

  tft_.fillRect(0, 0, Config::SCREEN_WIDTH, 43, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  tft_.drawString("POCKET ARCADE", 10, 20);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::CYAN, Theme::BG_2);
  char version[20];
  std::snprintf(version, sizeof(version), "v%s", Config::APP_VERSION);
  tft_.drawString(version, 309, 20);
  tft_.fillRect(0, 41, Config::SCREEN_WIDTH, 2, Theme::CYAN_DARK);

  drawGameCard(REACTION_CARD, "BURST", "Hunt", 0);
  drawGameCard(SLIDING_CARD, "SHIFT", "Vault", 1);
  drawGameCard(RUNNER_CARD, "STAR POD", "Sprint", 2);

  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("No Wi-Fi  |  No SD card  |  Touch only", 160, 228);
}

void App::open(Screen screen) {
  screen_ = screen;
  switch (screen_) {
    case Screen::Reaction:
      reaction_.enter();
      break;
    case Screen::Sliding:
      sliding_.enter();
      break;
    case Screen::Runner:
      runner_.enter();
      break;
    case Screen::Home:
      drawHome();
      break;
  }
}

void App::update(const InputFrame& input, uint32_t now) {
  if (screen_ != Screen::Home && input.pressed && contains(HOME_BUTTON, input.x, input.y)) {
    drawHome();
    return;
  }

  if (screen_ == Screen::Home) {
    if (!input.pressed) {
      return;
    }
    if (contains(REACTION_CARD, input.x, input.y)) {
      open(Screen::Reaction);
    } else if (contains(SLIDING_CARD, input.x, input.y)) {
      open(Screen::Sliding);
    } else if (contains(RUNNER_CARD, input.x, input.y)) {
      open(Screen::Runner);
    }
    return;
  }

  switch (screen_) {
    case Screen::Reaction:
      reaction_.update(input, now);
      break;
    case Screen::Sliding:
      sliding_.update(input, now);
      break;
    case Screen::Runner:
      runner_.update(input, now);
      break;
    case Screen::Home:
      break;
  }
}
