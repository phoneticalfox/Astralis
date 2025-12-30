#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HEADER="examples/ffi/simple_math.h"
EXPECTED="bindings/simple_math.astr"
OUTPUT="bindings/.simple_math.tmp.astr"
SCRIPT="tools/astrac_c_import.py"

if ! command -v clang >/dev/null 2>&1; then
  echo "error: clang is required to run the importer regression" >&2
  exit 1
fi

cd "$REPO_ROOT"

python "$SCRIPT" "$HEADER" --link c -o "$OUTPUT"

diff -u "$EXPECTED" "$OUTPUT"
rm -f "$OUTPUT"
echo "c-import regression passed"
