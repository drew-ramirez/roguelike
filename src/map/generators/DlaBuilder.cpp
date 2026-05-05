/**
 * **************************************************************************
 * @file     DlaBuilder.cpp
 * @brief    Builds organic caves with diffusion-limited aggregation
 * **************************************************************************
 */

#include "map/generators/DlaBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "map/MapBuilderCommon.hpp"
#include "map/MapBrush.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

[[nodiscard]] bool touchesFloor(const Map& map, Point point)
{
    for (const Point neighbor : cardinalNeighbors(point)) {
        if (map.inBounds(neighbor.x, neighbor.y) &&
            map.isWalkable(neighbor.x, neighbor.y)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Point randomMapEdge(int width, int height, Rng& rng)
{
    switch (rng.rangeInclusive(0, 3)) {
    case 0:
        return Point{rng.rangeInclusive(1, width - 2), 1};
    case 1:
        return Point{rng.rangeInclusive(1, width - 2), height - 2};
    case 2:
        return Point{1, rng.rangeInclusive(1, height - 2)};
    default:
        return Point{width - 2, rng.rangeInclusive(1, height - 2)};
    }
}

[[nodiscard]] Point stepTowardCenter(Point point, Point center, Rng& rng)
{
    if (rng.rangeInclusive(0, 4) == 0) {
        switch (rng.rangeInclusive(0, 3)) {
        case 0:
            return Point{point.x + 1, point.y};
        case 1:
            return Point{point.x - 1, point.y};
        case 2:
            return Point{point.x, point.y + 1};
        default:
            return Point{point.x, point.y - 1};
        }
    }

    if (rng.coinFlip() && point.x != center.x) {
        point.x += point.x < center.x ? 1 : -1;
    } else if (point.y != center.y) {
        point.y += point.y < center.y ? 1 : -1;
    }
    return point;
}

} // namespace

MapBuildResult
DlaBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument("DlaBuilder requires at least a 20x15 map");
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "dla";
    result.playerStart = Point{width / 2, height / 2};
    paintBrush(result.map,
               result.playerStart,
               TileType::Floor,
               BrushShape::Circle,
               2,
               Symmetry::None);
    addBuildStep(result, "base: diffusion-limited aggregation");

    const int targetFloors = (width * height) / 4;
    int accepted = 0;
    while (static_cast<int>(floorTiles(result.map).size()) < targetFloors &&
           accepted < targetFloors * 3) {
        Point walker = randomMapEdge(width, height, rng);
        constexpr int maxWalkerSteps = 120;
        for (int step = 0; step < maxWalkerSteps; ++step) {
            walker.x = std::clamp(walker.x, 1, width - 2);
            walker.y = std::clamp(walker.y, 1, height - 2);
            if (touchesFloor(result.map, walker)) {
                paintBrush(result.map,
                           walker,
                           TileType::Floor,
                           BrushShape::Circle,
                           1,
                           rng.coinFlip() ? Symmetry::None : Symmetry::Quad);
                ++accepted;
                break;
            }
            walker = stepTowardCenter(walker, result.playerStart, rng);
        }
    }

    recordSnapshot(
        result.snapshots, result.map, "grown dla", options.recordSnapshots);
    repairConnectivity(result.map, result.playerStart, rng);
    sealMapEdges(result.map);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    addBuildStep(result, "modifier: connectivity repair");
    addBuildStep(result, "modifier: distant exit");

    const auto validation =
        validateMapBuild(result.map, result.playerStart, result.stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error("DLA generation produced an invalid map");
    }

    return result;
}

} // namespace rl
