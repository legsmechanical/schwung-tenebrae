#!/usr/bin/env bash
# Everything that can be checked without the device, in one go.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -Wall -Wextra -Werror -Isrc -Isrc/dsp tools/tenebrae_tool.c -lm -o build/tenebrae_tool
node tools/gen_module_json.mjs
./build/tenebrae_tool selftest
for seed in 1 2 3; do ./build/tenebrae_tool fuzz 30 "$seed"; done
node tests/contract.test.mjs
python3 tests/dsp_checks.py
python3 tests/bank_checks.py
echo "ALL OK"
