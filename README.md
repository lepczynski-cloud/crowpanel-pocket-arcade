# CrowPanel Pocket Arcade

[![PlatformIO build](https://github.com/lepczynski-cloud/crowpanel-pocket-arcade/actions/workflows/platformio-build.yml/badge.svg)](https://github.com/lepczynski-cloud/crowpanel-pocket-arcade/actions/workflows/platformio-build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Three original offline touchscreen games for the Elecrow CrowPanel 2.8-inch ESP32 display.

The project is designed for the same 320 x 240 CrowPanel hardware configuration used by
CrowPanel Admin Pocket Toolkit, but it is a separate firmware focused entirely on games.
It does not require Wi-Fi, Bluetooth, an SD card or external game assets.

## Games

<p align="center">
  <a href="game V1[51].gif">
    <img src="game V1[51].gif" alt="ESP32 games demonstration" width="600">
  </a>
</p>

### Reflex Beacon

A reaction-speed challenge built for short sessions.

- 10 randomized rounds;
- target timing changes every round;
- smaller targets and shorter time windows as the run progresses;
- combo scoring for fast reactions;
- penalties for early taps and missed targets;
- average reaction time, fastest reaction and session high score.

### Shift Vault

A touch-friendly sliding-block logic puzzle.

- 5 x 5 board;
- select a block, then move it with large direction buttons;
- the cyan target block must reach the exit on the right;
- every puzzle is created from a solved configuration using legal reversible moves;
- no third-party level packs or copied puzzle layouts are included;
- restart and new-puzzle controls are available directly on the game screen.

### Circuit Worm

A fast grid arcade game controlled with swipe gestures.

- swipe up, down, left or right to steer;
- collect energy nodes to grow and score;
- movement speed increases during the run;
- new obstacles appear as the score rises;
- temporary bonus nodes add extra points;
- pause, restart and session high score support.

## User interface

The start screen contains three large game cards designed for the 320 x 240 resistive
touchscreen. All graphics are drawn at runtime with TFT primitives, so the firmware does
not need a filesystem full of PNG or JPEG assets.

The visual style uses a dark arcade background, cyan highlights, bright status colors and
large touch targets. Each game has a HOME button in the upper-left corner.

## Target hardware

Test target:

- Elecrow CrowPanel 2.8-inch ESP32 Miner/HMI display;
- ESP32-WROOM-32-N4;
- 320 x 240 ILI9341-compatible TFT in landscape orientation;
- XPT2046-compatible resistive touch controller;
- TFT_eSPI display and touch support;
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

## Project structure

```text
.
├── .github/workflows/platformio-build.yml
├── include/
│   ├── App.h
│   ├── AppConfig.h
│   ├── Theme.h
│   ├── TouchInput.h
│   ├── Types.h
│   ├── Ui.h
│   └── games/
│       ├── ReactionGame.h
│       ├── SlidingGame.h
│       └── WormGame.h
├── src/
│   ├── games/
│   │   ├── ReactionGame.cpp
│   │   ├── SlidingGame.cpp
│   │   └── WormGame.cpp
│   ├── App.cpp
│   ├── AppConfig.cpp
│   ├── TouchInput.cpp
│   ├── Ui.cpp
│   └── main.cpp
└── platformio.ini
```

## Build with CLion and PlatformIO

### 1. Install PlatformIO Core

macOS/Linux:

```bash
python3 -m pip install --user --upgrade platformio
```

Windows PowerShell:

```powershell
py -m pip install --user --upgrade platformio
```

If you already use PlatformIO with CLion, keep your existing installation.

### 2. Open the project

1. Clone or download this repository.
2. Open the repository directory in CLion.
3. Make sure PlatformIO support is enabled in your CLion setup.
4. Let PlatformIO resolve the ESP32 platform and TFT_eSPI dependency from `platformio.ini`.

### 3. Build

From the CLion terminal:

```bash
pio run
```

### 4. Upload

Connect the CrowPanel with a data-capable USB-C cable and run:

```bash
pio run --target upload
```

If automatic upload mode does not start, hold BOOT while upload begins and release it
when the terminal shows `Connecting...`.

### 5. Serial monitor

```bash
pio device monitor
```

The monitor speed is 115200 baud.

## Touch calibration

The calibration switch is in `include/AppConfig.h`. The default calibration values are
defined in `src/AppConfig.cpp` and are based on the working CrowPanel Admin Pocket Toolkit
configuration:

```cpp
uint16_t touchCalibration[5] = {189, 3416, 359, 3439, 1};
```

If touch input is offset on your unit:

1. Open `include/AppConfig.h`.
2. Set `RUN_TOUCH_CALIBRATION` to `true`.
3. Build and upload the firmware.
4. Follow the calibration points on the display.
5. Copy the five values printed to Serial Monitor into `src/AppConfig.cpp`.
6. Set `RUN_TOUCH_CALIBRATION` back to `false`.
7. Build and upload again.

## Memory and asset strategy

This firmware intentionally avoids large bitmap assets and full-screen frame buffers.
The interface is rendered from rectangles, circles, lines, text and small procedural
icons. Game state uses compact fixed-size arrays.

That approach keeps the project suitable for an ESP32-WROOM-32-N4 without requiring a
memory card or PSRAM.

## Original implementation and IP approach

The project uses familiar game genres, but the implementation is independent:

- all source code in this repository is written for this project;
- the UI and visual assets are generated specifically for this firmware;
- Shift Vault generates its own puzzle positions instead of shipping copied level data;
- no audio, sprites, logos or text from existing commercial games are included;
- the game names used in the firmware are project-specific labels rather than names of
  the commercial titles that inspired the general genres.

Generic descriptions such as reaction game, sliding-block puzzle and worm/snake-style
arcade game are used only to explain the gameplay category.

See [NOTICE](NOTICE) for the hardware-name and affiliation disclaimer.

## Development notes

- Framework: Arduino for ESP32.
- Build system: PlatformIO.
- Display library: TFT_eSPI 2.5.31.
- ESP32 PlatformIO platform: espressif32 6.6.0.
- Display SPI speed is intentionally kept at 16 MHz to match the known working hardware
  configuration from the Admin Pocket Toolkit project.
- Wi-Fi and Bluetooth are not initialized by this firmware.

## Related projects

- [Admin Toolkit](https://github.com/lepczynski-cloud/admin-toolkit) - browser-based admin utilities.
- [CrowPanel Admin Pocket Toolkit](https://github.com/lepczynski-cloud/crowpanel-Admin-Pocket-Toolkit) - offline ESP32 admin utilities for the same display family.

## License

MIT License. See [LICENSE](LICENSE).

Created by Wojciech Lepczynski.
