/**
 * **************************************************************************
 * @file     BuilderChain.cpp
 * @brief    Implements prefab, vault, door, and hybrid builder chains
 * **************************************************************************
 */

#include "map/generators/BuilderChain.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "map/prefabs/Prefab.hpp"

namespace rl {
namespace {

[[nodiscard]] bool isDoorCandidate(const Map& map, Point point)
{
    if (!map.isWalkable(point.x, point.y)) {
        return false;
    }

    // A generated door should sit in a one-tile choke: walls on one axis and
    // walkable tiles on the other, that keeps doors from appearing in open
    // room interiors or solid wall clusters
    const bool horizontalWalls =
        map.tileAt(point.x - 1, point.y) == TileType::Wall &&
        map.tileAt(point.x + 1, point.y) == TileType::Wall;
    const bool verticalFloors = map.isWalkable(point.x, point.y - 1) &&
                                map.isWalkable(point.x, point.y + 1);
    const bool verticalWalls =
        map.tileAt(point.x, point.y - 1) == TileType::Wall &&
        map.tileAt(point.x, point.y + 1) == TileType::Wall;
    const bool horizontalFloors = map.isWalkable(point.x - 1, point.y) &&
                                  map.isWalkable(point.x + 1, point.y);
    return (horizontalWalls && verticalFloors) ||
           (verticalWalls && horizontalFloors);
}

[[nodiscard]] bool isProtectedDoorTile(const MapBuildResult& result,
                                       Point point)
{
    if (point == result.playerStart || point == result.stairsPosition) {
        return true;
    }

    return std::any_of(result.spawnRequests.begin(),
                       result.spawnRequests.end(),
                       [point](const SpawnRequest& request) {
                           return request.position == point;
                       });
}

[[nodiscard]] bool tryPlaceDoor(MapBuildResult& result, Point point)
{
    if (isProtectedDoorTile(result, point) ||
        !isDoorCandidate(result.map, point)) {
        return false;
    }

    result.map.setTile(point.x, point.y, TileType::ClosedDoor);
    return true;
}

void validateHybrid(const MapBuildResult& result)
{
    validateBuildOrThrow(result, "Hybrid chain produced an invalid map");
}

} // namespace

void placeDoors(MapBuildResult& result,
                Rng& rng,
                MapBuildOptions options,
                bool allowFallbackScan)
{
    int placed = 0;
    if (!result.corridorPaths.empty()) {
        for (const auto& path : result.corridorPaths) {
            if (path.tiles.size() <= 2) {
                continue;
            }

            for (const Point point : path.tiles) {
                if (tryPlaceDoor(result, point)) {
                    ++placed;
                    break;
                }
            }
        }
    } else if (allowFallbackScan) {
        for (int y = 1; y < result.map.height() - 1; ++y) {
            for (int x = 1; x < result.map.width() - 1; ++x) {
                if (rng.rangeInclusive(1, 3) != 1) {
                    continue;
                }
                if (tryPlaceDoor(result, Point{x, y})) {
                    ++placed;
                }
            }
        }
    }

    if (placed > 0) {
        recordSnapshot(result.snapshots,
                       result.map,
                       "placed doors",
                       options.recordSnapshots);
    }
}

MapBuildResult BuilderChain::buildCavernVaults(int width,
                                               int height,
                                               Rng& rng,
                                               MapBuildOptions options)
{
    auto result = CellularAutomataBuilder::build(width, height, rng, options);
    result.builderName = "cavern-vaults";
    const auto vaults = vaultsForDepth(1);
    if (!vaults.empty()) {
        const auto& vault = vaults[static_cast<std::size_t>(
            rng.rangeInclusive(0, static_cast<int>(vaults.size()) - 1))];
        [[maybe_unused]] const auto placed =
            placePrefab(result, vault.prefab, rng, options);
    }
    repairConnectivity(result.map, result.playerStart, rng);
    placeDoors(result, rng, options, false);
    validateHybrid(result);
    return result;
}

MapBuildResult BuilderChain::buildRuinedRooms(int width,
                                              int height,
                                              Rng& rng,
                                              MapBuildOptions options)
{
    auto result = SimpleMapBuilder::build(width, height, rng, options);
    result.builderName = "ruined-rooms";
    [[maybe_unused]] const auto placed =
        placePrefab(result, shrinePrefab(), rng, options);

    for (const auto& room : result.rooms) {
        if (room.width() < 7 || room.height() < 7 ||
            rng.rangeInclusive(0, 2) != 0) {
            continue;
        }
        const Point rubble{rng.rangeInclusive(room.x1 + 1, room.x2 - 2),
                           rng.rangeInclusive(room.y1 + 1, room.y2 - 2)};
        if (rubble != result.playerStart && rubble != result.stairsPosition) {
            result.map.setTile(rubble.x, rubble.y, TileType::Wall);
        }
    }

    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    placeDoors(result, rng, options);
    recordSnapshot(
        result.snapshots, result.map, "ruined rooms", options.recordSnapshots);
    validateHybrid(result);
    return result;
}

} // namespace rl
