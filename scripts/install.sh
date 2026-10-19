#!/usr/bin/env bash
# Copy dist/tenebrae to the Move.
#
#   ./scripts/install.sh                               (WiFi, move.local)
#   MOVE_HOST=ableton@172.16.254.1 ./scripts/install.sh   (USB tether)
#
# Installs into the STOCK Schwung tree and does NOT restart anything: a restart
# ends whatever is running on the device. A new module shows up after the next
# host restart; an updated dsp.so is picked up when the synth is next loaded
# into a slot (swap it out and back in).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
ID="tenebrae"
MOVE_HOST="${MOVE_HOST:-ableton@move.local}"
DEST="/data/UserData/schwung/modules/sound_generators/$ID"
cd "$REPO_ROOT"

[ -d "dist/$ID" ] || { echo "dist/$ID not found: run ./scripts/build.sh first" >&2; exit 1; }

echo "=== Installing Tenebrae -> $MOVE_HOST:$DEST ==="
ssh "$MOVE_HOST" "mkdir -p '$DEST'"
# Each file goes up under a temporary name and is moved into place: writing
# straight over a dsp.so that is loaded overwrites the mapped file in place.
for f in "dist/$ID/"*; do
    fn="$(basename "$f")"
    scp -q "$f" "$MOVE_HOST:$DEST/$fn.new"
    ssh "$MOVE_HOST" "mv -f '$DEST/$fn.new' '$DEST/$fn' && chmod a+rw '$DEST/$fn'"
done

echo "Verifying..."
fail=0
for f in "dist/$ID/"*; do
    fn="$(basename "$f")"
    want="$(md5 -q "$f" 2>/dev/null || md5sum "$f" | cut -d' ' -f1)"
    got="$(ssh "$MOVE_HOST" "md5sum '$DEST/$fn' | cut -d' ' -f1")"
    if [ "$want" != "$got" ]; then echo "  MISMATCH: $fn"; fail=1; else echo "  ok  $fn"; fi
done
[ "$fail" = 0 ] || { echo "Verification FAILED" >&2; exit 1; }
echo "=== Installed. Nothing was restarted. ==="
