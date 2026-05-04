/**
 * **************************************************************************
 * @file     RoomCarver.cpp
 * @brief    Implements shaped room carving helpers
 * **************************************************************************
 */

#include "map/rooms/RoomCarver.hpp"

#include <algorithm>

#include "map/Map.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

void carvePoint(Map& map, std::vector<Point>& carved, Point point)
{
    if (!map.inBounds(point.x, point.y)) {
        return;
    }
    map.setTile(point.x, point.y, TileType::Floor);
    if (std::find(carved.begin(), carved.end(), point) == carved.end()) {
        carved.push_back(point);
    }
}

void carveRectInto(Map& map, std::vector<Point>& carved, const Rect& rect)
{
    for (int y = rect.y1; y < rect.y2; ++y) {
        for (int x = rect.x1; x < rect.x2; ++x) {
            carvePoint(map, carved, Point{x, y});
        }
    }
}

} // namespace

std::vector<Point> carveRoomShape(Map& map, const Rect& room, RoomShape shape)
{
    std::vector<Point> carved;
    const Point center = room.center();

    if (shape == RoomShape::Rectangle || room.width() < 6 ||
        room.height() < 6) {
        carveRectInto(map, carved, room);
        return carved;
    }

    if (shape == RoomShape::Circle) {
        const int radiusX = room.width() / 2;
        const int radiusY = room.height() / 2;
        const int radiusSquared = std::min(radiusX, radiusY);
        for (int y = room.y1; y < room.y2; ++y) {
            for (int x = room.x1; x < room.x2; ++x) {
                const int dx = x - center.x;
                const int dy = y - center.y;
                if ((dx * dx) + (dy * dy) <= radiusSquared * radiusSquared) {
                    carvePoint(map, carved, Point{x, y});
                }
            }
        }
        return carved;
    }

    if (shape == RoomShape::Cross) {
        const Rect horizontal{room.x1, center.y - 1, room.x2, center.y + 2};
        const Rect vertical{center.x - 1, room.y1, center.x + 2, room.y2};
        carveRectInto(map, carved, horizontal);
        carveRectInto(map, carved, vertical);
        return carved;
    }

    if (shape == RoomShape::DoubleRect) {
        const Rect first{room.x1, room.y1, center.x + 1, room.y2};
        const Rect second{center.x - 1, center.y - 1, room.x2, room.y2};
        carveRectInto(map, carved, first);
        carveRectInto(map, carved, second);
        return carved;
    }

    carveRectInto(map, carved, room);
    for (int y = room.y1 + 2; y < room.y2 - 2; y += 3) {
        for (int x = room.x1 + 2; x < room.x2 - 2; x += 3) {
            if (Point{x, y} == center) {
                continue;
            }
            map.setTile(x, y, TileType::Wall);
            carved.erase(std::remove(carved.begin(), carved.end(), Point{x, y}),
                         carved.end());
        }
    }
    return carved;
}

RoomShape chooseRoomShape(const Rect& room, int roll) noexcept
{
    if (room.width() < 7 || room.height() < 7) {
        return RoomShape::Rectangle;
    }

    switch (roll % 5) {
    case 1:
        return RoomShape::Circle;
    case 2:
        return RoomShape::Cross;
    case 3:
        return RoomShape::DoubleRect;
    case 4:
        return RoomShape::Pillared;
    default:
        return RoomShape::Rectangle;
    }
}

} // namespace rl
