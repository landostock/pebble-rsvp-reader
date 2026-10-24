# Protocol

Protocol status: v1 draft implemented in `shared/src/protocol.js`.

The transport is an application-level reliable protocol over Pebble AppMessage / PebbleKit Android 2 dictionaries. The binary packet below is the canonical payload shape inside the AppMessage data tuple.

## Versions

- `protocol_version`: `1`
- `reader_package_version`: `1`

Unknown versions must fail safely and must not be interpreted.

## Dictionary Mapping

Pebble AppMessage dictionary key `1` is named `Packet`. Its value is a byte array containing one binary packet.

## Packet Layout

See [shared/protocol/protocol.json](../shared/protocol/protocol.json).

Each packet contains:

- protocol version;
- message type;
- request id;
- book id;
- book revision;
- chunk id;
- sequence number;
- sequence count;
- payload length;
- CRC32 payload checksum;
- payload bytes.

## Message Types

- `data`: transfer payload.
- `ack`: positive acknowledgement.
- `nack`: retry or recovery request.
- `request`: watch asks Android for metadata, package chunks, or current state.
- `progress`: reading progress update.
- `settings`: compact language and library-sort update from the watch.

## Reliability Rules

- A receiver verifies header, version, declared length, sequence bounds, and checksum before delivering payload.
- Duplicate packets are acknowledged but not applied twice.
- Out-of-order packets are rejected with NACK in the initial implementation.
- Retries are idempotent because transfer identity includes request id, book id, revision, chunk id, and sequence number.
- Corrupt chunks must never reach the reader engine.
- Android sends packets sequentially and awaits PebbleKit/AppMessage delivery for each dictionary. Protocol NACKs remain available for malformed transfers; successful watch-side transfers do not emit a redundant control ACK because it would compete with the next window request for the single AppMessage outbox.

## Reader Window Payload

The first watch-readable payload is a compact `RWND` block carried by `data` packets. Version 3 contains the book title, a focus offset, and a bounded window of up to 80 tokens, with each token carrying variable-length UTF-8 display text, its 300 WPM base duration, and paragraph/chapter flags. Resume and navigation responses include up to twelve leading tokens before the focused word so paused context is immediately available; exact prefetch windows retain a zero focus offset and contiguous boundaries. The Android companion creates this block from the locally cached reader package; the Pebble reconstructs and validates it before activating the reader window.

The block is intentionally a rolling-window building block, not a complete book transfer. The watch keeps previous, current, next, and following windows in RAM and rotates them without interrupting normal RSVP playback. Subsequent windows come from Android's local package cache, never from the live server path.

The watch also accepts a compact versioned `LIBR` payload when the app opens. Version 6 carries synchronized language and library sort in its header and up to twelve entries with identity, revision, a UTF-8 title (48 bytes), an author (24 bytes), separate confirmed-resume and furthest-confirmed token indexes, progress/addition timestamps, and an optional 36x52 cover using an adaptive per-cover 16-color Pebble palette. The confirmed index is user-visible and used for resume; the high-water index remains monotonic for conflict resolution. Selecting an entry resumes through Android. `resume:N`, `window:N`, `rewind:N`, and `next:N` select exact or sentence-aware positions.

`RWND` record flags use bit 0 for paragraph end, bit 1 for chapter start, and bit 2 for sentence start. Bits 3-6 carry the prepared display-character ORP index and bit 7 marks that index as present; older payloads fall back to the same Unicode-aware length heuristic on the watch. Chapter-start words receive a short orientation pause in RSVP mode. Sentence flags let the watch rewind locally, while `resume:N`, `rewind:N`, and `next:N` requests resolve against canonical sentence metadata in Android when a sentence crosses a window boundary. Punctuation remains attached to its token, and device fields truncate only at UTF-8 character boundaries when necessary.

## AppMessage Notes

Pebble AppMessage requires explicit inbox/outbox buffers. The watch implementation must measure practical payload sizes on Pebble Time 2 and keep buffers conservative. The current shared tests prove binary packet behavior, not real Bluetooth throughput.
