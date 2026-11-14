/**
 * **************************************************************************
 * @file     SimpleMapBuilder.cpp
 * @brief    Builds deterministic rooms-and-corridors dungeon maps
 * **************************************************************************
 */

#include "map/generators/SimpleMapBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"
#include "map/corridors/CorridorBuilder.hpp"
#include "map/rooms/RoomCarver.hpp"

namespace rl {
namespace {

constexpr int maxRooms = 30;
constexpr int minRoomSize = 6;
constexpr int maxRoomSize = 10;
constexpr int minAcceptedRooms = 2;

bool intersectsAnyRoom(const Rect& candidate, const std::vector<Rect>& rooms)
{
    return std::any_of(
        rooms.begin(), rooms.end(), [&candidate](const Rect& room) {
            return candidate.intersects(room);
        });
}

[[nodiscard]] SpawnRegion roomSpawnRegion(std::vector<Point> tiles)
{
    return spawnRegionFromTiles(std::move(tiles), SpawnRegionKind::Room);
}

[[nodiscard]] SpawnRegion corridorSpawnRegion(const CorridorPath& path)
{
    return spawnRegionFromTiles(path.tiles, SpawnRegionKind::Corridor);
}

} // namespace

MapBuildResult SimpleMapBuilder::build(int width,
                                       int height,
                                       Rng& rng,
                                       MapBuildOptions options)
{
    validateBuilderDimensions("SimpleMapBuilder", width, height);

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "simple";
    result.rooms.reserve(maxRooms);
    addBuildStep(result, "base: mixed rooms");

    for (int attempt = 0; attempt < maxRooms; ++attempt) {
        const int roomWidth = rng.rangeInclusive(minRoomSize, maxRoomSize);
        const int roomHeight = rng.rangeInclusive(minRoomSize, maxRoomSize);
        const int x = rng.rangeInclusive(1, width - roomWidth - 1);
        const int y = rng.rangeInclusive(1, height - roomHeight - 1);
        const Rect room{x, y, x + roomWidth, y + roomHeight};

        if (intersectsAnyRoom(room, result.rooms)) {
            continue;
        }

        const RoomShape shape =
            chooseRoomShape(room, rng.rangeInclusive(0, 99));
        auto carvedTiles = carveRoomShape(result.map, room, shape);
        if (!result.rooms.empty()) {
            const CorridorStrategy strategy =
                chooseCorridorStrategy(rng.rangeInclusive(0, 99));
            auto path = carveCorridor(result.map,
                                      result.rooms.back().center(),
                                      room.center(),
                                      rng,
                                      strategy);
            result.spawnRegions.push_back(corridorSpawnRegion(path));
            result.corridorPaths.push_back(std::move(path));
            recordSnapshot(result.snapshots,
                           result.map,
                           "connected room " +
                               std::to_string(result.rooms.size() + 1),
                           options.recordSnapshots);
        }

        result.spawnRegions.push_back(roomSpawnRegion(std::move(carvedTiles)));
        result.rooms.push_back(room);
        recordSnapshot(result.snapshots,
                       result.map,
                       "carved room " + std::to_string(result.rooms.size()),
                       options.recordSnapshots);
    }

    if (static_cast<int>(result.rooms.size()) < minAcceptedRooms) {
        throw std::runtime_error("Map generation failed to place enough rooms");
    }

    result.playerStart = result.rooms.front().center();
    if (!result.map.isWalkable(result.playerStart.x, result.playerStart.y)) {
        const auto closest =
            closestWalkableFloorTo(result.map, result.playerStart);
        if (!closest.has_value()) {
            throw std::runtime_error(
                "Map generation placed the player start in a wall");
        }
        result.playerStart = *closest;
    }

    const int repaired =
        repairConnectivity(result.map, result.playerStart, rng);
    if (repaired > 0) {
        addBuildStep(result, "modifier: connectivity repair");
        recordSnapshot(result.snapshots,
                       result.map,
                       "repaired connectivity",
                       options.recordSnapshots);
    }

    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    recordSnapshot(
        result.snapshots, result.map, "placed stairs", options.recordSnapshots);
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "Simple generation produced an invalid map");

    return result;
}

} // namespace rl
