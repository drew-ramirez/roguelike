/**
 * **************************************************************************
 * @file     CellularAutomataBuilder.cpp
 * @brief    Builds cave maps by cellular automata smoothing
 * **************************************************************************
 */

#include "map/generators/CellularAutomataBuilder.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int initialWallChance = 55;
constexpr int smoothPasses = 15;

[[nodiscard]] int wallNeighborCount(const Map& map, int x, int y)
{
    int count = 0;
    for (int yy = y - 1; yy <= y + 1; ++yy) {
        for (int xx = x - 1; xx <= x + 1; ++xx) {
            if (xx == x && yy == y) {
                continue;
            }
            if (!map.inBounds(xx, yy) || map.tileAt(xx, yy) == TileType::Wall) {
                ++count;
            }
        }
    }
    return count;
}

void smooth(Map& map)
{
    Map next{map.width(), map.height(), TileType::Wall};
    for (int y = 1; y < map.height() - 1; ++y) {
        for (int x = 1; x < map.width() - 1; ++x) {
            const int neighbors = wallNeighborCount(map, x, y);
            // dense wall neighborhoods stay walls, while a zero-wall
            // neighborhood becomes a wall island to break huge empty blobs
            next.setTile(x,
                         y,
                         neighbors > 4 || neighbors == 0 ? TileType::Wall
                                                         : TileType::Floor);
        }
    }
    map = std::move(next);
}

} // namespace

MapBuildResult CellularAutomataBuilder::build(int width,
                                              int height,
                                              Rng& rng,
                                              MapBuildOptions options)
{
    validateBuilderDimensions("CellularAutomataBuilder", width, height);

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "cellular";
    addBuildStep(result, "base: cellular automata");

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            result.map.setTile(x,
                               y,
                               rng.rangeInclusive(1, 100) <= initialWallChance
                                   ? TileType::Wall
                                   : TileType::Floor);
        }
    }
    recordSnapshot(
        result.snapshots, result.map, "random cave", options.recordSnapshots);

    for (int pass = 0; pass < smoothPasses; ++pass) {
        smooth(result.map);
        recordSnapshot(result.snapshots,
                       result.map,
                       "smoothed cave " + std::to_string(pass + 1),
                       options.recordSnapshots);
    }

    auto start =
        closestReachableFloorTo(result.map, Point{width / 2, height / 2});
    if (!start.has_value()) {
        result.map.setTile(width / 2, height / 2, TileType::Floor);
        start = Point{width / 2, height / 2};
    }
    result.playerStart = *start;
    repairConnectivity(result.map, result.playerStart, rng);
    sealMapEdges(result.map);
    cullUnreachable(result.map, result.playerStart);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    addBuildStep(result, "modifier: connectivity repair");
    addBuildStep(result, "modifier: distant exit");
    recordSnapshot(result.snapshots,
                   result.map,
                   "validated cave",
                   options.recordSnapshots);

    validateBuildOrThrow(result, "Cellular generation produced an invalid map");

    return result;
}

} // namespace rl
