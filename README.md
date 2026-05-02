# Roguelike

A C++20 roguelike project built in small, playable slices.

## Status

Project bootstrap. The current executable is a minimal smoke target that proves the CMake/C++ toolchain is wired correctly.

## Planned Stack

- C++20 or newer
- CMake
- SDL2 + SDL_ttf for renderer/input
- EnTT for ECS
- JSON serialization
- Central seedable RNG wrapper

## Build

Install dependencies with vcpkg manifest mode, then configure with the vcpkg toolchain file:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
ctest --test-dir build -C Debug
```