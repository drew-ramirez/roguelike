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
        return DungeonRoute{depth, DungeonRegion::Woods, "woods", "woods"};
    }
    if (depth <= 4) {
        return DungeonRoute{
            depth, DungeonRegion::LimestoneCaverns, "limestone", "cellular"};
    }
    if (depth <= 6) {
        return DungeonRoute{
            depth, DungeonRegion::DeepCaverns, "deep_caverns", "cavern-vaults"};
    }
    if (depth == 7) {
        return DungeonRoute{depth,
                            DungeonRegion::DwarfFortTransition,
                            "dwarf_transition",
                            "ruined-rooms"};
    }
    if (depth == 8) {
        return DungeonRoute{
            depth, DungeonRegion::DragonLair, "dragon_lair", "dragon-lair"};
    }
    if (depth <= 10) {
        return DungeonRoute{depth,
                            DungeonRegion::MushroomForest,
                            "mushroom_forest",
                            "mushroom-forest"};
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
    case DungeonRegion::MushroomForest:
        return "Mushroom Forest";
    case DungeonRegion::DarkElfCity:
        return "Dark Elf City";
    }
    return "Unknown";
}

} // namespace rl
