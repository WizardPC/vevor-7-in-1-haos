#!/bin/sh
# Compile et exécute les tests hôte du décodeur (aucun matériel requis).
#
# Aucun compilateur C++ n'est installé sur cet hôte : on utilise `zig c++` via le
# paquet pip `ziglang` (pas de root nécessaire).
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENV="${VENV:-$HOME/.venvs/cpp}"
OUT="${TMPDIR:-/tmp}/vevor_decoder_tests"

if [ ! -x "$VENV/bin/python" ]; then
  echo "Environnement C++ absent : $VENV" >&2
  echo "  uv venv $VENV && uv pip install --python $VENV/bin/python ziglang" >&2
  exit 1
fi

echo "-> compilation"
"$VENV/bin/python" -m ziglang c++ -std=c++17 -Wall -Wextra -O1 \
  "$ROOT/tests/test_decoder.cpp" \
  "$ROOT/components/vevor_7in1/vevor_frame.cpp" \
  "$ROOT/components/vevor_7in1/vevor_pcm.cpp" \
  -o "$OUT"

echo "-> execution"
"$OUT"
