/**
 * **************************************************************************
 * @file     BspInteriorBuilder.cpp
 * @brief    Adds room-interior variation to the base BSP dungeon builder
 * **************************************************************************
 */

#include "map/generators/BspInteriorBuilder.hpp"

#include <stdexcept>
#include <utility>

#include "map/MapValidation.hpp"
#include "map/TileType.hpp"
#include "map/generators/BspDungeonBuilder.hpp"

namespace rl {
namespace {

void addPillars(Map& map, const Rect& room, Point start, Point stairs)
{
    if (room.width() < 8 || room.height() < 8) {
        return;
    }

    for (int y = room.y1 + 2; y < room.y2 - 2; y += 3) {
        for (int x = room.x1 + 2; x < room.x2 - 2; x += 3) {
            const Point point{x, y};
            if (point == start || point == stairs) {
                continue;
            }
            map.setTile(x, y, TileType::Wall);
        }
    }
}

void addInteriorVariation(MapBuildResult& result, bool recordSnapshots)
{
    for (const auto& room : result.rooms) {
        addPillars(result.map, room, result.playerStart, result.stairsPosition);
    }

    result.spawnRegions.clear();
    for (const auto& room : result.rooms) {
        SpawnRegion region;
        region.kind = SpawnRegionKind::Room;
        for (int y = room.y1; y < room.y2; ++y) {
            for (int x = room.x1; x < room.x2; ++x) {
                if (result.map.isWalkable(x, y)) {
                    region.tiles.push_back(Point{x, y});
                }
            }
        }
        result.spawnRegions.push_back(std::move(region));
    }

    if (recordSnapshots) {
        result.snapshots.push_back(
            MapSnapshot{"bsp interior pillars", result.map});
    }
}

} // namespace

MapBuildResult BspInteriorBuilder::build(int width,
                                         int height,
                                         Rng& rng,
                                         MapBuildOptions options)
{
    MapBuildResult result =
        BspDungeonBuilder::build(width, height, rng, options);
    addInteriorVariation(result, options.recordSnapshots);
    result.builderName = "bsp-interior";

    const auto validation =
        validateMapBuild(result.map, result.playerStart, result.stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error(
            "BSP interior generation produced an invalid map");
    }

    return result;
}

} // namespace rl
