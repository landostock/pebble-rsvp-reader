# Android Companion

Status: Phase 2 companion and offline library cache implemented; local tests, Pixel 10 installation, and Pebble Time 2 smoke transport verified.

The Android companion must be a native Android app using PebbleKit Android 2, not PebbleKit JS.

Current upstream API notes from https://github.com/pebble-dev/PebbleKitAndroid2:

- send with `DefaultPebbleSender().sendDataToPebble(APP_UUID, data)`;
- receive by extending `BasePebbleListenerService`;
- return `ReceiveResult.Ack` only after data is stored;
- register a service with `io.rebble.pebblekit2.RECEIVE_DATA_FROM_WATCH`;
- use explicit Pebble app selection for sensitive data rather than broad auto-selection.

The companion now includes:

- a minimal native Android project;
- a PebbleKit Android 2 listener service;
- binary protocol encode/decode matching the shared JavaScript contract;
- a sequential Pebble packet sender;
- paste-based pairing with the web app's revocable device-token contract;
- Android Keystore-backed encryption for the stored device token;
- HTTPS enforcement with explicit emulator/local-development exceptions;
- conditional library synchronization using `ETag` and `If-None-Match`;
- atomic local manifest, reader-package, and cover caches keyed by book revision;
- compact watch-library metadata with UTF-8-safe titles, authors, and monochrome cover thumbnails;
- on-demand first-window and absolute `window:N` requests for watch gallery selection and text navigation;
- stale-revision and deleted-book cache cleanup without discarding a previously valid cache after a failed download;
- a localized pairing, sync, cache, diagnostics, and Pebble smoke-test activity;
- Gradle 9.5 wrapper and AGP 9 built-in Kotlin configuration compatible with PebbleKit Android 2 v1.3.0.

Build locally with Android Studio's bundled JDK and the installed Android SDK:

```sh
JAVA_HOME='/Applications/Android Studio.app/Contents/jbr/Contents/Home' \
ANDROID_HOME="$HOME/Library/Android/sdk" \
./gradlew :app:testDebugUnitTest :app:assembleDebug
```

The verified debug APK is generated at `app/build/outputs/apk/debug/app-debug.apk`. Compilation, local cache tests, Pixel 10 installation, watch-app start, packet send, and application ACK are verified.

Verified hardware baseline on 2026-08-30:

- Pixel 10 running Android 17 with Pebble mobile app 1.11.0.3;
- Pebble Time 2 running PebbleOS 4.36.2;
- pairing and unchanged library sync with four cached books/packages/covers;
- watch-app installation over the USB developer tunnel;
- watch-app open command returned `Success`;
- request smoke packet returned an application-level control ACK.

Next device checks are Bluetooth interruption/recovery, practical AppMessage payload sizing, persistent transfer resume, and long-running battery/reliability behavior.
