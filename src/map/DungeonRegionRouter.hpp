/**
 * **************************************************************************
 * @file     DungeonRegionRouter.hpp
 * @brief    Declares depth-to-region routing for normal play
 * **************************************************************************
 */

#pragma once

#include <string>
#include <string_view>

namespace rl {

enum class DungeonRegion {
    Town,
    Woods,
    LimestoneCaverns,
    DeepCaverns,
    DwarfFortTransition,
    DragonLair,
    MushroomForest,
    DarkElfCity
};

struct DungeonRoute {
    int depth = 1;
    DungeonRegion region = DungeonRegion::Town;
    std::string regionId = "town";
    std::string builderId = "town";
    bool hasUpStairs = false;
    bool hasDownStairs = true;
};

[[nodiscard]] DungeonRoute routeForDepth(int depth);
[[nodiscard]] std::string_view regionName(DungeonRegion region) noexcept;

} // namespace rl
