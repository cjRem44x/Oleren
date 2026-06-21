#!/usr/bin/env bash
# deploy.sh — install olrn compiler + stdlib to system
#
# Usage:
#   ./deploy.sh [PREFIX]
#
# PREFIX defaults to /usr/local.
# Installs:
#   PREFIX/bin/olrn
#   PREFIX/lib/olrn/stdlib/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${1:-/usr/local}"
BIN_DIR="$PREFIX/bin"
LIB_DIR="$PREFIX/lib/olrn"

# Require a built binary
if [ ! -f "$SCRIPT_DIR/olrn" ]; then
    echo "error: olrn binary not found — run 'make' first" >&2
    exit 1
fi

echo "==> installing olrn to $PREFIX"

install -d "$BIN_DIR"
install -m 755 "$SCRIPT_DIR/olrn" "$BIN_DIR/olrn"

install -d "$LIB_DIR"
rm -rf "$LIB_DIR/stdlib"
cp -r "$SCRIPT_DIR/stdlib" "$LIB_DIR/stdlib"

echo "==> done"
echo "    binary:  $BIN_DIR/olrn"
echo "    stdlib:  $LIB_DIR/stdlib"
echo ""
echo "    Set OLRN_STDLIB=$LIB_DIR/stdlib if the compiler can't find it automatically."
