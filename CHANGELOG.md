# Changelog

All notable changes to this project are documented in this file.

## [1.1.0] - 2026-08-10

### Added

- Burst Hunt with four procedural target types, animated bursts, hazards, lives, combos and three difficulty modes.
- Star Pod Sprint, an original endless jumper with an egg-shaped pod, double-jump boost, four obstacle types, sparks, shields and three speed modes.
- Three distinct Shift Vault difficulty profiles with 8, 10 or 12 blocks and two block lengths.
- Direct swipe movement for Shift Vault while retaining precision arrow controls.
- A gameplay GIF placeholder and a YouTube link placeholder in the README.
- Firmware version display on the launcher.

### Changed

- Replaced Circuit Worm with Star Pod Sprint.
- Expanded Shift Vault from a 5 x 5 board to a 6 x 6 board.
- Reworked Shift Vault generation to create denser boards from solved layouts using reversible legal moves.
- Made the Shift Vault key block and exit tunnel more visible.
- Reworked the reaction game from fixed rounds into a continuous, progressively faster session.
- Simplified the repository documentation and removed nonessential contribution, testing and security files.

### Fixed

- Improved swipe detection by evaluating the complete gesture on touch release.
- Increased runner jump forgiveness and reduced the collision box for the resistive touchscreen format.
- Made the reaction timer safe across the ESP32 `millis()` rollover.

## [1.0.0] - 2026-08-10

### Added

- Initial Pocket Arcade launcher.
- Reflex Beacon reaction game.
- Shift Vault sliding-block puzzle.
- Circuit Worm swipe-controlled arcade game.
- CrowPanel 2.8-inch PlatformIO configuration and touch calibration mode.
- GitHub Actions build workflow and MIT license.
