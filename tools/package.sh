#!/bin/sh
# Builds the copy-to-device zip: unzip onto the Kobo's USB root, eject, done.
# Usage: tools/package.sh <build-dir>   (e.g. tools/package.sh build/kobo)
set -e

APP_NAME="minesweeper"
BUILD_DIR="${1:-build/kobo}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$BUILD_DIR/$APP_NAME"
STAGE="$(mktemp -d)"
OUT="$ROOT/dist/$APP_NAME.zip"

[ -f "$BIN" ] || { echo "!! $BIN not found — build the fbink flavor first"; exit 1; }

mkdir -p "$STAGE/.adds/$APP_NAME/assets" "$STAGE/.adds/kfmon/config" "$STAGE/.adds/nm"
cp "$BIN"                                       "$STAGE/.adds/$APP_NAME/$APP_NAME"
cp "$ROOT/dist/.adds/$APP_NAME/start.sh"        "$STAGE/.adds/$APP_NAME/"
cp "$ROOT/dist/.adds/nm/$APP_NAME"              "$STAGE/.adds/nm/"
cp "$ROOT"/dist/.adds/"$APP_NAME"/assets/*      "$STAGE/.adds/$APP_NAME/assets/"
cp "$ROOT/dist/kfmon/config/$APP_NAME.ini"      "$STAGE/.adds/kfmon/config/"
cp "$ROOT/dist/kfmon-$APP_NAME.png"             "$STAGE/kfmon-$APP_NAME.png"
chmod +x "$STAGE/.adds/$APP_NAME/$APP_NAME" "$STAGE/.adds/$APP_NAME/start.sh"

rm -f "$OUT"
(cd "$STAGE" && zip -r -X "$OUT" .adds "kfmon-$APP_NAME.png")
rm -rf "$STAGE"
echo ">> $OUT"
