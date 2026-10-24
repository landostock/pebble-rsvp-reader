# Phase 0 Investigation

Status: partially complete.

This file records what has actually been inspected and what still needs hardware validation. It is not a release-completion claim.

## Sources Inspected

- RePebble SDK documentation: https://developer.repebble.com/docs/
- RePebble guide table of contents: https://developer.repebble.com/guides/toc/
- Pebble recommended design guidelines: https://developer.repebble.com/guides/design-and-interaction/recommended/
- Pebble AppMessage data guide: https://developer.repebble.com/guides/communication/sending-and-receiving-data/
- Pebble buttons guide: https://developer.repebble.com/guides/events-and-services/buttons/
- Pebble persistent storage guide: https://developer.repebble.com/guides/events-and-services/persistent-storage/
- Pebble app metadata guide: https://developer.repebble.com/guides/tools-and-resources/app-metadata/
- Pebble hardware information: https://developer.repebble.com/guides/tools-and-resources/hardware-information/
- Pebble system fonts: https://developer.repebble.com/guides/app-resources/system-fonts/
- PebbleKit Android 2 source and README: https://github.com/pebble-dev/PebbleKitAndroid2
- RSVP Nano source and conversion spec: https://github.com/ionutdecebal/rsvpnano

## Findings

- Pebble Time 2 maps to the `emery` platform, with 200 x 228 resolution and 64-color display according to the current hardware table.
- Pebble `AppMessage` uses dictionary key/value tuples. The watch app must explicitly open inbox/outbox buffers, and oversized messages are dropped.
- Pebble documentation recommends waiting for `outbox_sent` before sending a burst of messages. The transport protocol therefore needs one-in-flight or bounded send windows with ACK/NACK at the application layer.
- Pebble persistent storage is small: 4 kB per app and 256 bytes per stored value. The watch can persist current reader state, but not whole books.
- Pebble buttons are the primary input method. The prototype uses select for play/pause, up/down for WPM, back for rewind/context exit, and select hold for context.
- PebbleKit Android 2 v1.3.0 provides `DefaultPebbleSender`, `BasePebbleListenerService`, `Map<UInt, PebbleDictionaryItem>`, and `ReceiveResult` for coroutine-based companion communication.
- RSVP Nano reader behavior worth adapting: deterministic WPM base interval, word-length delay, complexity delay, punctuation delay, real sentence rewind, paragraph context reconstruction, and ORP anchoring.
- RSVP Nano ORP logic chooses the 0th, 1st, 2nd, 3rd, or 4th word character depending on token length buckets.
- RSVP Nano EPUB conversion uses `container.xml`, OPF manifest/spine, EPUB2 NCX / EPUB3 nav, XHTML parsing, scoped language/direction directives, entity decoding, and paragraph/chapter boundaries.

## Reused Or Adapted Algorithms

- Adapted conceptually in `shared/src/rsvp.js`: word-length bonus, complexity bonus, punctuation delay, abbreviation guard, WPM stepping, and ORP focus position.
- Adapted conceptually in `shared/src/reader-package.js`: sentence-boundary rewind and paragraph context reconstruction.
- Adapted conceptually in `shared/src/unicode.js`: entity decoding, whitespace collapse, soft-hyphen removal, and conservative line-break hyphenation.

No RSVP Nano source code was copied into runtime modules. The MIT license is retained because algorithmic behavior and tests were derived from the reference project.

## Still Required Before Phase 2

- Retest the basic Pebble Time 2 app with the latest available SDK. Local compile succeeded for `emery` with Pebble Tool v5.0.39 and SDK v4.17; the tool reported SDK v4.33.1 as available.
- Install on a real Pebble Time 2.
- Confirm German glyph rendering for the mandatory sample text.
- Keep the verified Android debug build green as server synchronization and rolling-buffer transfer are added.
- Send bidirectional test packets between Pixel 10 and Pebble Time 2.
- Record AppMessage payload limits and practical chunk size on real hardware.
