# CrowPanel Pocket Arcade

**Three original touchscreen games. One tiny offline ESP32 arcade.**

[**Play with it, watch the video and install firmware from your browser**](https://pocket-arcade.lepczynski.it/) · [YouTube](https://www.youtube.com/watch?v=BkuglAy6qOQ) · [Blog](https://lepczynski.it/) · [More projects](https://github.com/lepczynski-cloud)

[![Version](https://img.shields.io/badge/firmware-1.2.0-48e7ca.svg)](CHANGELOG.md) [![License: MIT](https://img.shields.io/badge/license-MIT-9f8aff.svg)](LICENSE)

<p align="center"><img src="web/assets/arcade-v2.gif" alt="Real CrowPanel Pocket Arcade running the newest firmware, filmed on the physical device" width="320"></p>

The Elecrow **CrowPanel 2.8-inch ESP32** runs three games, a touch-first launcher and original procedural graphics. No Wi-Fi, Bluetooth, PSRAM, SD card, downloaded game assets, account or background service is required to play.

The GIF and photos below come from footage of the actual device, not rendered product mockups.

## The games

| Burst Hunt | Shift Vault | Star Pod Sprint |
| --- | --- | --- |
| <img src="web/assets/burst.webp" alt="Burst Hunt on the real CrowPanel" width="220"> | <img src="web/assets/shift.webp" alt="Shift Vault on the real CrowPanel" width="220"> | <img src="web/assets/sprint.webp" alt="Star Pod Sprint on the real CrowPanel" width="220"> |
| **Reaction game.** Tap the safe targets, avoid red-X mines, build combos and protect your lives. Easy, Normal and Hard. | **Sliding puzzle.** Swipe pieces in four directions to move the gold KEY to the green exit. Generated 6×6 boards and three difficulty levels. | **One-touch runner.** Tap to jump over spikes and barriers, collect coins and survive longer. Chill, Arcade and Turbo. |

Tap a card on the device to start a game. `HOME` returns to the launcher. Shift Vault uses swipe gestures; Star Pod Sprint uses one-tap jumps and has a pause button. The drawing code updates only changed regions of the display to reduce flashing.

### Previous and current gameplay

| Earlier version | Current version (v1.2.0) |
| --- | --- |
| <img src="web/assets/arcade-v1.gif" alt="Earlier Pocket Arcade gameplay" width="270"> | <img src="web/assets/arcade-v2.gif" alt="Current Pocket Arcade gameplay" width="270"> |

[Watch the Pocket Arcade video on YouTube](https://www.youtube.com/watch?v=BkuglAy6qOQ). The website also embeds this video next to the project information.

## Install directly from your browser

Open **https://pocket-arcade.lepczynski.it/** in desktop **Chrome or Edge**, using HTTPS. Connect the device with a **data-capable USB cable** and follow the installer's serial-port prompt. If your board does not enter download mode automatically, hold **BOOT**, briefly press **RESET**, then release BOOT. Press RESET after flashing if the game does not start automatically.

The page has two installation choices:

- **Install / recover:** a complete factory image starting at `0x0`, including the ESP32 bootloader (`0x1000`), partition table (`0x8000`), OTA initialisation (`0xe000`) and application (`0x10000`). Recommended for first installation and recovery.
- **Update firmware:** application only at `0x10000`. Use only if the existing device has the **same `huge_app.csv` partition scheme**. Avoid choosing *Erase device* if you want to preserve any other flash contents.

**Important:** The installer is specifically for the **Elecrow CrowPanel 2.8-inch ESP32-WROOM-32-N4 (320×240, ILI9341)**. It is **not** compatible with the CrowPanel 1.28-inch ESP32-S3 Rotary Buddy, M5Stack or other ESP32 display variants. ESP Web Tools detects the ESP32 chip family, not the attached panel model or wiring. Flashing a different board can overwrite its current firmware. Review the selected USB port before flashing.

Installation requires an ESP Web Tools-compatible Web Serial browser and a local USB connection. Normal gameplay does not require internet access. The browser flasher uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Automatic website and firmware deployment on Cloudflare Pages

Every push to `main` triggers a **Cloudflare Pages Git build**. The build script compiles firmware from the checked-in C++ sources, creates the complete factory image and application-only update, validates both, writes versioned SHA-256-named `.bin` files and generates the two ESP Web Tools manifests. The resulting static website and firmware are deployed together. **No separate GitHub Actions workflow, manual binary upload or website framework is required.**

In **Cloudflare > Workers & Pages**, create a new Pages project connected to `lepczynski-cloud/crowpanel-pocket-arcade` with these settings:

| Setting | Value |
| --- | --- |
| Production branch | `main` |
| Framework preset | `None` |
| Build command | `bash scripts/build_pages.sh` |
| Build output directory | `web` |
| Root directory | Repository root (default) |
| Build system | `v3` |
| Build environment variable | `PYTHON_VERSION=3.11.11` |
| Custom domain | `pocket-arcade.lepczynski.it` |

In the Pages project, open **Custom domains > Set up a domain** and add `pocket-arcade.lepczynski.it`. If `lepczynski.it` uses Cloudflare DNS in the same account, Pages normally creates the relevant CNAME automatically. Otherwise add a CNAME for `pocket-arcade` to the actual `YOUR-PAGES-PROJECT.pages.dev` hostname, **after** associating the domain inside Pages. Wait until the domain is active with HTTPS before using Web Serial.

The generated manifests and binaries are deliberately ignored by Git and created **only during a successful build**. Failed builds do not publish incomplete firmware. The website uses native HTML/CSS, a small inline script, real device footage and a single external ESP Web Tools script; no Node package installation is needed.

To build locally (requires Python 3, internet access for the toolchain and Linux/macOS Bash):

```bash
bash scripts/build_pages.sh
```

This produces `web/manifest-factory.json`, `web/manifest-update.json` and `web/firmware/*.bin`. You can preview the result with a basic HTTP server, but **serial flashing requires HTTPS or a browser-trusted localhost origin**.

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
- `web/assets/`: optimised original gameplay GIFs and three real video-frame photos.
- `scripts/build_pages.sh`: complete automatic firmware compilation and static-site packaging.
- `CHANGELOG.md`, `LICENSE`, `NOTICE`, `SECURITY.md`: release history, rights and security information.

The source, original game names, procedural game art and generated puzzles belong to this project. Commercial game assets are not included. `Elecrow` and `CrowPanel` identify supported third-party hardware, not an endorsement or affiliation. See [NOTICE](NOTICE).

**License:** [MIT](LICENSE). Created by [Wojciech Lepczyński](https://lepczynski.it/).
