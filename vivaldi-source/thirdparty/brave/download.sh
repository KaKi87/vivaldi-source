#!/usr/bin/env bash
# Download and verify Brave upstream source files.
# Usage: ./download.sh [config]
set -euo pipefail

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
URL_BASE="https://raw.githubusercontent.com/brave/brave-core/refs/heads/master"
FILES_CONFIG="${1:-$SRC_DIR/files.txt}"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

ok=true

# Strip comments and blank lines before feeding to while-read loop.
while IFS='|' read -r path expected _desc; do
  tmpfile="${tmpdir}/$(basename "$path")"
  destfile="$SRC_DIR/$path"
  dest_dir="$(dirname "$destfile")"
  url="$URL_BASE/$path"

  echo " * Updating $path..."

  if ! curl -fsSL "$url" -o "$tmpfile"; then
    echo " ! Download failed for $path" >&2
    ok=false
    continue
  fi

  actual=$(sha256sum "$tmpfile" | awk '{print $1}')

  if [[ "$actual" != "$expected" ]]; then
    echo "ERROR: Hash mismatch for $path:" >&2
    echo "   expected: $expected" >&2
    echo "   actual:   $actual" >&2
    ok=false
    continue
  fi

  echo "   - $path OK ($actual)"

  if [[ -f "$destfile" ]]; then
    dest_actual=$(sha256sum "$destfile" | awk '{print $1}')

    if [[ "$dest_actual" == "$actual" ]]; then
      echo "   [unchanged $destfile]"
      continue
    fi
  fi

  mkdir -p "$dest_dir"
  cp "$tmpfile" "$destfile"
  echo "   - updated $destfile"
done < <(grep -v '^#' "$FILES_CONFIG" | grep -v '^$')

if $ok; then
  echo "All files verified and in place."
else
  echo "ERROR: One or more checks failed — destination files unchanged." >&2
  exit 1
fi
