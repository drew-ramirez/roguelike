/**
 * **************************************************************************
 * @file     CorridorBuilder.hpp
 * @brief    Declares corridor carving strategies
 * **************************************************************************
 */

#pragma once

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class Map;

enum class CorridorStrategy {
    LShaped,
    StraightLine,
    Dogleg,
    Winding,
    WideLShaped
};

[[nodiscard]] CorridorStrategy chooseCorridorStrategy(int roll) noexcept;
[[nodiscard]] CorridorPath carveCorridor(
    Map& map, Point from, Point to, Rng& rng, CorridorStrategy strategy);

} // namespace rl
