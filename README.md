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

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```
