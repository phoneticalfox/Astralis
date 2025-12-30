#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO_ROOT/src/seed0/astralis"
EXAMPLE_DIR="$REPO_ROOT/examples"

if [ ! -x "$BIN" ]; then
  echo "error: interpreter not built at $BIN" >&2
  exit 1
fi

status=0
for astr in "$EXAMPLE_DIR"/*.astr; do
  base="${astr##*/}"
  stem="${base%.astr}"
  expected="$EXAMPLE_DIR/$stem.out"
  input="$EXAMPLE_DIR/$stem.in"
  skip="$EXAMPLE_DIR/$stem.skip"

  if [ -f "$skip" ]; then
    echo "skip: $base"
    continue
  fi

  if [ ! -f "$expected" ]; then
    echo "missing expected output for $base (expected $expected)" >&2
    status=1
    continue
  fi

  tmp=$(mktemp)
  if [ -f "$input" ]; then
    if ! "$BIN" "$astr" <"$input" >"$tmp" 2>&1; then
      echo "program $base failed" >&2
      cat "$tmp"
      status=1
      rm -f "$tmp"
      continue
    fi
  else
    if ! "$BIN" "$astr" >"$tmp" 2>&1; then
      echo "program $base failed" >&2
      cat "$tmp"
      status=1
      rm -f "$tmp"
      continue
    fi
  fi

  if ! diff -u "$expected" "$tmp" >/dev/null; then
    echo "output mismatch for $base" >&2
    diff -u "$expected" "$tmp" || true
    status=1
  else
    echo "ok: $base"
  fi
  rm -f "$tmp"

done
exit $status
