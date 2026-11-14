/**
 * **************************************************************************
 * @file     MapQualityMetrics.cpp
 * @brief    Measures reusable map generation quality characteristics
 * **************************************************************************
 */

#include "map/MapQualityMetrics.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr std::size_t tinyComponentLimit = 8;

[[nodiscard]] int cardinalPassableNeighbors(const Map& map, Point point)
{
    int count = 0;
    for (const Point neighbor : cardinalNeighbors(point)) {
        if (isConnectivityPassable(map, neighbor)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] int surroundingPassableNeighbors(const Map& map, Point point)
{
    int count = 0;
    for (int y = point.y - 1; y <= point.y + 1; ++y) {
        for (int x = point.x - 1; x <= point.x + 1; ++x) {
            if ((x != point.x || y != point.y) &&
                isConnectivityPassable(map, Point{x, y})) {
                ++count;
            }
        }
    }
    return count;
}

[[nodiscard]] bool isLocalChokepoint(const Map& map, Point point)
{
    if (!isConnectivityPassable(map, point)) {
        return false;
    }

    const std::array<bool, 4> open{
        isConnectivityPassable(map, Point{point.x, point.y - 1}),
        isConnectivityPassable(map, Point{point.x + 1, point.y}),
        isConnectivityPassable(map, Point{point.x, point.y + 1}),
        isConnectivityPassable(map, Point{point.x - 1, point.y}),
    };
    const int count = static_cast<int>(open[0]) + static_cast<int>(open[1]) +
                      static_cast<int>(open[2]) + static_cast<int>(open[3]);
    return count == 2 && ((open[0] && open[2]) || (open[1] && open[3]));
}

[[nodiscard]] int countTinyNaturalBlockerComponents(const Map& map)
{
    std::vector<bool> visited(static_cast<std::size_t>(map.tileCount()), false);
    int tinyComponents = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point start{x, y};
            const auto startIndex = static_cast<std::size_t>(map.xyIdx(x, y));
            if (visited[startIndex] || !isNaturalBlocker(map.tileAt(x, y))) {
                continue;
            }

            std::size_t size = 0;
            std::vector<Point> frontier{start};
            visited[startIndex] = true;
            while (!frontier.empty()) {
                const Point current = frontier.back();
                frontier.pop_back();
                ++size;
                for (const Point next : cardinalNeighbors(current)) {
                    if (!map.inBounds(next.x, next.y) ||
                        !isNaturalBlocker(map.tileAt(next.x, next.y))) {
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
            if (size <= tinyComponentLimit) {
                ++tinyComponents;
            }
        }
    }
    return tinyComponents;
}

} // namespace

MapQualityMetrics measureMapQuality(const Map& map)
{
    MapQualityMetrics metrics;
    metrics.totalTiles = map.tileCount();
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point point{x, y};
            if (!isConnectivityPassable(map, point)) {
                continue;
            }
            ++metrics.walkableTiles;
            if (surroundingPassableNeighbors(map, point) >= 7) {
                ++metrics.broadOpenTiles;
            }
            if (cardinalPassableNeighbors(map, point) <= 2) {
                ++metrics.narrowTiles;
            }
            if (isLocalChokepoint(map, point)) {
                ++metrics.chokepoints;
            }
        }
    }

    const auto regions = findConnectedRegions(map);
    metrics.disconnectedRegions =
        regions.empty() ? 0 : static_cast<int>(regions.size()) - 1;
    metrics.tinyBlockerComponents = countTinyNaturalBlockerComponents(map);
    if (metrics.totalTiles > 0) {
        metrics.walkableCoverage = static_cast<double>(metrics.walkableTiles) /
                                   static_cast<double>(metrics.totalTiles);
    }
    if (metrics.walkableTiles > 0) {
        metrics.broadOpenPercent = static_cast<double>(metrics.broadOpenTiles) /
                                   static_cast<double>(metrics.walkableTiles);
        metrics.narrowPercent = static_cast<double>(metrics.narrowTiles) /
                                static_cast<double>(metrics.walkableTiles);
    }
    return metrics;
}

} // namespace rl
