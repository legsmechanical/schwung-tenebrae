#!/usr/bin/env bash
# Measure Tenebrae's CPU cost ON THE MOVE (no host involved).
#
#   tools/bench_move.sh                 8, 12 and 16 voices
#   VOICES="8" tools/bench_move.sh      just the shipped voice count
#
# Cross-compiles tools/tenebrae_tool.c -- the plugin source itself, with the
# flags scripts/build.sh uses -- copies it to a scratch folder on the device,
# runs `bench` (worst case: every voice held, full stack, every effect) and,
# for the shipped voice count, `cost` (each preset playing its phrase), then
# removes the scratch folder. It runs at the lowest priority and restarts
# nothing, but it does keep one of the Move's cores busy for a few minutes.
#
# The figures are CPU time on one core as a share of real time. Inside the
# host the same code shares its core and its cache with everything else, so
# read them as a floor.
set -euo pipefail
cd "$(dirname "$0")/.."
MOVE_HOST="${MOVE_HOST:-ableton@move.local}"
VOICES="${VOICES:-8 12 16}"
SCRATCH="/data/UserData/tenebrae-bench"
mkdir -p build/bench
for v in $VOICES; do
    docker run --rm -v "$PWD:/build" -w /build tenebrae-builder \
        aarch64-linux-gnu-gcc -O3 -fPIC -std=gnu11 -march=armv8-a -mtune=cortex-a72 \
        -fno-math-errno -fno-trapping-math -DTB_VOICES="$v" \
        -Isrc -Isrc/dsp tools/tenebrae_tool.c -o "build/bench/tenebrae_tool_v$v" -lm -lpthread
done
ssh "$MOVE_HOST" "mkdir -p '$SCRATCH'"
for v in $VOICES; do scp -q "build/bench/tenebrae_tool_v$v" "$MOVE_HOST:$SCRATCH/"; done
for v in $VOICES; do
    echo "=== $v voices: worst case ==="
    ssh "$MOVE_HOST" "cd '$SCRATCH' && nice -n 19 ./tenebrae_tool_v$v bench"
    if [ "$v" = 8 ]; then
        echo "=== 8 voices: every preset playing its phrase ==="
        ssh "$MOVE_HOST" "cd '$SCRATCH' && nice -n 19 ./tenebrae_tool_v8 cost" | tee build/bench/cost_move.txt | sort -t'%' -k1 | awk '{print}' | tail -4
    fi
done
for v in $VOICES; do ssh "$MOVE_HOST" "rm -f '$SCRATCH/tenebrae_tool_v$v'"; done
ssh "$MOVE_HOST" "rmdir '$SCRATCH'"
