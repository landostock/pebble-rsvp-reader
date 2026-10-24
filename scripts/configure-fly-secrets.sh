#!/bin/bash

set -euo pipefail

APP=${FLY_APP:-lando-pebble-rsvp}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

read -r -s -p "Administratorpasswort: " ADMIN_PASSWORD
printf '\n'
read -r -s -p "Administratorpasswort wiederholen: " ADMIN_PASSWORD_CONFIRMATION
printf '\n'

if [[ "$ADMIN_PASSWORD" != "$ADMIN_PASSWORD_CONFIRMATION" ]]; then
  unset ADMIN_PASSWORD ADMIN_PASSWORD_CONFIRMATION
  echo "Die Passwörter stimmen nicht überein." >&2
  exit 1
fi
if (( ${#ADMIN_PASSWORD} < 12 )); then
  unset ADMIN_PASSWORD ADMIN_PASSWORD_CONFIRMATION
  echo "Das Passwort muss mindestens 12 Zeichen lang sein." >&2
  exit 1
fi

cd "$ROOT"
ADMIN_PASSWORD_HASH=$(npm run --silent hash-password -- "$ADMIN_PASSWORD")
SESSION_SECRET=$(openssl rand -hex 32)
unset ADMIN_PASSWORD ADMIN_PASSWORD_CONFIRMATION

flyctl secrets set \
  ADMIN_PASSWORD_HASH="$ADMIN_PASSWORD_HASH" \
  SESSION_SECRET="$SESSION_SECRET" \
  --app "$APP"

unset ADMIN_PASSWORD_HASH SESSION_SECRET
echo "Die Produktionsgeheimnisse wurden für $APP hinterlegt."
