/**
 * **************************************************************************
 * @file     Section4Builders.cpp
 * @brief    Builds authored gameplay region maps
 * **************************************************************************
 */

#include "map/generators/Section4Builders.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/Rect.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

void carveRect(Map& map, const Rect& rect)
{
    for (int y = rect.y1; y < rect.y2; ++y) {
        for (int x = rect.x1; x < rect.x2; ++x) {
            if (map.inBounds(x, y)) {
                map.setTile(x, y, TileType::Floor);
            }
        }
    }
}

void carveHorizontal(Map& map, int y, int x1, int x2)
{
    const int begin = std::max(1, std::min(x1, x2));
    const int end = std::min(map.width() - 2, std::max(x1, x2));
    for (int x = begin; x <= end; ++x) {
        map.setTile(x, y, TileType::Floor);
    }
}

void carveVertical(Map& map, int x, int y1, int y2)
{
    const int begin = std::max(1, std::min(y1, y2));
    const int end = std::min(map.height() - 2, std::max(y1, y2));
    for (int y = begin; y <= end; ++y) {
        map.setTile(x, y, TileType::Floor);
    }
}

void finalizeRegion(MapBuildResult& result,
                    std::string builderName,
                    bool snapshots)
{
    result.builderName = std::move(builderName);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    recordSnapshot(result.snapshots, result.map, "town final", snapshots);
    if (!result.map.isWalkable(result.playerStart.x, result.playerStart.y) ||
        !result.map.isWalkable(result.stairsPosition.x,
                               result.stairsPosition.y)) {
        throw std::runtime_error("Town builder failed validation");
    }
}

} // namespace

MapBuildResult Section4Builders::buildTown(int width,
                                           int height,
                                           [[maybe_unused]] Rng& rng,
                                           MapBuildOptions options)
{
    validateBuilderDimensions("town", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    const int midX = width / 2;
    const int midY = height / 2;

    carveHorizontal(result.map, midY, 1, width - 2);
    carveVertical(result.map, midX, 1, height - 2);
    carveRect(result.map, Rect{3, 3, 12, 9});
    carveRect(result.map, Rect{width - 13, 3, width - 4, 9});
    carveRect(result.map, Rect{3, height - 10, 13, height - 4});
    carveRect(result.map, Rect{width - 15, height - 10, width - 4, height - 4});
    carveRect(result.map, Rect{midX - 4, midY - 3, midX + 5, midY + 4});
    carveHorizontal(result.map, 6, 11, midX);
    carveHorizontal(result.map, 6, midX, width - 13);
    carveHorizontal(result.map, height - 7, 12, midX);
    carveHorizontal(result.map, height - 7, midX, width - 15);
    sealMapEdges(result.map);

    result.playerStart = Point{midX, midY};
    finalizeRegion(result, "town", options.recordSnapshots);
    return result;
}

} // namespace rl
