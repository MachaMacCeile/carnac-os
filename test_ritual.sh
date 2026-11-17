#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
[ -f src/PresetStructures.h ] || { echo 'Run inside Carnac checkout' >&2; exit 1; }
[ -f src/dsp/ritual.c ] || { echo 'Install ritual core first' >&2; exit 1; }
CC="${CC:-cc}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FLAGS=(-std=c11 -O2 -Wall -Wextra -Werror -Wconversion -Wshadow -pedantic -Isrc -Isrc/dsp)
"$CC" "${FLAGS[@]}" src/dsp/ritual.c src/host/test_ritual.c -o "$TMP/test_ritual"
"$TMP/test_ritual"
"$CC" "${FLAGS[@]}" src/dsp/ritual.c src/host/test_ritual_adapter.c -o "$TMP/test_adapter"
"$TMP/test_adapter"
echo 'RITUAL PASS: host core + real Carnac adapter'
