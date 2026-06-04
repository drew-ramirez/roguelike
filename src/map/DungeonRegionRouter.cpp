/**
 * **************************************************************************
 * @file     DungeonRegionRouter.cpp
 * @brief    Maps normal-play depths to region builders
 * **************************************************************************
 */

#include "map/DungeonRegionRouter.hpp"

#include <stdexcept>

namespace rl {

DungeonRoute routeForDepth(int depth)
{
    if (depth < 1) {
        throw std::invalid_argument("Dungeon depth must be positive");
    }

    if (depth == 1) {
        return DungeonRoute{depth, DungeonRegion::Town, "town", "town"};
    }
    if (depth == 2) {
        return DungeonRoute{
            depth, DungeonRegion::Woods, "woods", "woods", true};
    }
    if (depth == 3) {
        return DungeonRoute{depth,
                            DungeonRegion::LimestoneCaverns,
                            "limestone",
                            "limestone-caverns",
                            true};
    }
    if (depth == 4) {
        return DungeonRoute{depth,
                            DungeonRegion::DeepCaverns,
                            "deep_caverns",
                            "deep-caverns",
                            true};
    }
    if (depth == 5) {
        return DungeonRoute{depth,
                            DungeonRegion::DwarfFortTransition,
                            "dwarf_transition",
                            "dwarf-transition",
                            true};
    }
    if (depth == 6) {
        return DungeonRoute{depth,
                            DungeonRegion::DragonLair,
                            "dragon_lair",
                            "dragon-lair",
                            true};
    }
    if (depth == 7) {
        return DungeonRoute{depth,
                            DungeonRegion::MushroomEntrance,
                            "mushroom_entrance",
                            "mushroom-entrance",
                            true};
    }
    if (depth == 8) {
        return DungeonRoute{depth,
                            DungeonRegion::MushroomForest,
                            "mushroom_forest",
                            "mushroom-forest",
                            true};
    }
    if (depth == 9) {
        return DungeonRoute{depth,
                            DungeonRegion::MushroomExit,
                            "mushroom_exit",
                            "mushroom-exit",
                            true};
    }
    return DungeonRoute{depth,
                        DungeonRegion::DarkElfCity,
                        "dark_elf_city",
                        "dark-elf-city",
                        true,
                        false};
}

std::string_view regionName(DungeonRegion region) noexcept
{
    switch (region) {
    case DungeonRegion::Town:
        return "Town";
    case DungeonRegion::Woods:
        return "Woods";
    case DungeonRegion::LimestoneCaverns:
        return "Limestone Caverns";
    case DungeonRegion::DeepCaverns:
        return "Deep Caverns";
    case DungeonRegion::DwarfFortTransition:
        return "Dwarf Fort Transition";
    case DungeonRegion::DragonLair:
        return "Dragon Lair";
    case DungeonRegion::MushroomEntrance:
        return "Mushroom Entrance";
    case DungeonRegion::MushroomForest:
        return "Mushroom Forest";
    case DungeonRegion::MushroomExit:
        return "Mushroom Exit";
    case DungeonRegion::DarkElfCity:
        return "Dark Elf City";
    }
    return "Unknown";
}

} // namespace rl
