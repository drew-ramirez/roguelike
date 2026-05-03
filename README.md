# Roguelike

Generic roguelike

## Features

- SDL2 console renderer
- ECS entities
- Tile map rendering
- Movement input
- Seeded map generation
- Field of view and map memory
- Monster spawning
- Bump combat
- HUD and message log

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

Run the game executable without arguments to open the window. The smoke test uses:

```bash
./build/Debug/roguelike --smoke
```

Use a reproducible map seed with:

```bash
./build/Debug/roguelike --seed 12345
```

The SDL_ttf renderer looks for a local monospace font. Set `ROGUELIKE_FONT_PATH` to a `.ttf` or `.ttc` file if automatic font discovery fails.

## Formatting And Linting

Use CMake targets:

```bash
cmake -S . -B build-lint -G Ninja -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build-lint --target format-check
cmake --build build-lint --target format
cmake --build build-lint --target lint
```

## Controls

- Arrow keys or WASD: move one tile.
- Window close button: quit.
