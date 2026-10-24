#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ANDROID_HOME=${ANDROID_HOME:-"$HOME/Library/Android/sdk"}
ADB="$ANDROID_HOME/platform-tools/adb"

if [ ! -x "$ADB" ]; then
  echo "ADB wurde nicht unter $ADB gefunden." >&2
  exit 1
fi

if ! command -v pebble >/dev/null 2>&1; then
  echo "Das pebble-Werkzeug wurde nicht gefunden." >&2
  exit 1
fi

DEVICE_COUNT=$(
  "$ADB" devices |
    awk 'NR > 1 && $2 == "device" { count += 1 } END { print count + 0 }'
)

if [ "$DEVICE_COUNT" -ne 1 ]; then
  "$ADB" devices -l
  echo "Erwartet wird genau ein entsperrtes und autorisiertes Android-Gerät." >&2
  exit 1
fi

echo "Lokale Pebble-Entwicklerverbindung auf dem Pixel starten ..."
RESULT=$(
  "$ADB" -d shell am broadcast \
    -a coredevices.coreapp.DEV_CONNECTION \
    -n coredevices.coreapp/coredevices.coreapp.debug.DevConnectionReceiver
)
printf '%s\n' "$RESULT"

case "$RESULT" in
  *'result=0, data="9000"'*) ;;
  *)
    echo "Die Pebble-App konnte keine lokale Entwicklerverbindung starten." >&2
    echo "Prüfe, ob die Pebble Time 2 in der Pebble-App verbunden ist." >&2
    exit 1
    ;;
esac

echo "Entwicklerport über USB weiterleiten ..."
"$ADB" -d forward tcp:9000 tcp:9000 >/dev/null

echo "Watch-App bauen und installieren ..."
(
  cd "$ROOT/watch"
  pebble build
  pebble install --phone 127.0.0.1
)

echo "Pebble RSVP wurde erfolgreich auf der verbundenen Watch installiert."
