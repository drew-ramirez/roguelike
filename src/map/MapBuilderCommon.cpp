/**
 * **************************************************************************
 * @file     MapBuilderCommon.cpp
 * @brief    Implements shared map builder finalization helpers
 * **************************************************************************
 */

#include "map/MapBuilderCommon.hpp"

#include <stdexcept>
#include <utility>

#include "map/Map.hpp"
#include "map/MapConnectivity.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {

void validateBuilderDimensions(std::string_view builderName,
                               int width,
                               int height,
                               int minWidth,
                               int minHeight)
{
    if (width < minWidth || height < minHeight) {
        throw std::invalid_argument(std::string{builderName} +
                                    " requires at least a " +
                                    std::to_string(minWidth) + "x" +
                                    std::to_string(minHeight) + " map");
    }
}

void sealMapEdges(Map& map, TileType tile)
{
    for (int x = 0; x < map.width(); ++x) {
        map.setTile(x, 0, tile);
        map.setTile(x, map.height() - 1, tile);
    }
    for (int y = 0; y < map.height(); ++y) {
        map.setTile(0, y, tile);
        map.setTile(map.width() - 1, y, tile);
    }
}

void setDistantStairs(Map& map, Point start, Point& stairsPosition)
{
    const auto farthest = farthestReachableFloor(map, start);
    if (!farthest.has_value()) {
        throw std::runtime_error("Unable to find a reachable stairs position");
    }

    stairsPosition = *farthest;
    map.setTile(stairsPosition.x, stairsPosition.y, TileType::DownStairs);
}

void setUpStairsAtStart(Map& map, Point start, Point& upStairsPosition)
{
    if (!map.inBounds(start.x, start.y) || !map.isWalkable(start.x, start.y)) {
        throw std::runtime_error("Unable to place up stairs at level start");
    }

    upStairsPosition = start;
    map.setTile(upStairsPosition.x, upStairsPosition.y, TileType::UpStairs);
}

void recordSnapshot(std::vector<MapSnapshot>& snapshots,
                    const Map& map,
                    std::string label,
                    bool enabled)
{
    if (enabled) {
        snapshots.push_back(MapSnapshot{std::move(label), map});
    }
}

void validateBuildOrThrow(const MapBuildResult& result,
                          std::string_view failureMessage)
{
    const auto validation =
        validateMapBuild(result.map, result.playerStart, result.stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error(std::string{failureMessage});
    }
}

std::vector<SpawnRegion> spawnRegionsFromConnectedFloors(const Map& map,
                                                         SpawnRegionKind kind)
{
    std::vector<SpawnRegion> regions;
    for (auto& region : findConnectedRegions(map)) {
        std::vector<Point> floors;
        for (const Point point : region.tiles) {
            if (map.isWalkable(point.x, point.y)) {
                floors.push_back(point);
            }
        }
        if (!floors.empty()) {
            regions.push_back(spawnRegionFromTiles(std::move(floors), kind));
        }
    }
    return regions;
}

SpawnRegion spawnRegionFromTiles(std::vector<Point> tiles, SpawnRegionKind kind)
{
    SpawnRegion region;
    region.kind = kind;
    region.tiles = std::move(tiles);
    return region;
}

} // namespace rl
