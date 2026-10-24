#!/bin/sh

set -eu

ANDROID_HOME=${ANDROID_HOME:-"$HOME/Library/Android/sdk"}
ADB="$ANDROID_HOME/platform-tools/adb"

if [ ! -x "$ADB" ]; then
  echo "ADB wurde nicht unter $ADB gefunden." >&2
  exit 1
fi

echo "Warte auf Pebble-RSVP-Nachrichten. Beenden mit Ctrl-C."
"$ADB" -d logcat -s 'PebbleRsvpListener:D' '*:S'
