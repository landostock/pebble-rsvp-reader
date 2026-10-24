# Phase 2 Transport

Status: companion transport and offline library cache implemented; Pixel 10 to Pebble Time 2 baseline verified.

This phase establishes the Android-to-Pebble transport path using PebbleKit Android 2 and a checked application-level packet envelope.

## Implemented Locally

- Shared binary packet encoder/decoder with CRC32 payload checks.
- Shared payload splitter for deterministic AppMessage-sized data packets.
- Shared ACK/NACK packet helpers.
- Shared transfer receiver with duplicate and out-of-order detection.
- Watch-side AppMessage packet receiver that validates header, version, length, sequence fields, and checksum before replying with ACK or NACK.
- Android-side packet encoder/decoder mirroring the shared protocol.
- Android listener service using `BasePebbleListenerService`.
- Android sender bridge using `DefaultPebbleSender`.
- Revocable device-token pairing with Android Keystore-backed token encryption.
- Conditional library sync using server `ETag` revisions.
- Atomic Android caches for the manifest, reader packages, and covers.
- Revision pruning for changed/deleted books while preserving the previous valid cache after failed downloads.
- Localized companion activity for pairing, sync, cache diagnostics, opening the watch app, and sending a request packet.
- Cached-book controls that open the watch app and send the first compact 64-token `RWND` reader window.
- Watch-side library with up to twelve cached titles, authors, separate resume/high-water progress, one-decimal progress summaries, and fixed-palette 16-color cover thumbnails; selecting an entry resumes that book through Android.
- Watch-side `RWND` reconstruction, validation, title display, dynamic RSVP words, paragraph context, and duration scaling.
- Automatic watch requests for the next reader window when the current window reaches its end.
- Four-window rotation keeps the previous, current, next, and following windows available; normal playback and adjacent full-text navigation do not wait on Bluetooth.
- Smart backlight forces illumination only during RSVP playback and full-text reading. Pause, menus, and the library return control to PebbleOS; `Always` and `System` remain explicit alternatives.
- Watch progress is cached on Android before upload, retried on app open, and reconciled against manifest timestamps. Language and library sort use the same local-first outbox pattern.
- Inbound messages are accepted only for the Pebble RSVP UUID, and transfer reassembly has explicit fragment, byte, active-transfer, and history limits.

## AppMessage Shape

Pebble dictionary key:

```text
1 = Packet
```

The value is a byte array containing the binary packet documented in [protocol.md](protocol.md).

## Current Chunk Size

Default packet payload size is 160 bytes before the 46-byte protocol header. This is deliberately conservative until real Pebble Time 2 measurements are available.

Do not raise this default based only on emulator behavior.

## Local Verification

- `npm test` covers packet encode/decode, checksums, version mismatch, duplicates, out-of-order packets, ACK/NACK generation, and packet splitting.
- `./gradlew :app:testDebugUnitTest` covers Android cache hit/miss behavior and revision pruning.
- `./gradlew :app:assembleDebug` compiles the companion against PebbleKit Android 2.
- `pebble build` compiles the watch receiver for `emery` on the locally installed SDK.

## Verified Hardware Baseline

- Pixel 10, Android 17, Pebble mobile app 1.11.0.3.
- Pebble Time 2, PebbleOS 4.36.2.
- Android companion installed and paired with the local server.
- Four manifests/packages/covers retained in the offline cache.
- Watch app installed through the USB ADB developer tunnel.
- Watch open command succeeded.
- Request packet produced the expected application ACK on Android.
- The Android build includes encoder tests for metadata, window sizing, and UTF-8-safe token truncation.

## Device Test Procedure

1. Install the built watch app on Pebble Time 2.
2. Build and install `android/:app` on Pixel 10.
3. Open the Android companion and select the active Pebble app if PebbleKit prompts for access.
4. Tap `Open watch app`.
5. Tap `Send packet smoke test`.
6. Confirm the watch receives a valid packet and sends an ACK without interrupting local RSVP playback.
7. Repeat with Bluetooth toggled during send.
8. Record maximum stable payload size and keep the production default below that value with margin.

## Not Done Yet

- Persistent mid-transfer continuation after the Android process or Pebble app is restarted.
- Long-running Bluetooth interruption, high-WPM boundary, and battery measurements on real hardware.
- Production Fly/Tigris deployment and multi-account ownership if the product expands beyond one protected library.
