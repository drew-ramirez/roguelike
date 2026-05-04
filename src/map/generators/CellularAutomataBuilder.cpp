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

#include "map/MapAlgorithms.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int minMapWidth = 20;
constexpr int minMapHeight = 15;
constexpr int initialWallChance = 45;
constexpr int smoothPasses = 5;

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
            next.setTile(x,
                         y,
                         wallNeighborCount(map, x, y) > 4 ? TileType::Wall
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
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument(
            "CellularAutomataBuilder requires at least a 20x15 map");
    }

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

    const auto validation =
        validateMapBuild(result.map, result.playerStart, result.stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error("Cellular generation produced an invalid map");
    }

    return result;
}

} // namespace rl
