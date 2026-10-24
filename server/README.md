# Server

Status: Phase 3 local scaffold.

The server will be a stateless Fly.io API backed only by Tigris Object Storage. Do not add a relational database, Redis, Fly Volumes, a background worker, or always-on infrastructure.

The EPUB processing pipeline must publish revisions atomically and never replace a working reader package with a failed processing attempt.

Local run:

```sh
PEBBLE_RSVP_DEV_NO_AUTH=1 PORT=8787 npm run start:server
```

`PEBBLE_RSVP_DEV_NO_AUTH=1` is only for local development. In that mode the API reports `auth_mode: "development"` and does not require a password.

Production auth requires:

```text
ADMIN_PASSWORD_HASH
SESSION_SECRET
```

Generate the password hash from the repository root:

```sh
npm run hash-password -- '<password>'
```

Without the development bypass, administrative operations require a valid signed session cookie. Static web files and `/api/session` remain public so the browser can show the sign-in screen.

The administrator can create one revocable device token per Android phone. Only a SHA-256 token hash is stored; the secret is returned once at creation. A device token is accepted as `Authorization: Bearer <token>` for library/package/cover reads and progress reads/writes. It cannot upload, edit, or delete books, manage covers, or create and revoke other tokens.
