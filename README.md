# CrowPanel Pocket Arcade

**Six original touchscreen games. One tiny offline ESP32 arcade.**

[**Play with it, watch the video and install firmware from your browser**](https://pocket-arcade.lepczynski.it/) · [YouTube](https://www.youtube.com/watch?v=BkuglAy6qOQ) · [Blog](https://lepczynski.it/) · [More projects](https://github.com/lepczynski-cloud)

[![Version](https://img.shields.io/badge/firmware-1.3.0-48e7ca.svg)](CHANGELOG.md) [![License: MIT](https://img.shields.io/badge/license-MIT-9f8aff.svg)](LICENSE)

<p align="center"><img src="web/assets/arcade-v2.gif" alt="Real CrowPanel Pocket Arcade filmed on the physical device with firmware v1.2" width="320"></p>

The Elecrow **CrowPanel 2.8-inch ESP32** runs six games, an animated touch-first launcher and original procedural graphics. No Wi-Fi, Bluetooth, PSRAM, SD card, downloaded game assets, account or background service is required to play.

The GIFs and photos show real hardware running earlier firmware versions (up to v1.2), not rendered mockups. The three games introduced in v1.3 are not shown in these recordings.

## The six games

| Game | How to play |
| --- | --- |
| **Burst Hunt** | Tap safe targets, avoid mines and build combos. Easy, Normal and Hard. |
| **Shift Vault** | Swipe pieces in four directions to guide the golden key to the exit. Generated 6×6 puzzles. |
| **Star Pod Sprint** | Tap to jump over geometric obstacles, collect coins and survive. Three speed modes. |
| **Line Four** *(new)* | Drop cyan and pink discs into seven columns. Get four in a row to win. **Solo vs local CPU** or **two players on one device**. The CPU tries to win, blocks immediate threats and considers the next move. |
| **Pair Quest** *(new)* | Flip 16 cards to match eight procedural symbols. **Solo:** finish in fewer tries and less time. **Two players:** take turns, earn a point for each match and keep your turn when you find one. |
| **Tile Slide** *(new)* | Tap numbered tiles next to the empty space to put them in order. Choose a **3×3** or **4×4** board. Scrambles are generated through legal moves, so each puzzle is solvable. |

All six games are accessible directly on the **same 320×240 launcher screen**. The launcher staggers card appearances and cycles a subtle animated highlight. `HOME` returns to the launcher. The new games have their own procedural graphics and small touch-friendly controls. No external images, licensed sprites, audio assets or internet connection are needed at runtime.

### Actual-device photos of the original games

| Burst Hunt | Shift Vault | Star Pod Sprint |
| --- | --- | --- |
| <img src="web/assets/burst.webp" alt="Real display running Burst Hunt" width="220"> | <img src="web/assets/shift.webp" alt="Real display running Shift Vault" width="220"> | <img src="web/assets/sprint.webp" alt="Real display running Star Pod Sprint" width="220"> |

### Gameplay recordings from older versions

| Earlier gameplay (v1) | Later gameplay (v1.2) |
| --- | --- |
| <img src="web/assets/arcade-v1.gif" alt="Earlier CrowPanel gameplay" width="270"> | <img src="web/assets/arcade-v2.gif" alt="Real v1.2 CrowPanel gameplay showing the original three games" width="270"> |

Both GIFs are from older releases and do not show all six games in v1.3. [Watch the original gameplay video on YouTube](https://www.youtube.com/watch?v=BkuglAy6qOQ). The video is also embedded on the project website.

## Install directly from your browser

Open **https://pocket-arcade.lepczynski.it/** in desktop **Chrome or Edge**, using HTTPS. Connect the device with a **data-capable USB cable** and follow the installer's serial-port prompt. If your board does not enter download mode automatically, hold **BOOT**, briefly press **RESET**, then release BOOT. Press RESET after flashing if the game does not start automatically.

The page has two installation choices:

- **Install / recover:** a complete factory image starting at `0x0`, including the ESP32 bootloader (`0x1000`), partition table (`0x8000`), OTA initialisation (`0xe000`) and application (`0x10000`). Recommended for first installation and recovery.
- **Update firmware:** application only at `0x10000`. Use only if the existing device has the **same `huge_app.csv` partition scheme**. Avoid choosing *Erase device* if you want to preserve any other flash contents.

**Important:** The installer is specifically for the **Elecrow CrowPanel 2.8-inch ESP32-WROOM-32-N4 (320×240, ILI9341)**. It is **not** compatible with the CrowPanel 1.28-inch ESP32-S3 Rotary Buddy, M5Stack or other ESP32 display variants. ESP Web Tools detects the ESP32 chip family, not the attached panel model or wiring. Flashing a different board can overwrite its current firmware. Review the selected USB port before flashing.

Installation requires an ESP Web Tools-compatible Web Serial browser and a local USB connection. Normal gameplay does not require internet access. The browser flasher uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Hardware and PlatformIO

| Detail | Value |
| --- | --- |
| Device | Elecrow CrowPanel 2.8-inch ESP32 |
| Microcontroller | ESP32-WROOM-32-N4, 4 MB flash |
| Display | 320×240 landscape, ILI9341-compatible TFT |
| Touch | XPT2046-compatible resistive touch |
| Firmware | Arduino C++, PlatformIO, TFT_eSPI 2.5.31 |
| Partitions | `huge_app.csv` |
| TFT MISO / MOSI / SCLK | GPIO 4 / 13 / 14 |
| TFT CS / DC / backlight | GPIO 15 / 2 / 27 |
| Touch CS | GPIO 33 |

You can still build and upload directly with PlatformIO:

```bash
python3 -m pip install --user --upgrade platformio
pio run -e crowpanel_28
pio run -e crowpanel_28 --target upload
pio device monitor --baud 115200
```

For manual touch calibration, set `RUN_TOUCH_CALIBRATION = true` in `include/AppConfig.h`, upload the firmware and copy the five calibration values printed over serial into `src/AppConfig.cpp`. The supplied defaults are calibrated for the original test device and may differ on another unit.

## Project files

- `src/`, `include/`, `platformio.ini`: device code and firmware configuration.
- `web/index.html`, `web/_headers`: lightweight animated website and hosting headers.
- `web/assets/`: optimised original gameplay GIFs and three real video-frame photos (existing games only).
- `scripts/build_pages.sh`: firmware compilation, verified images and ESP Web Tools manifests.
- `CHANGELOG.md`, `LICENSE`, `NOTICE`, `SECURITY.md`: release history, rights and security information.

The source, six original game names, procedural game art, game logic and generated puzzles belong to this project. Line Four implements generic four-in-a-row rules using original code and art. Pair Quest uses original symbols, not a commercial card deck. Tile Slide uses the traditional numbered-tile puzzle mechanic with independently generated scrambles. Commercial game assets are not included. `Elecrow` and `CrowPanel` identify supported third-party hardware, not an endorsement or affiliation. See [NOTICE](NOTICE).

**License:** [MIT](LICENSE). Created by [Wojciech Lepczyński](https://lepczynski.it/).
