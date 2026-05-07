/**
 * **************************************************************************
 * @file     CorridorBuilder.cpp
 * @brief    Implements corridor carving strategies
 * **************************************************************************
 */

#include "map/corridors/CorridorBuilder.hpp"

#include <algorithm>
#include <string>

#include "map/Map.hpp"
#include "map/MapGeometry.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

void carvePoint(Map& map, CorridorPath& path, Point point, int radius = 0)
{
    for (int y = point.y - radius; y <= point.y + radius; ++y) {
        for (int x = point.x - radius; x <= point.x + radius; ++x) {
            if (!map.inBounds(x, y)) {
                continue;
            }
            map.setTile(x, y, TileType::Floor);
            const Point carved{x, y};
            if (std::find(path.tiles.begin(), path.tiles.end(), carved) ==
                path.tiles.end()) {
                path.tiles.push_back(carved);
            }
        }
    }
}

void carveHorizontal(Map& map, CorridorPath& path, int x1, int x2, int y)
{
    const auto [first, last] = std::minmax(x1, x2);
    for (int x = first; x <= last; ++x) {
        carvePoint(map, path, Point{x, y});
    }
}

void carveVertical(Map& map, CorridorPath& path, int y1, int y2, int x)
{
    const auto [first, last] = std::minmax(y1, y2);
    for (int y = first; y <= last; ++y) {
        carvePoint(map, path, Point{x, y});
    }
}

void carveWideHorizontal(Map& map, CorridorPath& path, int x1, int x2, int y)
{
    const auto [first, last] = std::minmax(x1, x2);
    for (int x = first; x <= last; ++x) {
        carvePoint(map, path, Point{x, y}, 1);
    }
}

void carveWideVertical(Map& map, CorridorPath& path, int y1, int y2, int x)
{
    const auto [first, last] = std::minmax(y1, y2);
    for (int y = first; y <= last; ++y) {
        carvePoint(map, path, Point{x, y}, 1);
    }
}

void carveStraightLine(Map& map, CorridorPath& path, Point from, Point to)
{
    for (const Point point :
         bresenhamLine(from, to, LineStartPolicy::IncludeStart)) {
        carvePoint(map, path, point);
    }
}

[[nodiscard]] std::string strategyName(CorridorStrategy strategy)
{
    switch (strategy) {
    case CorridorStrategy::LShaped:
        return "l-shaped";
    case CorridorStrategy::StraightLine:
        return "straight";
    case CorridorStrategy::Dogleg:
        return "dogleg";
    case CorridorStrategy::Winding:
        return "winding";
    case CorridorStrategy::WideLShaped:
        return "wide";
    }
    return "l-shaped";
}

} // namespace

CorridorStrategy chooseCorridorStrategy(int roll) noexcept
{
    switch (roll % 5) {
    case 1:
        return CorridorStrategy::StraightLine;
    case 2:
        return CorridorStrategy::Dogleg;
    case 3:
        return CorridorStrategy::Winding;
    case 4:
        return CorridorStrategy::WideLShaped;
    default:
        return CorridorStrategy::LShaped;
    }
}

CorridorPath carveCorridor(
    Map& map, Point from, Point to, Rng& rng, CorridorStrategy strategy)
{
    CorridorPath path;
    path.strategy = strategyName(strategy);

    if (strategy == CorridorStrategy::StraightLine) {
        carveStraightLine(map, path, from, to);
        return path;
    }

    if (strategy == CorridorStrategy::Dogleg) {
        const Point middle{
            rng.rangeInclusive(std::min(from.x, to.x), std::max(from.x, to.x)),
            rng.rangeInclusive(std::min(from.y, to.y), std::max(from.y, to.y))};
        carveHorizontal(map, path, from.x, middle.x, from.y);
        carveVertical(map, path, from.y, middle.y, middle.x);
        carveHorizontal(map, path, middle.x, to.x, middle.y);
        carveVertical(map, path, middle.y, to.y, to.x);
        return path;
    }

    if (strategy == CorridorStrategy::Winding) {
        Point current = from;
        constexpr int maxSteps = 240;
        for (int step = 0; step < maxSteps && current != to; ++step) {
            carvePoint(map, path, current);
            const bool moveHorizontal =
                current.y == to.y || (current.x != to.x && rng.coinFlip());
            if (moveHorizontal) {
                current.x += current.x < to.x ? 1 : -1;
            } else {
                current.y += current.y < to.y ? 1 : -1;
            }
        }
        carvePoint(map, path, to);
        return path;
    }

    const bool horizontalFirst = rng.coinFlip();
    if (strategy == CorridorStrategy::WideLShaped) {
        if (horizontalFirst) {
            carveWideHorizontal(map, path, from.x, to.x, from.y);
            carveWideVertical(map, path, from.y, to.y, to.x);
        } else {
            carveWideVertical(map, path, from.y, to.y, from.x);
            carveWideHorizontal(map, path, from.x, to.x, to.y);
        }
        return path;
    }

    if (horizontalFirst) {
        carveHorizontal(map, path, from.x, to.x, from.y);
        carveVertical(map, path, from.y, to.y, to.x);
    } else {
        carveVertical(map, path, from.y, to.y, from.x);
        carveHorizontal(map, path, from.x, to.x, to.y);
    }

    return path;
}

} // namespace rl
