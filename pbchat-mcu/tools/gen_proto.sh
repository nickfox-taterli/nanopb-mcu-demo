#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PROTO_DIR="$ROOT_DIR/App/Proto"
OUT_DIR="$ROOT_DIR/App/Proto/generated"
mkdir -p "$OUT_DIR"
protoc \
  --plugin=protoc-gen-nanopb="$(command -v protoc-gen-nanopb)" \
  --nanopb_out="$OUT_DIR" \
  --nanopb_opt="-f$PROTO_DIR/chat.options" \
  -I "$PROTO_DIR" \
  "$PROTO_DIR/chat.proto"
