# Architecture

Pebble RSVP keeps durable storage, phone cache, and watch reader state separate.

```text
Web/PWA -> Fly.io API -> encrypted Fly Volume -> Android Companion -> Pebble Time 2
```

## Component Responsibilities

Fly.io API:

- authenticates the operator of a self-hosted library instance;
- finalizes uploads;
- processes EPUBs into reader packages and watch covers;
- writes library objects atomically to the mounted Fly volume;
- serves library manifests, metadata, reader packages, and progress endpoints;
- keeps no required state in the Machine root filesystem and is safe to stop between requests.

Fly Volume:

- stores the durable library, source EPUBs, covers, processed reader packages, settings, and progress JSON;
- is the source of truth for the permanent library.
- is encrypted at rest, limited to 1 GB, snapshotted for seven days, and supplemented by downloadable backups.

## Access Model

The current product is optimized for self-hosted Pebble users: one deployed instance owns one protected library. Production runs require `ADMIN_PASSWORD_HASH` and `SESSION_SECRET`; administrator mutations require a signed session and CSRF token. Android uses a separate revocable device token limited to library, package, cover, reader-settings, and progress routes. `PEBBLE_RSVP_DEV_NO_AUTH=1` is loopback-only and is surfaced to the UI as development mode.

Future community features should add explicit user ownership, invites, and sharing permissions instead of weakening the protected single-library model.

Android companion:

- authenticates with its own revocable, narrowly scoped device token rather than the administrator password;
- owns local cache and Pebble transport;
- downloads changed manifests/packages opportunistically;
- serves cached books to the watch without contacting Fly.io on the critical reading path;
- persists progress locally first, then syncs to Fly when available.

Pebble watch app:

- owns the immediate reading experience;
- starts from local persisted state without network access;
- keeps only current state, bounded metadata, and a rolling text buffer;
- never stores whole EPUBs or depends on live server availability during RSVP playback.

## Phase Boundaries

Phase 1 is a local watch reader prototype. It has hard-coded text and no backend dependency.

Phase 2 adds Android transport and cache with reader packages. The current companion covers secure device-token pairing, conditional manifest sync, revisioned package/cover caches, PebbleKit wiring, bounded packet validation, a four-window watch buffer, and locally durable progress/settings synchronization.

Phase 3 adds the hosted EPUB pipeline. Production uses the same file-backed object-store semantics on a mounted Fly volume; this is simpler and cheaper for the current single-user library than adding Tigris and a presigned-upload flow.

Phase 4 adds the responsive PWA library UI. The current PWA implements the local upload/library/detail/progress/settings/diagnostics workflow against the Phase 3 API.

Phase 5 adds full watch library views and book switching.

Phase 6 is reliability, performance, memory, battery, and cost hardening.

## Release Blockers

- Reading position loss beyond a few seconds.
- Silent Unicode corruption.
- Interpreting corrupt Bluetooth chunks as valid text.
- Fly.io on the watch reading critical path.
- Missing independent backups of the Fly volume.
- Recurring infrastructure cost caused by always-on workers or extra machines.
