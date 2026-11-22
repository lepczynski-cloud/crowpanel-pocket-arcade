#include "App.h"

#include <Arduino.h>
#include <cstdio>

#include "AboutQr.h"
#include "AppConfig.h"
#include "Theme.h"
#include "Ui.h"

constexpr Rect App::HOME_BUTTON;
constexpr Rect App::ABOUT_LINK;

namespace {
constexpr Rect kGameCards[6] = {
    {6, 51, 98, 80}, {111, 51, 98, 80}, {216, 51, 98, 80},
    {6, 139, 98, 80}, {111, 139, 98, 80}, {216, 139, 98, 80},
};
constexpr const char* kTitles[6] = {
    "BURST", "SHIFT", "STAR POD", "LINE FOUR", "PAIR QUEST", "TILE SLIDE"};
constexpr const char* kSubtitles[6] = {
    "HUNT", "VAULT", "SPRINT", "1P / 2P", "1P / 2P", "3x3 / 4x4"};
constexpr uint16_t kColors[6] = {
    Theme::CYAN, Theme::PURPLE, Theme::PINK, Theme::YELLOW, Theme::LIME, Theme::CYAN};
constexpr uint32_t kHomeGlowIntervalMs = 750;
}  // namespace

App::App(TFT_eSPI& display)
    : tft_(display), reaction_(display), sliding_(display), runner_(display),
      lineFour_(display), pairs_(display), tiles_(display) {}

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
  tft_.drawString("SIX ORIGINAL GAMES  |  OFFLINE", 160, 172);
  tft_.fillRoundRect(58, 198, 204, 7, 4, Theme::PANEL);
  for (int16_t width = 4; width <= 200; width += 14) {
    tft_.fillRoundRect(60, 200, width, 3, 2, Theme::CYAN);
    delay(20);
  }
  delay(160);
}

void App::drawMiniIcon(uint8_t index, int16_t x, int16_t y, uint16_t color) {
  switch (index) {
    case 0: // Target rings
      tft_.drawCircle(x, y, 15, color);
      tft_.drawCircle(x, y, 10, color);
      tft_.fillCircle(x, y, 4, Theme::WHITE);
      tft_.fillCircle(x + 10, y - 10, 3, Theme::PINK);
      break;
    case 1: // Movable pieces
      tft_.fillRoundRect(x - 18, y - 13, 13, 26, 3, color);
      tft_.fillRoundRect(x - 1, y - 13, 25, 11, 3, Theme::CYAN);
      tft_.fillRoundRect(x - 1, y + 2, 18, 11, 3, Theme::PINK);
      break;
    case 2: // Pod in motion
      tft_.fillCircle(x - 3, y, 12, color);
      tft_.fillRoundRect(x - 12, y - 1, 16, 10, 4, Theme::BG_2);
      tft_.fillTriangle(x + 9, y + 11, x + 16, y - 3, x + 22, y + 11, Theme::CYAN);
      break;
    case 3: // Four aligned discs
      for (int8_t c = 0; c < 4; ++c) {
        tft_.fillCircle(x - 15 + c * 10, y + 5, 4, color);
        tft_.drawCircle(x - 15 + c * 10, y - 7, 4, Theme::GRID);
      }
      break;
    case 4: // Matching emblems
      tft_.fillRoundRect(x - 20, y - 13, 18, 26, 4, Theme::CYAN_DARK);
      tft_.fillRoundRect(x + 2, y - 13, 18, 26, 4, Theme::CYAN_DARK);
      tft_.fillCircle(x - 11, y, 5, color);
      tft_.fillCircle(x + 11, y, 5, color);
      break;
    default: // Number puzzle
      for (uint8_t i = 0; i < 3; ++i) {
        const int16_t xx = x - 17 + i * 13;
        tft_.fillRoundRect(xx, y - 10, 11, 19, 3, Theme::CYAN_DARK);
        tft_.setTextDatum(MC_DATUM);
        tft_.setTextFont(1);
        tft_.setTextColor(Theme::WHITE, Theme::CYAN_DARK);
        char digit[2] = {static_cast<char>('1' + i), '\0'};
        tft_.drawString(digit, xx + 5, y);
      }
      break;
  }
}

void App::drawGameCard(uint8_t index) {
  const Rect& r = kGameCards[index];
  const int16_t cx = r.x + r.w / 2;
  tft_.fillRoundRect(r.x, r.y, r.w, r.h, 9, Theme::PANEL);
  tft_.drawRoundRect(r.x, r.y, r.w, r.h, 9, Theme::BORDER);
  tft_.fillRoundRect(r.x + 5, r.y + 5, r.w - 10, 38, 7, Theme::BG_2);
  drawMiniIcon(index, cx, r.y + 24, kColors[index]);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::TEXT, Theme::PANEL);
  tft_.drawString(kTitles[index], cx, r.y + 54);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::PANEL);
  tft_.drawString(kSubtitles[index], cx, r.y + 69);
  if (index >= 3) {
    tft_.fillCircle(r.x + r.w - 10, r.y + 10, 3, Theme::LIME);
  }
}

void App::drawCardGlow(uint8_t index, bool active) {
  const Rect& r = kGameCards[index];
  tft_.drawRoundRect(r.x, r.y, r.w, r.h, 9, active ? kColors[index] : Theme::BORDER);
  if (active) {
    tft_.fillCircle(r.x + 8, r.y + 8, 2, Theme::WHITE);
  } else {
    tft_.fillCircle(r.x + 8, r.y + 8, 2, Theme::PANEL);
  }
}

void App::drawHome() {
  screen_ = Screen::Home;
  Ui::fillBackground(tft_);
  Ui::drawSparkles(tft_, 16, 48, 230);
  tft_.fillRect(0, 0, Config::SCREEN_WIDTH, 41, Theme::BG_2);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  tft_.drawString("POCKET ARCADE", 9, 19);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::CYAN, Theme::BG_2);
  char version[20];
  std::snprintf(version, sizeof(version), "v%s", Config::APP_VERSION);
  tft_.drawString(version, 311, 19);
  tft_.fillRect(0, 40, Config::SCREEN_WIDTH, 2, Theme::CYAN_DARK);
  for (uint8_t i = 0; i < 6; ++i) {
    drawGameCard(i);
    delay(25); // Staggered six-card reveal, only when opening the launcher.
  }
  glowCard_ = 0;
  drawCardGlow(glowCard_, true);
  lastHomeFrame_ = millis();
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("6 GAMES  |  OFFLINE", 8, 230);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("lepczynski.it", 312, 230);
  tft_.drawFastHLine(234, 236, 78, Theme::CYAN_DARK);
}

void App::drawAboutQr(int16_t x, int16_t y, uint8_t moduleSize) {
  constexpr uint8_t quietZone = 4;
  const int16_t modules = AboutQr::SIZE + quietZone * 2;
  const int16_t size = modules * moduleSize;
  tft_.fillRoundRect(x - 3, y - 3, size + 6, size + 6, 7, Theme::PANEL);
  tft_.fillRect(x, y, size, size, Theme::WHITE);
  for (uint8_t row = 0; row < AboutQr::SIZE; ++row) {
    for (uint8_t col = 0; col < AboutQr::SIZE; ++col) {
      if ((AboutQr::ROWS[row] & (1UL << col)) == 0) continue;
      tft_.fillRect(x + (col + quietZone) * moduleSize,
                    y + (row + quietZone) * moduleSize,
                    moduleSize, moduleSize, Theme::BLACK);
    }
  }
}

void App::drawAbout() {
  screen_ = Screen::About;
  Ui::fillBackground(tft_);
  Ui::drawSparkles(tft_, 10, 46, 180);
  Ui::drawTopBar(tft_, "ABOUT THE BUILDER");

  tft_.setTextDatum(ML_DATUM);
  tft_.setTextFont(2);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("SMALL SCREENS.", 10, 55);
  tft_.setTextFont(4);
  tft_.setTextColor(Theme::TEXT, Theme::BG);
  tft_.drawString("BIG IDEAS.", 10, 80);

  tft_.setTextFont(2);
  tft_.setTextColor(Theme::WHITE, Theme::BG);
  tft_.drawString("Hi, I'm Wojtek.", 10, 108);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::MUTED, Theme::BG);
  tft_.drawString("I turn tiny hardware", 10, 130);
  tft_.drawString("into useful tools,", 10, 143);
  tft_.drawString("unusual gadgets and", 10, 156);
  tft_.drawString("pocket-sized games.", 10, 169);

  drawAboutQr(214, 50, 3);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::CYAN, Theme::BG);
  tft_.drawString("SCAN FOR MORE", 263, 160);

  tft_.fillRect(0, 184, Config::SCREEN_WIDTH, 56, Theme::BG_2);
  tft_.fillRect(0, 184, Config::SCREEN_WIDTH, 2, Theme::CYAN_DARK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(1);
  tft_.setTextColor(Theme::CYAN, Theme::BG_2);
  tft_.drawString("SCAN TO SEE WHAT I BUILD NEXT", 160, 197);
  tft_.setTextColor(Theme::TEXT, Theme::BG_2);
  tft_.drawString("GITHUB  |  YOUTUBE  |  BLOG  |  LINKEDIN", 160, 213);
  tft_.setTextColor(Theme::MUTED, Theme::BG_2);
  char version[28];
  std::snprintf(version, sizeof(version), "POCKET ARCADE v%s", Config::APP_VERSION);
  tft_.drawString(version, 160, 230);
}

void App::updateHomeAnimation(uint32_t now) {
  if (static_cast<uint32_t>(now - lastHomeFrame_) < kHomeGlowIntervalMs) return;
  lastHomeFrame_ = now;
  drawCardGlow(glowCard_, false);
  glowCard_ = (glowCard_ + 1) % 6;
  drawCardGlow(glowCard_, true);
}

void App::open(Screen screen) {
  screen_ = screen;
  switch (screen_) {
    case Screen::Reaction: reaction_.enter(); break;
    case Screen::Sliding: sliding_.enter(); break;
    case Screen::Runner: runner_.enter(); break;
    case Screen::LineFour: lineFour_.enter(); break;
    case Screen::PairQuest: pairs_.enter(); break;
    case Screen::TileSlide: tiles_.enter(); break;
    case Screen::About: drawAbout(); break;
    case Screen::Home: drawHome(); break;
  }
}

void App::update(const InputFrame& input, uint32_t now) {
  if (screen_ != Screen::Home && input.pressed && contains(HOME_BUTTON, input.x, input.y)) {
    drawHome();
    return;
  }
  if (screen_ == Screen::Home) {
    updateHomeAnimation(now);
    if (!input.pressed) return;
    if (contains(ABOUT_LINK, input.x, input.y)) {
      open(Screen::About);
      return;
    }
    for (uint8_t i = 0; i < 6; ++i) {
      if (contains(kGameCards[i], input.x, input.y)) {
        // Screen enum is ordered to match the six menu cards.
        open(static_cast<Screen>(i + 1));
        return;
      }
    }
    return;
  }
  switch (screen_) {
    case Screen::Reaction: reaction_.update(input, now); break;
    case Screen::Sliding: sliding_.update(input, now); break;
    case Screen::Runner: runner_.update(input, now); break;
    case Screen::LineFour: lineFour_.update(input, now); break;
    case Screen::PairQuest: pairs_.update(input, now); break;
    case Screen::TileSlide: tiles_.update(input, now); break;
    case Screen::About: break;
    case Screen::Home: break;
  }
}
