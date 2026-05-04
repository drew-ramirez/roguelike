/**
 * **************************************************************************
 * @file     RoomCarver.hpp
 * @brief    Declares shaped room carving helpers
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "core/Point.hpp"
#include "map/Rect.hpp"

namespace rl {

class Map;

enum class RoomShape { Rectangle, Circle, Cross, DoubleRect, Pillared };

[[nodiscard]] std::vector<Point>
carveRoomShape(Map& map, const Rect& room, RoomShape shape);

[[nodiscard]] RoomShape chooseRoomShape(const Rect& room, int roll) noexcept;

} // namespace rl
