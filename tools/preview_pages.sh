#!/usr/bin/env bash
# Draw Tenebrae's knob pages with the host's own renderer (no device needed).
#
#   tools/preview_pages.sh [out_dir]      PNGs, one per page, in build/pages
#
# Needs the host checkout beside this repo (../schwung-current). What it draws
# is what the module SERVES: the contract comes out of build/tenebrae_tool.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOST="$ROOT/../schwung-current"
OUT="${1:-$ROOT/build/pages}"
[ -d "$HOST/tools/param-pages" ] || { echo "host checkout not found at $HOST" >&2; exit 1; }
mkdir -p "$OUT"
node -e '
const { execFileSync } = require("node:child_process");
const t = process.argv[1] + "/build/tenebrae_tool";
const get = (k) => JSON.parse(execFileSync(t, ["json", k], { encoding: "utf8" }));
process.stdout.write(JSON.stringify({ _source: "tenebrae_tool", generated_at: new Date().toISOString(), module_count: 1,
  modules: [{ id: "tenebrae", category: "sound_generator", component_key: "synth", status: "ok", name: "Tenebrae",
              version: "dev", ui_hierarchy: get("ui_hierarchy"), chain_params: get("chain_params"), presets: null }],
  not_captured: [] }));
' "$ROOT" > "$OUT/fixture.json"
cd "$HOST"
node tools/param-pages/preview.mjs tenebrae --fixture "$OUT/fixture.json" --all --layout movy --png "$OUT" --scale 4
echo "pages in $OUT"
