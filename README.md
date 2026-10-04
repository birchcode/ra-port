# RA Port

**Native macOS, Linux, Android, and iOS source port of Command & Conquer: Red Alert.**

[![macOS](https://img.shields.io/badge/macOS-native-111111?logo=apple&logoColor=white)](#quick-start)
[![Linux](https://img.shields.io/badge/Linux-Ubuntu-e95420?logo=ubuntu&logoColor=white)](#linux-desktop)
[![Android](https://img.shields.io/badge/Android-debug%20APK-3ddc84?logo=android&logoColor=white)](#android-debug-apk)
[![iOS](https://img.shields.io/badge/iOS-debug%20app-111111?logo=apple&logoColor=white)](#ios-debug-app)
[![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064f8c)](#build-from-source)
[![Runtime](https://img.shields.io/badge/runtime-SDL2-cc3333)](#current-status)
[![Source-only](https://img.shields.io/badge/source--only-no%20game%20data-lightgrey)](#game-data)
[![License](https://img.shields.io/badge/license-GPLv3%20with%20additional%20terms-blue)](#license-and-notice)

`ra-port` lets you play Red Alert (1996) on modern platforms using SDL2. This fork develops [dk8827/ra-port](https://github.com/dk8827/ra-port), adding desktop LAN multiplayer, widescreen presentation, optional CRT rendering, camera controls, and changes to enemy AI and unit movement. The macOS, Linux, Android, and iOS platform support comes from upstream; the newer gameplay and presentation work has primarily been exercised on desktop.

![Red Alert running natively in a macOS window](docs/images/ra-port-macos-window.png)

The repository contains source, build tooling, and optional presentation artwork and icons. Original game data, movies, music, and disc images are not included. To play, provide legally obtained Red Alert assets from your own discs, mounted images, or local backups.

## What This Fork Adds

| Area | Changes from upstream |
| --- | --- |
| LAN multiplayer | Local UDP discovery, host/join screens, and match startup between macOS and Linux. Reliability, packet handling, and cross-platform AI/combat random-number sequencing fixes support the existing lockstep simulation. |
| Fullscreen and widescreen | Fullscreen toggles, an expanded battlefield, aspect-aware rendering and input, wider network/skirmish setup screens, centered compact dialogs, and optional widescreen title artwork. |
| Desktop camera | Middle-button grab panning, pointer-centered wheel/trackpad zoom from 1× down to ½×, sidebar list scrolling, and gesture cancellation on focus loss or dialog entry. |
| CRT presentation | Optional OpenGL phosphor-mask and scanline rendering, clean/CRT split comparison, adjustable mask strength, an optional highlight-glow experiment, and framebuffer diagnostics. |
| Enemy AI | Defensive posts, scouting, reserves, group attacks, incursion response, retreat/repair, counter-production, and improved campaign base economy. Easy difficulty has slower decisions, smaller attacks, and an opening grace period. |
| Movement and orders | Traffic-aware routing, waits and local yielding at blocked paths, harvester and formation yielding, fresh-order retry resets, nearby ground-unit defense, and helicopter/transport order fixes. |
| Desktop polish | Responsive in-game menu input, corrected timer comparisons, a zoom/tooltip overlap crash fix, desktop icons, and a local macOS widescreen playtest app packager. |
| Parsing and regression coverage | Hardened CPS/LCW and VQA decoding, plus tests for AI, movement, camera geometry, combat determinism, menus, timers, networking, and CRT rendering. |

See [fork changes and verification](docs/FORK_CHANGES.md) for the scope, evidence, and remaining checks. These changes preserve the original game engine and local asset workflow; internet multiplayer and release distribution are still unfinished.

## Why This Exists

Red Alert was released for a very different desktop world. This project keeps the original code recognizable while supporting macOS, Linux, Android, and iOS.

This is an unofficial source port based on the source code Electronic Arts released under GPLv3 with additional terms: <https://github.com/electronicarts/CnC_Red_Alert>.

## Current Status

| Status | Feature | Notes |
| --- | --- | --- |
| :white_check_mark: | macOS on Apple Silicon | Native desktop build and local playtest app; current input-test failure is recorded under [Tests](#tests). |
| :white_check_mark: | Linux on Ubuntu | Native SDL2 desktop executable; latest source checkpoint passed Linux CI. |
| :white_check_mark: | Android debug APK | Inherited local landscape APK for arm64-v8a; latest shared-engine changes need a fresh mobile build/playtest. |
| :white_check_mark: | iOS debug app | Inherited landscape simulator/device target; latest shared-engine changes need a fresh mobile build/playtest. |
| :white_check_mark: | Campaign | Allied/Soviet campaign play with updated enemy AI; balance and broader campaign acceptance remain open. |
| :white_check_mark: | Skirmish | Local skirmish with AI and movement changes; live crowded-path checks remain open. |
| :white_check_mark: | Videos | Videos are playing with sound. |
| :white_check_mark: | Controls and audio | macOS keyboard/mouse plus Android and iOS touch/audio work. |
| :white_check_mark: | LAN multiplayer | macOS/Linux discovery, join, and match startup verified; a complete synchronized match, rematch, and disconnect recovery remain open. |
| :x: | Internet multiplayer | Planned: private invite-code “dial-up” sessions; see [project plan](docs/PROJECT_PLAN.md). |
| :white_check_mark: | Fullscreen and widescreen | Desktop fullscreen toggle, wider battlefield, and smoother presentation pacing. |
| :white_check_mark: | Optional CRT display | Desktop OpenGL shader; enabled with `RA_CRT=1`. |
| :white_check_mark: | Desktop camera controls | Middle-drag panning, pointer-centered zoom, and sidebar wheel scrolling. |
| :white_check_mark: | AI and movement improvements | Implemented with engine regressions; see [fork changes](docs/FORK_CHANGES.md) for limits. |
| :x: | User-facing asset setup | Local preparation/run helpers exist; a guided installer is not implemented. |
| :x: | Expansion packs | Not a focus yet. |
| :white_check_mark: | macOS playtest `.app` | `scripts/package_mac_playtest.py` builds a local app that links to your ignored game assets. |
| :x: | Android release build | Only local debug APKs are supported right now. |
| :x: | iOS release build | Only local debug simulator/device builds are supported right now. |

## Quick Start

Clone this fork:

```sh
git clone https://github.com/birchcode/ra-port.git
cd ra-port
```

Install the macOS build tools:

```sh
brew install cmake ninja pkg-config sdl2
xcode-select --install
```

Build the port:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
```

On Ubuntu, install the Linux build tools and build the port:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config libsdl2-dev libgl1-mesa-dev
cmake -S . -B build-linux -G Ninja
cmake --build build-linux --target redalert_linux -j 8
```

Prepare local game data:

```sh
scripts/prepare_assets_from_local.sh \
  --allies /path/to/allies-disc \
  --soviet /path/to/soviet-disc
```

Run:

```sh
scripts/run_mac_dev.sh --no-build
```

On Ubuntu, run:

```sh
scripts/run_linux_dev.sh --no-build
```

For LAN multiplayer, run the same build and game data on both computers, then choose **Multiplayer Game → Network**. One player selects **New** and the other selects the advertised game and **Join**. Both machines must be on the same subnet and allow UDP port `34835` through their firewalls.

To build and run the Android debug APK, install the Android prerequisites listed below, keep the same prepared local game data under `assets/redalert`, then run:

```sh
scripts/build_android_debug.sh
scripts/run_android_debug.sh --no-build
```

To build and run the iOS simulator debug app, install full Xcode, keep the same prepared local game data under `assets/redalert`, then run:

```sh
scripts/build_ios_debug.sh
scripts/run_ios_simulator.sh --no-build
```

## Game Data

Original game archives, movies, music, disc images, installers, generated runtime palette caches, and packaged executables are excluded from Git. The optional widescreen title artwork and desktop icons are included separately from the original game data.

The asset preparation script copies from local paths that you provide:

- `assets/redalert/allies`
- `assets/redalert/soviet`

Those directories are ignored by git. They should contain original disc-root style files such as `INSTALL/REDALERT.INI` and the base-game `.MIX` files.

The Android and iOS debug builds use the same ignored `assets/redalert` tree. Gradle copies those local files into generated Android debug assets, and the iOS CMake target copies them into the debug app bundle. They are not checked in and they are not used for release packaging.

## Build From Source

Configure and build on macOS:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
```

If you do not have Ninja installed, omit `-G Ninja` and CMake will choose the default generator.

The build currently creates a raw macOS executable:

```sh
build/redalert_mac
```

To package that executable as the local widescreen playtest app:

```sh
python3 scripts/package_mac_playtest.py
open "build/RA Widescreen Playtest.app"
```

Prepare your game assets first. The app links to this checkout's `assets/` directory and copies the optional `presentation/` files; it is a local playtest package, not a standalone distributable. Moving the checkout requires rebuilding the package so its asset link points to the new location.

## Linux Desktop

Both desktop builds use the original Red Alert icon. The macOS playtest
packager includes it in the app bundle. On Linux, after configuring a build,
copy `build-linux/redalert.desktop` to `~/.local/share/applications/` to add
the game and its icon to the applications menu. The launcher points to the
current checkout, so recopy it after moving or reconfiguring the build.

Install Ubuntu build dependencies:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config libsdl2-dev libgl1-mesa-dev
```

Configure and build:

```sh
cmake -S . -B build-linux -G Ninja
cmake --build build-linux --target redalert_linux -j 8
```

Run from the repository root:

```sh
scripts/run_linux_dev.sh
```

For headless smoke validation, install Xvfb and ImageMagick, then capture the title/menu:

```sh
sudo apt-get install -y xvfb imagemagick
scripts/smoke_linux_menu.sh --seconds 20
```

## Run

The normal development run command builds if needed, verifies local assets, codesigns the executable, and launches from the repository root:

```sh
scripts/run_mac_dev.sh
```

Useful variants:

```sh
scripts/run_mac_dev.sh --no-build
scripts/run_mac_dev.sh --prepare-only
```

The Linux run helper follows the same flow without codesigning:

```sh
scripts/run_linux_dev.sh
scripts/run_linux_dev.sh --no-build
scripts/run_linux_dev.sh --prepare-only
```

You can also run the built executable directly after codesigning:

```sh
codesign --force --sign - build/redalert_mac
./build/redalert_mac
```

Runtime files such as `SAVEGAME.*`, `OPTIONS.INI`, `ASSERT.TXT`, screenshots, logs, and generated palette caches are ignored by git.

## Android Debug APK

The Android target is for local development and testing only. It builds an arm64-v8a debug APK, locks the activity to landscape, uses touch-native input, and extracts the bundled debug assets into app storage on first launch.

Install Android tooling:

- JDK 17
- Gradle compatible with Android Gradle Plugin 9.2.0
- Android SDK Platform 36
- Android SDK Build Tools
- Android NDK `28.2.13676358`
- Android CMake package
- Android platform-tools for `adb`

Android Studio is the easiest way to install the SDK, NDK, CMake, emulator, and platform-tools. On Homebrew-based macOS setups, the helper scripts auto-detect common `openjdk@17` and Android SDK locations when `JAVA_HOME`, `ANDROID_HOME`, or `ANDROID_SDK_ROOT` are not already set.

Build the debug APK:

```sh
scripts/build_android_debug.sh
```

The first build downloads SDL2 sources into ignored local storage under `android/third_party/`. The APK is written to:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

Install and launch on a connected device or emulator:

```sh
scripts/run_android_debug.sh --no-build
```

If an emulator is low on space and in-place install fails, uninstall the old debug app first:

```sh
scripts/run_android_debug.sh --no-build --fresh-install
```

`--fresh-install` removes existing app data, including extracted assets and saves.

Build, install, launch, and tail Android logs:

```sh
scripts/run_android_debug.sh --logcat
```

Useful direct commands:

```sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.raport.redalert/.RedAlertActivity
```

The debug APK intentionally includes your local ignored game data so the app can run on the device without external storage setup. Do not distribute that APK.

## iOS Debug App

The iOS target is for local development and testing only. It builds a landscape app for the iOS simulator by default, or for a signed arm64 device build when you provide an Apple development team.

Install iOS tooling:

- Full Xcode, not only Command Line Tools
- CMake
- Local Red Alert assets prepared under `assets/redalert`

Build the simulator app:

```sh
scripts/build_ios_debug.sh
```

The first build downloads SDL2 sources into ignored local storage under `ios/third_party/`. The app is written under:

```text
ios/build-simulator/Debug-iphonesimulator/redalert_ios.app
```

Install and launch on the booted iOS simulator:

```sh
scripts/run_ios_simulator.sh --no-build
```

Build a signed device app for iPhone or iPad:

```sh
RA_IOS_DEVELOPMENT_TEAM=TEAMID scripts/build_ios_debug.sh --device
```

The debug iOS app intentionally includes your local ignored game data. On first launch it copies that bundled data into app-writable storage before starting the game, so saves and options can be written inside the iOS sandbox. Do not distribute that app bundle.

## Fullscreen

Start fullscreen:

```sh
RA_FULLSCREEN=1 scripts/run_mac_dev.sh
```

Toggle fullscreen while running:

```text
Command+Return on macOS
Alt+Return on Linux
```

Use the monitor aspect ratio to show more battlefield horizontally without stretching:

```sh
RA_FULLSCREEN=1 RA_WIDESCREEN=1 scripts/run_linux_dev.sh
```

Add the optional VGA CRT phosphor and scanline treatment:

```sh
RA_FULLSCREEN=1 RA_WIDESCREEN=1 RA_CRT=1 scripts/run_linux_dev.sh
```

CRT mode uses a desktop OpenGL 2.1 compatibility shader; mobile keeps its SDL renderer. CRT mode displays the original 640x400 image at its intended 4:3 monitor aspect. For an A/B comparison, add `RA_CRT_DEBUG=split`; the left half remains clean and the right half receives the CRT treatment.
The default RGB phosphor pattern uses a finer 3×4 output-pixel repeat. The optional physical-subpixel mode keeps its original pixel calibration.
Set `RA_CRT_MASK_STRENGTH=0..100` to adjust only phosphor contrast (default `25`). At zero, beam reconstruction still runs; use `RA_CRT=0` for a complete bypass. Bright source rows have wider beams, and the shader uses sRGB light conversion without exponential highlight compression. Phosphor contrast fades near peak white to preserve SDR brightness, and at small source-to-output scales to reduce interference. The output-pixel pitch remains fixed; this is not physical panel-density calibration.

Use `RA_CRT_MODEL=legacy` for the previous fine-pattern shader. `RA_CRT_DEBUG=split` compares either model against clean pixels. Diagnostics: `RA_CRT_TEST=white|gray|red|green|blue|ramp|primaries|rows|columns|lines`; `RA_CRT_CAPTURE=/absolute/path.bmp` saves the displayed framebuffer after five seconds. The optional glass-glow experiment reuses beam samples for a small two-dimensional halo around highlights: `RA_CRT_GLOW=25` is subtle; `0` (default) disables it; `100` is the inspection maximum. It preserves black and peak white and uses available SDR headroom, rather than an HDR optical simulation. The legacy model ignores this setting. The screen stays flat; no temporal trails are added. See [CRT review and verification](docs/CRT_REVIEW.md).

Try it with `RA_CRT=1 RA_CRT_GLOW=25 scripts/run_mac_dev.sh --no-build` (or the Linux run script).

## Title artwork and physical CRTs

Widescreen main menus use `presentation/title-wide-854x400.idx` when available: generated wider artwork quantized to the original TITLE palette, with the original Westwood plaque and copyright pixels restored. The wider background is composed from the first title frame and stays behind compact dialogs, including New Game. Controls keep their original 640-pixel canvas, centered with matching pointer coordinates. Gameplay, movies and full-screen dialog backgrounds clear the title wings. Classic 640×400 mode, or a missing/invalid optional asset, keeps the original title. Other wider ratios fit the 16:9-authored composition to the logical viewport.

The network browser, network game creation, serial/modem setup and skirmish creation now use the full logical screen width in widescreen mode. Player/scenario lists grow with available space. Original grid and straight border sprites cover the wider background; small dialogs remain compact and centered. Leaving these screens restores the previous viewport.

Set `RA_TITLE_ART=original` to force the original title independently of widescreen gameplay. For a physical CRT, use `RA_CRT=0 RA_TITLE_ART=original`; add `RA_WIDESCREEN=0` for the classic layout. Display connections do not reliably identify CRT hardware, so this choice is explicit. The simulated CRT shader remains separately opt-in.

Build with `cmake --build build --target redalert_mac`, then package with `python3 scripts/package_mac_playtest.py`. The launcher sets explicit playtest defaults while respecting environment overrides and records startup/title selection in `Contents/MacOS/playtest.log`.

The updated local app is `RA Widescreen Playtest.app`; the older Camera Playtest app lacks this title integration. When bundling, include `presentation/` alongside the executable's resource root. Original game data remains separately required.

## Desktop camera controls

- **Middle-button drag:** grab and move the battlefield.
- **Mouse wheel:** scroll up to zoom out around the pointer; scroll down to return to the original scale. Range: ½×–1×, limited by map size.
- **Trackpad:** vertical scrolling zooms smoothly; horizontal scrolling does not move the camera.
- **Wheel over the sidebar:** scroll the build lists.

Left-drag selection and right-click commands remain available. Camera gestures are disabled in dialogs and movies, and dragging stops on Escape, focus loss, or fullscreen/size changes.

Edge scrolling remains enabled. Set `RA_EDGE_SCROLL=0` to disable it. For example:

```sh
RA_FULLSCREEN=1 RA_WIDESCREEN=1 RA_EDGE_SCROLL=0 scripts/run_mac_dev.sh
```

## Tests

At the saved source checkpoint [`c33a31b`](https://github.com/birchcode/ra-port/commit/c33a31b3a3c72145af9c4aafa8c3858a828e0269), [Linux CI passed](https://github.com/birchcode/ra-port/actions/runs/37217102852). The [macOS CI build completed but its test step failed](https://github.com/birchcode/ra-port/actions/runs/37217102868) at `tests/input_shim_test.cpp:43`: `camera_input_test` could not obtain its expected SDL window (`assert(window)`). The same assertion occurred in the local 4 October 2026 script-suite run. The full current macOS suite is therefore not claimed to pass. Older successful checks in the handoffs describe their tested snapshots.

Run the source-level tests and script checks:

```sh
tests/run_script_tests.sh
```

After building with Ninja and a compilation database, run the real-engine regressions:

```sh
python3 tests/run_ai_campaign_test.py build ai
python3 tests/run_ai_campaign_test.py build menu
python3 tests/run_ai_campaign_test.py build camera
python3 tests/run_ai_campaign_test.py build combat
python3 tests/run_ai_campaign_test.py build path
```

On Linux, replace `build` with `build-linux`. These fixtures exercise engine methods without requiring original game assets. They supplement gameplay checks; they do not establish full campaign, LAN, mobile, or release acceptance.

Validate a fresh checkout with a full build first:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
cmake --build build --target lan_udp_test -j 8
./build/lan_udp_test
tests/run_script_tests.sh
```

On Linux:

```sh
cmake -S . -B build-linux -G Ninja
cmake --build build-linux --target redalert_linux -j 8
cmake --build build-linux --target lan_udp_test -j 8
./build-linux/lan_udp_test
tests/run_script_tests.sh
```

## Project Layout

| Path | Purpose |
| --- | --- |
| `CODE/` | Main Red Alert game code |
| `PORT/MAC/` | Shared desktop runtime, compatibility shims, SDL2 integration |
| `PORT/ANDROID/` | Android entrypoint and platform-specific resource setup |
| `PORT/IOS/` | iOS entrypoint and writable sandbox resource setup |
| `android/` | Gradle Android app that builds the debug APK |
| `ios/` | CMake/Xcode iOS app target |
| `WIN32LIB/`, `WINVQ/` | Legacy support libraries used by the port |
| `scripts/` | Asset preparation, run helpers, smoke capture, Linux include overlay generation |
| `tests/` | Focused source-level and shim tests |
| `docs/` | Fork changes, validation handoffs, and remaining project work |
| `presentation/` | Optional widescreen title artwork and its provenance |
| `packaging/` | Desktop icons and Linux launcher template |
| `docs/images/` | README images only, not game data |

## Contributing

Keep the original source layout recognizable and preserve scripted missions, save compatibility, and deterministic multiplayer behavior when changing gameplay. Prefer focused changes with regression coverage. Work directly on `main` in this fork, following [AGENTS.md](AGENTS.md), unless a different workflow is explicitly requested.

The next checks are the current macOS input-test failure, live movement and Easy-difficulty playtests, a complete synchronized LAN match, and fresh mobile builds. Save/load and longer-session coverage, guided asset setup, portable desktop packaging, and mobile release/device validation remain useful follow-ups. A private internet multiplayer prototype comes after the LAN and protocol checks in the [multiplayer contract](docs/MULTIPLAYER_CONTRACT.md). See [fork changes](docs/FORK_CHANGES.md) for the current summary and [project plan](docs/PROJECT_PLAN.md) for the development history and acceptance details.

## License And Notice

The source code is distributed under GPLv3 with additional terms. See `LICENSE.md`.

This is an unofficial modified source port. It is not affiliated with, endorsed by, sponsored by, or supported by Electronic Arts or any other rights holder. See `NOTICE.md`.
