# Roguelike

A roguelike built in C++20 with SDL2 and EnTT. Explore generated maps, fight
monsters, and collect equipment.

## Features

- Seeded maps using rooms, BSP, caves, mazes, Voronoi, and WFC.
- Field of view, explored-map memory, and camera scrolling.
- Melee and ranged combat, spells, traps, and monster AI.
- Inventory, equipment, vendors, and XP progression.
- Surface travel, persistent dungeon levels, and town portals.
- Hunger, foraging, camp rest, cooking, and shrine offerings.
- Save/load and an interactive map generation gallery.

## Build and run

On Windows, install Git for Windows, CMake 3.21+, and Visual Studio 2022 or
Build Tools with the **Desktop development with C++** workload.
Run from the repository root in Git Bash:

```bash
bash ./run.sh
```

The script sets up dependencies, builds as needed, and launches the game.
The first run downloads the required libraries.

Add `--seed 12345` for a repeatable map or `--gallery` to browse map generators.

## Controls

Move with arrows or WASD. Bump monsters to attack. `G` picks up items,
`I` opens inventory, and `F5` saves. `>` descends and `<` ascends.

## Repo map

```text
.
├── src/
│   ├── main.cpp           # CLI options and application entry
│   ├── assets/            # REXPaint asset loading
│   ├── core/              # Game loop, run state, RNG, and logs
│   ├── debug/             # Map generation gallery and playback
│   ├── ecs/               # Components and shared entity queries
│   ├── fov/               # Field-of-view calculations
│   ├── input/             # Input event and key types
│   ├── map/               # Tile maps, generation, and level snapshots
│   │   ├── corridors/     # Corridor carving
│   │   ├── generators/    # Map generation algorithms
│   │   ├── prefabs/       # Authored terrain and spawn layouts
│   │   └── rooms/         # Room carving
│   ├── progression/       # XP and player leveling
│   ├── raws/              # JSON content parsing and validation
│   ├── render/            # Console renderer, camera, and themes
│   ├── save/              # Save files and entity serialization
│   ├── spawn/             # Entity creation and level population
│   ├── systems/           # Input, turns, and gameplay domains
│   │   ├── ai/            # Bystander and monster decisions
│   │   ├── combat/        # Damage, melee, ranged attacks, spells, and traps
│   │   ├── effects/       # Shared item and spell effects
│   │   ├── inventory/     # Inventory and equipment
│   │   ├── rendering/     # Map, entity, and particle presentation
│   │   ├── services/      # Vendors, settlement services, and shrines
│   │   ├── spatial/       # Tile queries and visibility
│   │   ├── survival/      # Hunger, camps, foraging, and cooking
│   │   └── travel/        # Dungeon, portal, and world travel
│   ├── ui/                # HUD, menus, and targeting
│   └── world/             # World generation, biomes, and routes
├── assets/                # Art, palettes, and map patterns
├── data/
│   └── raws/              # Monsters, items, spells, and recipes
└── tests/                 # Unit and integration tests
```
