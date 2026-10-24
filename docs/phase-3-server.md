# Phase 3 Server

Status: implemented locally and deployed for personal production use.

The server is a Node HTTP API over an object-store abstraction. Development uses local files under `server/.data/`; production mounts an encrypted Fly Volume at `/data` and retains the same API semantics.

## Implemented

- Static PWA serving from `web/`.
- Health and diagnostics endpoints.
- Single-library session endpoints with scrypt password verification, signed stateless cookies, explicit `auth_mode`, visible status, and logout.
- CSRF enforcement for authenticated non-GET requests.
- Local development auth bypass with `PEBBLE_RSVP_DEV_NO_AUTH=1`.
- Revocable Android device tokens stored only as SHA-256 hashes, with library/package/cover, reader-settings, and progress Bearer scope.
- Loopback-only development bypass, signed production sessions, CSRF checks, login throttling, bounded request bodies, security headers, and generic internal-error responses.
- Direct development EPUB upload endpoint.
- EPUB processing through the shared pipeline.
- Automatic EPUB cover extraction from OPF `cover-image`, OPF `meta name="cover"`, EPUB `guide` cover pages, and cover-like image filenames.
- Atomic book publish from the client perspective: source/package/metadata are written before `library/library.json` is updated.
- Library manifest endpoint with revision-derived `ETag` and `304 Not Modified` support.
- Book metadata read/update.
- Custom cover upload/download.
- Book deletion with source, cover, reader-package, metadata, and progress cleanup.
- Reader package download.
- Progress read/write.
- Storage usage diagnostics.
- Fly deployment with HTTPS, idle stop/wake, health checking, fixed 1 GB persistent storage, and independent backup tooling.

## API

```text
GET  /health
GET  /api/session
POST /api/session
DELETE /api/session
GET  /api/device-tokens
POST /api/device-tokens
DELETE /api/device-tokens/{token_id}
GET  /api/diagnostics
GET  /api/library
POST /api/books?filename=<name.epub>
DELETE /api/books/{book_id}
GET  /api/books/{book_id}/metadata
PUT  /api/books/{book_id}/metadata
GET  /api/books/{book_id}/cover
POST /api/books/{book_id}/cover
POST /api/books/{book_id}/cover/extract
GET  /api/books/{book_id}/reader-package
GET  /api/progress/{book_id}
PUT  /api/progress/{book_id}
```

## Local Development

```sh
PEBBLE_RSVP_DEV_NO_AUTH=1 PORT=8787 npm run start:server
```

Open:

```text
http://localhost:8787
```

For production-style auth, generate a password hash:

```sh
npm run hash-password -- '<password>'
```

Then run with:

```sh
ADMIN_PASSWORD_HASH='<hash>' SESSION_SECRET='<long random secret>' npm run start:server
```

## Still Required

- Replacement EPUB revision workflow.
- Multi-device progress conflict policy before expanding beyond one primary reader.
