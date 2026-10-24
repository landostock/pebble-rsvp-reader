# Fly.io Deployment

The personal production instance is `https://lando-pebble-rsvp.fly.dev` in the Frankfurt region.

## Cost Controls

- one `shared-cpu-1x` Machine with 512 MB RAM;
- the Machine stops when idle and starts on the next request;
- zero Machines remain running while idle;
- one 1 GB volume, with no automatic growth;
- no dedicated IPv4, database, Redis, worker, or custom domain;
- the Android companion caches books, so reading does not keep Fly awake.

The 512 MB limit is intentional. Processing the largest current EPUB peaks at about 208 MB locally,
which leaves too little safety margin on a 256 MB Machine.

## First Deployment

Set the production secrets with the interactive helper. The password is neither printed nor stored in
the repository or shell history:

```sh
./scripts/configure-fly-secrets.sh
```

Deploy the service:

```sh
flyctl deploy
```

## Data Migration

Create a local archive without embedding it in the Docker image:

```sh
tar -C server/.data -czf /tmp/pebble-rsvp-data.tar.gz .
flyctl sftp put /tmp/pebble-rsvp-data.tar.gz /tmp/pebble-rsvp-data.tar.gz \
  --app lando-pebble-rsvp
```

Upload the archive to `/tmp`, then extract it into `/data` and fix ownership:

```sh
flyctl ssh console --app lando-pebble-rsvp \
  --command "tar -C /data -xzf /tmp/pebble-rsvp-data.tar.gz && chown -R node:node /data && rm /tmp/pebble-rsvp-data.tar.gz"
```

## Verification

```sh
curl --fail https://lando-pebble-rsvp.fly.dev/health
flyctl status --app lando-pebble-rsvp
flyctl volumes list --app lando-pebble-rsvp
```

Create a new Android device token in the hosted web UI and pair the companion with the generated
HTTPS credentials. Revoke the old local-development token after hosted synchronization succeeds.

The first request after an idle stop can take several seconds while Fly starts the Machine. Android
continues serving cached books to the watch during that time.

## Backups

Fly volume snapshots are a recovery aid, not the primary backup. Download a separate archive after
uploads and periodically thereafter:

```sh
./scripts/backup-fly-data.sh
```
