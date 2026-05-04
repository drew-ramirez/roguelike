/**
 * **************************************************************************
 * @file     MapValidation.cpp
 * @brief    Validates generated map reachability and required anchors
 * **************************************************************************
 */

#include "map/MapValidation.hpp"

#include <array>
#include <vector>

#include "map/Map.hpp"

namespace rl {
namespace {

[[nodiscard]] int countWalkableTiles(const Map& map)
{
    int count = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.isWalkable(x, y)) {
                ++count;
            }
        }
    }
    return count;
}

[[nodiscard]] int countReachableTiles(const Map& map, Point start)
{
    if (!map.isWalkable(start.x, start.y)) {
        return 0;
    }

    int reached = 0;
    std::vector<bool> visited(
        static_cast<std::size_t>(map.width() * map.height()), false);
    std::vector<Point> frontier{start};
    visited[static_cast<std::size_t>(map.xyIdx(start.x, start.y))] = true;

    constexpr std::array<Point, 4> neighbors{
        Point{1, 0},
        Point{-1, 0},
        Point{0, 1},
        Point{0, -1},
    };

    while (!frontier.empty()) {
        const Point current = frontier.back();
        frontier.pop_back();
        ++reached;

        for (const Point delta : neighbors) {
            const Point next{current.x + delta.x, current.y + delta.y};
            if (!map.isWalkable(next.x, next.y)) {
                continue;
            }

            const auto index =
                static_cast<std::size_t>(map.xyIdx(next.x, next.y));
            if (!visited[index]) {
                visited[index] = true;
                frontier.push_back(next);
            }
        }
    }

    return reached;
}

} // namespace

MapValidationResult validateMapBuild(const Map& map, Point start, Point stairs)
{
    MapValidationResult result;
    result.walkableTiles = countWalkableTiles(map);

    if (!map.inBounds(start.x, start.y) || !map.isWalkable(start.x, start.y)) {
        result.errors.push_back("start is not walkable");
    }
    if (!map.inBounds(stairs.x, stairs.y) ||
        !map.isWalkable(stairs.x, stairs.y)) {
        result.errors.push_back("stairs are not walkable");
    }

    result.reachableTiles = countReachableTiles(map, start);
    if (result.reachableTiles != result.walkableTiles) {
        result.errors.push_back("not all walkable tiles are reachable");
    }

    result.valid = result.errors.empty();
    return result;
}

} // namespace rl
