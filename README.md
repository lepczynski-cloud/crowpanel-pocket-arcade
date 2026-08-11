# CrowPanel Pocket Arcade

[![PlatformIO build](https://github.com/lepczynski-cloud/crowpanel-pocket-arcade/actions/workflows/platformio-build.yml/badge.svg)](https://github.com/lepczynski-cloud/crowpanel-pocket-arcade/actions/workflows/platformio-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.2.0-37dfff.svg)](CHANGELOG.md)

CrowPanel Pocket Arcade is an original offline mini-arcade for the Elecrow CrowPanel 2.8-inch ESP32 display. It provides three touch-first games, a compact launcher and procedural graphics without Wi-Fi, Bluetooth, PSRAM, an SD card or external game assets.

## Gameplay comparison

| Version 1 | Version 2 |
| --- | --- |
| <img src="docs/game_v1.gif" alt="CrowPanel Pocket Arcade version 1 gameplay" width="390"> | <img src="docs/game_v2.gif" alt="CrowPanel Pocket Arcade version 2 gameplay" width="390"> |

## Video

[Watch the full gameplay video on YouTube](https://www.youtube.com/watch?v=YOUR_VIDEO_ID)

## Games

### Burst Hunt

A clear, fast target game built around short sessions and accurate taps.

- tap colorful objects surrounded by a bright green ring;
- avoid black mines marked with a large red-and-white `X`;
- use the permanent on-screen legend to identify safe targets and hazards;
- build combos for higher scores;
- lose points for empty taps and lose lives for hitting hazards;
- play Easy, Normal or Hard sessions with different pacing and target sizes;
- enjoy local hit bursts without full-screen flashes or continuous screen clearing.

### Shift Vault

A generated 6 x 6 spatial puzzle with free four-direction movement.

- move the gold `KEY` piece to the bright green exit on the right;
- swipe any piece up, down, left or right when there is free space;
- move dominoes, long bars, squares and several L-shaped pieces;
- pieces never overlap and cannot leave the board;
- choose Easy, Normal or Hard generation profiles;
- restart the current puzzle or generate a new one from the game screen;
- each board is created from a reachable state using legal reversible moves.

### Star Pod Sprint

A minimal one-touch endless runner with an original pod character.

- tap once to jump;
- clear single, double and triple spike groups;
- avoid geometric barriers;
- collect gold coins for bonus points;
- increase the score by running farther and passing obstacles;
- choose Chill, Arcade or Turbo speed profiles;
- play on a stable, minimal background rendered with small dirty rectangles instead of full-screen redraws.

## Controls

- Select a game from the three cards on the launcher.
- Use `HOME` in the upper-left corner to return to the launcher.
- Burst Hunt uses single taps.
- Shift Vault uses swipe gestures in all four directions.
- Star Pod Sprint uses a single tap to jump and includes a pause button.

## Hardware target

The supplied PlatformIO environment targets:

- Elecrow CrowPanel 2.8-inch ESP32 display;
- ESP32-WROOM-32-N4;
- 320 x 240 ILI9341-compatible TFT in landscape orientation;
- XPT2046-compatible resistive touch controller;
- TFT_eSPI 2.5.31;
- no PSRAM requirement;
- no SD card requirement.

Configured pins:

| Function | GPIO |
| --- | ---: |
| TFT MISO | 4 |
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| Backlight | 27 |
| Touch CS | 33 |

## Build and upload

Install PlatformIO Core:

```bash
python3 -m pip install --user --upgrade platformio
```

Clone the repository:

```bash
git clone https://github.com/lepczynski-cloud/crowpanel-pocket-arcade.git
cd crowpanel-pocket-arcade
```

Build the firmware:

```bash
pio run -e crowpanel_28
```

Upload it to the connected CrowPanel:

```bash
pio run -e crowpanel_28 --target upload
```

Open the serial monitor:

```bash
pio device monitor --baud 115200
```

The project can be opened directly in CLion with PlatformIO support enabled.

## Touch calibration

The default calibration values are stored in `src/AppConfig.cpp`:

```cpp
uint16_t touchCalibration[5] = {189, 3416, 359, 3439, 1};
```

Calibration mode is controlled by `RUN_TOUCH_CALIBRATION` in `include/AppConfig.h`. When enabled, the firmware displays calibration points and prints the resulting five values to the serial monitor.

## Rendering and memory strategy

The firmware draws its interface with TFT primitives and fixed-size game-state arrays. It does not load bitmap packs or create a full-screen framebuffer.

Burst Hunt keeps the playfield static and updates only target-sized regions, HUD values and the timer bar. Star Pod Sprint keeps the sky and ground static and erases only the previous bounds of the player, obstacles and coins before drawing their new positions. This reduces visible flashing and keeps memory use suitable for the ESP32-WROOM-32-N4.

## Original implementation and third-party rights

The source code, launcher, game names, procedural graphics, object designs and generated puzzle layouts in this repository were created for this project.

Star Pod Sprint uses the general idea of a one-button side-scrolling runner, but it has an original pod character, geometric hazards, coin graphics, scoring rules and independent source code. It does not include browser-dinosaur artwork, animation, audio, level data, branding or source code.

Shift Vault does not ship copied commercial level packs. Its boards are generated at runtime from legal reversible moves. Burst Hunt uses original procedural targets and hazard graphics.

`Elecrow` and `CrowPanel` are used only to identify compatible hardware. See [NOTICE](NOTICE) for the affiliation and trademark disclaimer.

## Project structure

```text
.
├── .github/workflows/platformio-build.yml
├── docs/
│   ├── game_v1.gif
│   └── game_v2.gif
├── include/
│   ├── games/
│   │   ├── ReactionGame.h
│   │   ├── RunnerGame.h
│   │   └── SlidingGame.h
│   ├── App.h
│   ├── AppConfig.h
│   ├── Theme.h
│   ├── TouchInput.h
│   ├── Types.h
│   └── Ui.h
├── src/
│   ├── games/
│   │   ├── ReactionGame.cpp
│   │   ├── RunnerGame.cpp
│   │   └── SlidingGame.cpp
│   ├── App.cpp
│   ├── AppConfig.cpp
│   ├── TouchInput.cpp
│   ├── Ui.cpp
│   └── main.cpp
├── CHANGELOG.md
├── LICENSE
├── NOTICE
├── README.md
└── platformio.ini
```

The two GIF files are optional repository media and are not required by the firmware.

## Related project

- [Admin Toolkit](https://github.com/lepczynski-cloud/admin-toolkit)

## License

Released under the [MIT License](LICENSE).

Created by Wojciech Lepczynski.
