#!/usr/bin/env bash
# Build Tenebrae for the Move (aarch64) and package dist/tenebrae + the tarball.
#
# On the host: builds the native tool, regenerates module.json's chain_params
# from it, then re-runs itself inside the cross-compile container. Set
# CROSS_PREFIX to use a local cross toolchain instead of Docker.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE="tenebrae-builder"
ID="tenebrae"
cd "$REPO_ROOT"

if [ ! -f /.dockerenv ]; then
    # The host half. A build step that can be skipped in silence leaves a stale
    # module.json in the package, so every one of these is a hard failure.
    command -v cc >/dev/null   || { echo "ERROR: no C compiler for the native tool" >&2; exit 1; }
    command -v node >/dev/null || { echo "ERROR: node not found; module.json would be stale" >&2; exit 1; }
    mkdir -p build
    cc -O2 -Wall -Wextra -Isrc -Isrc/dsp tools/tenebrae_tool.c -lm -o build/tenebrae_tool
    node tools/gen_module_json.mjs
    if [ -z "${CROSS_PREFIX:-}" ]; then
        if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
            echo "Building the cross-compile image (first time only)..."
            docker build -t "$IMAGE" -f "$SCRIPT_DIR/Dockerfile" "$REPO_ROOT"
        fi
        docker run --rm -v "$REPO_ROOT:/build" -u "$(id -u):$(id -g)" -w /build "$IMAGE" ./scripts/build.sh
        exit 0
    fi
fi

CROSS_PREFIX="${CROSS_PREFIX:-aarch64-linux-gnu-}"
echo "=== Building Tenebrae ($CROSS_PREFIX) ==="

# Everything in dist/ is regenerated, so anything left there is from an older
# build. Removing a file from the build must remove it from the package.
rm -rf "dist/$ID"
mkdir -p build "dist/$ID"

"${CROSS_PREFIX}gcc" -O3 -shared -fPIC -std=gnu11 \
    -march=armv8-a -mtune=cortex-a72 \
    -fno-math-errno -fno-trapping-math -fvisibility=hidden \
    -Wall -Wextra \
    -Isrc -Isrc/dsp \
    src/dsp/tenebrae.c \
    -o build/dsp.so -lm -lpthread

# The Move runs an older C library than the build image. Fail here, not on the
# device, if the plugin asks for a newer one or for a C++ runtime.
need="$("${CROSS_PREFIX}objdump" -T build/dsp.so | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1)"
echo "newest C library symbol required: $need"
case "$need" in
    GLIBC_2.3[5-9]*|GLIBC_2.[4-9][0-9]*) echo "ERROR: needs $need; the Move has 2.35 at most" >&2; exit 1 ;;
esac
if "${CROSS_PREFIX}readelf" -d build/dsp.so | grep -q 'libstdc++\|libgcc_s'; then
    echo "ERROR: dsp.so links a C++ / gcc runtime" >&2; exit 1
fi
"${CROSS_PREFIX}nm" -D build/dsp.so | grep -q ' T move_plugin_init_v2' \
    || { echo "ERROR: move_plugin_init_v2 is not exported" >&2; exit 1; }

cat build/dsp.so      > "dist/$ID/dsp.so"
cat src/module.json   > "dist/$ID/module.json"
cat src/ui.js         > "dist/$ID/ui.js"
cat src/help.json     > "dist/$ID/help.json"
chmod +x "dist/$ID/dsp.so"

(cd dist && tar -czf "$ID-module.tar.gz" "$ID/")
echo "=== Done: dist/$ID/  and  dist/$ID-module.tar.gz ==="
ls -l "dist/$ID"
