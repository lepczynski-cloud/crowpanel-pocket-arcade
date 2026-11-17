# Changelog

All notable changes to this project are documented in this file.

## [1.3.0] - 2026-11-17

### Added

- **Line Four**, an original four-in-a-row game with one-player local CPU and two-player hot-seat modes, disc-drop animation and a highlighted winning sequence.
- **Pair Quest**, a 16-card memory challenge with procedural symbols, animated flips, a solo mode and two-player turn-based scoring.
- **Tile Slide**, a 3×3 or 4×4 number-tile puzzle using solvable random-walk scrambles, move counter, timer and sliding animation.
- Six-card touch launcher with staged appearance and a rotating highlight, without adding image assets or increasing firmware dependencies.
- Website cards describing all three new games without claiming to show photos of unrecorded gameplay.

### Changed

- Updated the firmware version to 1.3.0 and the README, site metadata and game descriptions to six games.
- Preserved original GIFs and the embedded YouTube video, clearly marking the recorded footage as v1.2.
- Kept the browser firmware installer and automated firmware build unchanged.

## Website and installer refresh - 2026-11-16 (v1.2 firmware)

- Added a lightweight animated static website with actual-device gameplay, three still photos extracted from the newest footage and an embedded YouTube video.
- Added two ESP Web Tools installation options: verified factory firmware and application-only update.
- Added automated firmware compilation, merged ESP32 flash image, SHA-256-named binary assets and installer manifests.
- Compressed the two gameplay GIFs and removed the redundant GitHub Actions workflow.
- Clarified supported hardware and installer limitations in README.
- Firmware gameplay was unchanged from v1.2.0 in this website-only update.

## [1.2.0] - 2026-08-11

### Added

- A permanent Burst Hunt legend that clearly separates safe targets from red-X hazards.
- Distinct green rings around safe Burst Hunt targets and a high-contrast black, red and white hazard design.
- Domino, long-bar, square and L-shaped Shift Vault pieces.
- Four-direction movement for every Shift Vault piece.
- A dedicated Shift Vault exit panel separated from the game controls.
- Optional README locations for `docs/game_v1.gif` and `docs/game_v2.gif`.

### Changed

- Replaced continuous Burst Hunt playfield redraws with target-sized dirty-region updates.
- Replaced the blocking multi-frame Burst Hunt hit animation with a short local burst.
- Redesigned Shift Vault generation around movable polyomino pieces and reversible four-direction scrambles.
- Removed Shift Vault arrow buttons in favor of direct swipe controls.
- Made the gold key piece and green exit gate more prominent.
- Simplified Star Pod Sprint into a one-tap endless runner with spikes, barriers and coins.
- Removed the air boost, shield system, moving star field and scrolling hills from Star Pod Sprint.
- Replaced continuous Star Pod Sprint playfield redraws with local dirty-rectangle rendering.
- Updated the README, controls, rendering notes and originality statement for the revised games.

### Fixed

- Removed the repeated full-playfield clearing that caused visible flashing in Burst Hunt.
- Removed the repeated full-playfield clearing that caused visible flashing in Star Pod Sprint.
- Prevented the Shift Vault exit artwork from visually colliding with direction controls.
- Made Burst Hunt safe and unsafe objects immediately distinguishable during play.

## [1.1.0] - 2026-08-10

### Added

- Burst Hunt with four procedural target types, animated bursts, hazards, lives, combos and three difficulty modes.
- Star Pod Sprint with an original egg-shaped pod, air boost, multiple obstacles, sparks, shields and three speed modes.
- Three Shift Vault difficulty profiles with generated 6 x 6 layouts.
- Direct swipe movement for Shift Vault while retaining precision arrow controls.
- A gameplay media section and firmware version display.

### Changed

- Replaced Circuit Worm with Star Pod Sprint.
- Expanded Shift Vault from a 5 x 5 board to a 6 x 6 board.
- Reworked Shift Vault generation to create denser boards from solved layouts using reversible legal moves.
- Reworked the reaction game from fixed rounds into a continuous timed session.
- Simplified the repository documentation and removed nonessential contribution, testing and security files.

### Fixed

- Improved swipe detection by evaluating the complete gesture on touch release.
- Increased runner jump forgiveness and reduced the collision box.
- Made the reaction timer safe across the ESP32 `millis()` rollover.

## [1.0.0] - 2026-08-10

### Added

- The first Pocket Arcade launcher with three large touch cards.
- Reflex Beacon, a ten-round reaction-time game with randomized delays, combos, penalties and result statistics.
- Shift Vault, a generated 5 x 5 sliding-block puzzle with a highlighted target block and right-side exit.
- Circuit Worm, a swipe-controlled grid arcade game with food, growth, obstacles, bonuses and increasing speed.
- A shared dark arcade interface with a `HOME` button in every game.
- PlatformIO configuration for the Elecrow CrowPanel 2.8-inch ESP32 display.
- TFT_eSPI display and XPT2046-compatible touch configuration.
- Touch calibration mode and default calibration values.
- GitHub Actions firmware build workflow.
- MIT License and hardware-name disclaimer.
