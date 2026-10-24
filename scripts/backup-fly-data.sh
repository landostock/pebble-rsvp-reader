#!/bin/sh

set -eu

APP=${FLY_APP:-lando-pebble-rsvp}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
DESTINATION=${1:-"$ROOT/backups/$APP-$STAMP.tar.gz"}
REMOTE="/tmp/$APP-backup-$STAMP.tar.gz"

mkdir -p "$(dirname -- "$DESTINATION")"

flyctl ssh console --app "$APP" --command "tar -C /data -czf $REMOTE ."
flyctl sftp get --app "$APP" "$REMOTE" "$DESTINATION"
flyctl ssh console --app "$APP" --command "rm -f $REMOTE"

shasum -a 256 "$DESTINATION" > "$DESTINATION.sha256"
printf 'Backup: %s\nChecksum: %s.sha256\n' "$DESTINATION" "$DESTINATION"
