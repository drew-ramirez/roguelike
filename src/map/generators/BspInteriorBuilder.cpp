/**
 * **************************************************************************
 * @file     BspInteriorBuilder.cpp
 * @brief    Builds BSP interior maps from recursively split leaf rooms
 * **************************************************************************
 */

#include "map/generators/BspInteriorBuilder.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/TileType.hpp"
#include "map/corridors/CorridorBuilder.hpp"
#include "map/rooms/RoomCarver.hpp"

namespace rl {
namespace {

constexpr int minLeafSize = 8;
constexpr int maxSplitDepth = 5;

[[nodiscard]] std::optional<Point>
carveInteriorSubtree(Map& map,
                     const Rect& rect,
                     int depth,
                     Rng& rng,
                     std::vector<Rect>& rooms,
                     std::vector<CorridorPath>& corridors,
                     std::vector<MapSnapshot>& snapshots,
                     bool recordSnapshots)
{
    const bool canSplitHorizontal = rect.height() >= minLeafSize * 2;
    const bool canSplitVertical = rect.width() >= minLeafSize * 2;
    if (depth >= maxSplitDepth || (!canSplitHorizontal && !canSplitVertical)) {
        const int maxInsetX = std::max(2, std::min(3, (rect.width() - 3) / 2));
        const int maxInsetY = std::max(2, std::min(3, (rect.height() - 3) / 2));
        const int insetX = rng.rangeInclusive(2, maxInsetX);
        const int insetY = rng.rangeInclusive(2, maxInsetY);
        const Rect room{
            rect.x1 + insetX,
            rect.y1 + insetY,
            rect.x2 - insetX,
            rect.y2 - insetY,
        };
        if (room.width() < 3 || room.height() < 3) {
            return std::nullopt;
        }
        [[maybe_unused]] const auto carved = carveRectRoom(map, room);
        rooms.push_back(room);
        recordSnapshot(snapshots,
                       map,
                       "bsp interior room " + std::to_string(rooms.size()),
                       recordSnapshots);
        return randomInteriorPointInRoom(room, rng);
    }

    bool splitHorizontal = rng.coinFlip();
    if (rect.width() > rect.height() * 2) {
        splitHorizontal = false;
    } else if (rect.height() > rect.width() * 2) {
        splitHorizontal = true;
    }
    if (splitHorizontal && !canSplitHorizontal) {
        splitHorizontal = false;
    } else if (!splitHorizontal && !canSplitVertical) {
        splitHorizontal = true;
    }

    if (splitHorizontal) {
        const int split =
            rng.rangeInclusive(rect.y1 + minLeafSize, rect.y2 - minLeafSize);
        const auto first =
            carveInteriorSubtree(map,
                                 Rect{rect.x1, rect.y1, rect.x2, split},
                                 depth + 1,
                                 rng,
                                 rooms,
                                 corridors,
                                 snapshots,
                                 recordSnapshots);
        const auto second =
            carveInteriorSubtree(map,
                                 Rect{rect.x1, split, rect.x2, rect.y2},
                                 depth + 1,
                                 rng,
                                 rooms,
                                 corridors,
                                 snapshots,
                                 recordSnapshots);
        if (first.has_value() && second.has_value()) {
            corridors.push_back(carveCorridor(
                map, *first, *second, rng, CorridorStrategy::LShaped));
            return rng.coinFlip() ? first : second;
        }
        return first.has_value() ? first : second;
    }

    const int split =
        rng.rangeInclusive(rect.x1 + minLeafSize, rect.x2 - minLeafSize);
    const auto first =
        carveInteriorSubtree(map,
                             Rect{rect.x1, rect.y1, split, rect.y2},
                             depth + 1,
                             rng,
                             rooms,
                             corridors,
                             snapshots,
                             recordSnapshots);
    const auto second =
        carveInteriorSubtree(map,
                             Rect{split, rect.y1, rect.x2, rect.y2},
                             depth + 1,
                             rng,
                             rooms,
                             corridors,
                             snapshots,
                             recordSnapshots);
    if (first.has_value() && second.has_value()) {
        corridors.push_back(carveCorridor(
            map, *first, *second, rng, CorridorStrategy::LShaped));
        return rng.coinFlip() ? first : second;
    }
    return first.has_value() ? first : second;
}

[[nodiscard]] SpawnRegion walkableRoomRegion(const Map& map, const Rect& room)
{
    return spawnRegionFromTiles(walkableRoomTiles(map, room),
                                SpawnRegionKind::Room);
}

} // namespace

MapBuildResult BspInteriorBuilder::build(int width,
                                         int height,
                                         Rng& rng,
                                         MapBuildOptions options)
{
    validateBuilderDimensions("BspInteriorBuilder", width, height);

    Map map{width, height, TileType::Wall};
    std::vector<Rect> rooms;
    std::vector<MapSnapshot> snapshots;
    std::vector<CorridorPath> corridorPaths;
    [[maybe_unused]] const auto representative =
        carveInteriorSubtree(map,
                             Rect{1, 1, width - 1, height - 1},
                             0,
                             rng,
                             rooms,
                             corridorPaths,
                             snapshots,
                             options.recordSnapshots);

    if (rooms.size() < 2) {
        throw std::runtime_error(
            "BSP interior generation failed to place enough rooms");
    }

    std::vector<SpawnRegion> spawnRegions;
    spawnRegions.reserve(rooms.size());
    for (const Rect& room : rooms) {
        auto region = walkableRoomRegion(map, room);
        if (!region.tiles.empty()) {
            spawnRegions.push_back(std::move(region));
        }
    }

    const Point playerStart = rooms.front().center();
    const Point stairsPosition = rooms.back().center();
    map.setTile(stairsPosition.x, stairsPosition.y, TileType::DownStairs);
    recordSnapshot(
        snapshots, map, "bsp interior stairs", options.recordSnapshots);

    MapBuildResult result{std::move(map),
                          playerStart,
                          stairsPosition,
                          std::move(rooms),
                          std::move(spawnRegions),
                          std::move(snapshots),
                          "bsp-interior",
                          std::move(corridorPaths)};
    addBuildStep(result, "base: bsp interior leaf rooms");
    addBuildStep(result, "connector: sibling subtrees");

    validateBuildOrThrow(result,
                         "BSP interior generation produced an invalid map");

    return result;
}

} // namespace rl
