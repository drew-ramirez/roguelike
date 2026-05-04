/**
 * **************************************************************************
 * @file     DrunkardsWalkBuilder.cpp
 * @brief    Builds cave maps with constrained random walkers
 * **************************************************************************
 */

#include "map/generators/DrunkardsWalkBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "map/MapAlgorithms.hpp"
#include "map/MapBrush.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int minMapWidth = 20;
constexpr int minMapHeight = 15;

[[nodiscard]] int countFloors(const Map& map)
{
    return static_cast<int>(floorTiles(map).size());
}

[[nodiscard]] Point randomStep(Point point, Rng& rng)
{
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

} // namespace

MapBuildResult DrunkardsWalkBuilder::build(int width,
                                           int height,
                                           Rng& rng,
                                           MapBuildOptions options)
{
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument(
            "DrunkardsWalkBuilder requires at least a 20x15 map");
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "drunkard";
    result.playerStart = Point{width / 2, height / 2};
    result.map.setTile(
        result.playerStart.x, result.playerStart.y, TileType::Floor);
    addBuildStep(result, "base: drunkard walk");

    const int desiredFloors = (width * height) / 3;
    int walkerCount = 0;
    while (countFloors(result.map) < desiredFloors && walkerCount < 80) {
        Point walker = result.playerStart;
        if (walkerCount > 0 && rng.rangeInclusive(0, 2) == 0) {
            walker = Point{rng.rangeInclusive(2, width - 3),
                           rng.rangeInclusive(2, height - 3)};
        }

        constexpr int walkLength = 420;
        for (int step = 0; step < walkLength; ++step) {
            walker.x = std::clamp(walker.x, 2, width - 3);
            walker.y = std::clamp(walker.y, 2, height - 3);
            const BrushShape brush = rng.rangeInclusive(0, 9) == 0
                                         ? BrushShape::Circle
                                         : BrushShape::Single;
            paintBrush(result.map, walker, TileType::Floor, brush, 1);
            walker = randomStep(walker, rng);
        }

        ++walkerCount;
        recordSnapshot(result.snapshots,
                       result.map,
                       "walker " + std::to_string(walkerCount),
                       options.recordSnapshots);
    }

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
        throw std::runtime_error("Drunkard generation produced an invalid map");
    }

    return result;
}

} // namespace rl
