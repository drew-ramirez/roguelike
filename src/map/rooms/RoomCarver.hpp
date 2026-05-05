/**
 * **************************************************************************
 * @file     RoomCarver.hpp
 * @brief    Declares shaped room carving helpers
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/Rect.hpp"

namespace rl {

class Map;

enum class RoomShape { Rectangle, Circle, Cross, DoubleRect, Pillared };

[[nodiscard]] std::vector<Point> carveRectRoom(Map& map, const Rect& room);
[[nodiscard]] std::vector<Point>
carveRoomShape(Map& map, const Rect& room, RoomShape shape);
[[nodiscard]] std::vector<Point> roomTiles(const Rect& room);
[[nodiscard]] std::vector<Point> walkableRoomTiles(const Map& map,
                                                   const Rect& room);
[[nodiscard]] Point randomInteriorPointInRoom(const Rect& room, Rng& rng);

[[nodiscard]] RoomShape chooseRoomShape(const Rect& room, int roll) noexcept;

} // namespace rl
