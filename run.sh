#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
binary="$repo_root/build/windows-debug/Debug/roguelike.exe"

if [[ ! -f "$binary" ]]; then
    echo "Game binary not found. Build it with:" >&2
    echo "  bash ./setup.sh" >&2
    echo "  cmake --preset windows-debug" >&2
    echo "  cmake --build --preset windows-debug" >&2
    exit 1
fi

cd -- "$repo_root"
exec "$binary" "$@"
