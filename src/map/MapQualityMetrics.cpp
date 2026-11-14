/**
 * **************************************************************************
 * @file     MapQualityMetrics.cpp
 * @brief    Measures reusable map generation quality characteristics
 * **************************************************************************
 */

#include "map/MapQualityMetrics.hpp"

#include <array>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int tinyComponentLimit = 8;

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
    metrics.tinyBlockerComponents =
        countSmallNaturalBlockerComponents(map, tinyComponentLimit);
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
