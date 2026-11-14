#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
binary="$repo_root/build/windows-debug/Debug/roguelike.exe"

cd -- "$repo_root"
bash ./setup.sh >&2
cmake --preset windows-debug >&2
cmake --build --preset windows-debug --target roguelike >&2
exec "$binary" "$@"
