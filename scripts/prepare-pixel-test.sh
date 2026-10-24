#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ANDROID_HOME=${ANDROID_HOME:-"$HOME/Library/Android/sdk"}
JAVA_HOME=${JAVA_HOME:-"/Applications/Android Studio.app/Contents/jbr/Contents/Home"}
ADB="$ANDROID_HOME/platform-tools/adb"
APK="$ROOT/android/app/build/outputs/apk/debug/app-debug.apk"

if [ ! -x "$ADB" ]; then
  echo "ADB wurde nicht unter $ADB gefunden." >&2
  exit 1
fi

if [ ! -x "$JAVA_HOME/bin/java" ]; then
  echo "Java wurde nicht unter $JAVA_HOME gefunden." >&2
  exit 1
fi

if ! curl --fail --silent --show-error http://localhost:8787/health >/dev/null; then
  echo "Pebble RSVP läuft nicht unter http://localhost:8787." >&2
  echo "Starte zuerst: PEBBLE_RSVP_DEV_NO_AUTH=1 PORT=8787 npm run start:server" >&2
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

echo "Android-App bauen und testen ..."
(
  cd "$ROOT/android"
  JAVA_HOME="$JAVA_HOME" ANDROID_HOME="$ANDROID_HOME" \
    ./gradlew --no-daemon :app:testDebugUnitTest :app:assembleDebug
)

echo "APK auf dem Pixel installieren ..."
"$ADB" -d install -r "$APK"

echo "USB-Weiterleitung für den lokalen Server einrichten ..."
"$ADB" -d reverse tcp:8787 tcp:8787

echo "Bibliothek im Browser des Pixels öffnen ..."
"$ADB" -d shell am start \
  -a android.intent.action.VIEW \
  -d http://localhost:8787/

cat <<'EOF'

Bereit.

1. Im Pixel-Browser: Einstellungen > Android Companion.
2. Geräte-Token für "Pixel 10" erzeugen und Kopplungsdaten kopieren.
3. Pebble RSVP auf dem Pixel öffnen, Daten einfügen und Kopplung speichern.
4. Für den Transporttest in einem zweiten Terminal ausführen:
   ./scripts/watch-android-logs.sh

Die USB-Verbindung muss für http://localhost:8787 bestehen bleiben.
EOF
