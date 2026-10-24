# Pebble RSVP

Pebble RSVP is a personal reading system for Pebble Time 2 (`emery`) and Android.

This repository currently contains the Phase 0 / Phase 1 foundation and working parts of the later phases:

- shared protocol, reader-package, text-normalization, pacing, sentence, context, and progress logic;
- automated tests for the highest-risk text and transfer behaviors;
- a Pebble C watch app with a four-window RSVP buffer, sentence rewind across window boundaries, paged full text, a local library, chapter navigation, configurable reading/display behavior, smart backlight control, and persistent progress/statistics;
- a Phase 2 Android companion with secure device-token pairing, revision-aware offline library/package/cover caching, settings and progress outboxes, 16-color watch-cover transfer, and rolling reader-window prefetch over PebbleKit Android 2;
- a protected server and PWA for EPUB library management, device-token pairing, processing, metadata edits, progress, and diagnostics;
- architecture, protocol, text-normalization, cost, and investigation notes.

The personal production instance runs at [lando-pebble-rsvp.fly.dev](https://lando-pebble-rsvp.fly.dev) on one auto-stopping Fly Machine with an encrypted 1 GB volume. Multi-device conflict policy and the latest real-device regression pass are intentionally not marked complete yet.

## Repository Layout

```text
pebble-rsvp/
├── android/
├── docs/
├── fixtures/
├── LICENSES/
├── server/
├── shared/
├── watch/
└── web/
```

## Local Development

Shared logic requires Node.js 22 or newer.

```sh
npm test
```

Run the local server and PWA:

```sh
PEBBLE_RSVP_DEV_NO_AUTH=1 PORT=8787 npm run start:server
```

Open `http://localhost:8787`.

Production deployment and backup instructions are in [docs/deployment-fly.md](docs/deployment-fly.md).

The watch app requires the RePebble/Pebble SDK and an `emery` build target:

```sh
cd watch
pebble build
```

Build the Android companion with Android Studio's bundled JDK and the installed SDK:

```sh
cd android
JAVA_HOME='/Applications/Android Studio.app/Contents/jbr/Contents/Home' \
ANDROID_HOME="$HOME/Library/Android/sdk" \
./gradlew :app:testDebugUnitTest :app:assembleDebug
```

For the first Pixel 10 and Pebble Time 2 installation, follow the German [hardware test runbook](docs/hardware-test-de.md). It includes a one-command Pixel setup using USB-only `adb reverse` for the local server.

## Current Verification Status

- Shared Node tests: passing.
- Pebble C build: passing for `emery` with SDK v4.33.1. The official tap and swipe recognizers compile, link, install, and start on Pebble Time 2.
- Server/PWA smoke test: local server served `/`, uploaded an in-memory EPUB fixture, updated `/api/library`, and returned a reader package.
- Production: HTTPS, password enforcement, Android-token routes, health checks, encrypted volume persistence, idle stop/wake, migrated data integrity, and an independent downloadable backup are verified on Fly.io.
- Android tests and build: passing with Gradle 9.5, compile SDK 37, Kotlin 2.4, and PebbleKit Android 2 v1.3.0. Cache hit/miss and revision-pruning behavior are covered.
- Reader-window encoder tests: passing for compact metadata/token blocks and UTF-8-safe truncation. The watch validates and renders the received window dynamically.
- Hardware baseline: verified on Pixel 10 with Android 17 and Pebble Time 2 with PebbleOS 4.36.2. Companion installation, offline library sync, watch-app installation/start, sentence-aware resume, native PebbleKit transfer, four-window continuity, and cross-window sentence rewind pass. The 16-color covers, revised statistics/display menus, and high-water progress recovery still require a real-device regression pass.

## Upstream References

- [RePebble developer docs](https://developer.repebble.com/docs/)
- [RePebble guides](https://developer.repebble.com/guides/toc/)
- [Pebble recommended guidelines](https://developer.repebble.com/guides/design-and-interaction/recommended/)
- [PebbleKit Android 2](https://github.com/pebble-dev/PebbleKitAndroid2)
- [Pebble examples](https://github.com/pebble-examples)
- [RSVP Nano](https://github.com/ionutdecebal/rsvpnano)

RSVP Nano is MIT licensed. Its license is preserved in [LICENSES/RSVP_NANO_LICENSE.txt](LICENSES/RSVP_NANO_LICENSE.txt).
