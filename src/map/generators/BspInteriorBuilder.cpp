/**
 * **************************************************************************
 * @file     BspInteriorBuilder.cpp
 * @brief    Builds BSP interior maps from recursively split leaf rooms
 * **************************************************************************
 */

#include "map/generators/BspInteriorBuilder.hpp"

#include <algorithm>
#include <cstddef>
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

void splitRect(const Rect& rect, int depth, Rng& rng, std::vector<Rect>& leaves)
{
    const bool canSplitHorizontal = rect.height() >= minLeafSize * 2;
    const bool canSplitVertical = rect.width() >= minLeafSize * 2;
    if (depth >= maxSplitDepth || (!canSplitHorizontal && !canSplitVertical)) {
        leaves.push_back(rect);
        return;
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
        splitRect(
            Rect{rect.x1, rect.y1, rect.x2, split}, depth + 1, rng, leaves);
        splitRect(
            Rect{rect.x1, split, rect.x2, rect.y2}, depth + 1, rng, leaves);
        return;
    }

    const int split =
        rng.rangeInclusive(rect.x1 + minLeafSize, rect.x2 - minLeafSize);
    splitRect(Rect{rect.x1, rect.y1, split, rect.y2}, depth + 1, rng, leaves);
    splitRect(Rect{split, rect.y1, rect.x2, rect.y2}, depth + 1, rng, leaves);
}

[[nodiscard]] Rect leafRoom(const Rect& leaf)
{
    return Rect{leaf.x1 + 1, leaf.y1 + 1, leaf.x2 - 1, leaf.y2 - 1};
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
    std::vector<Rect> leaves;
    std::vector<Rect> rooms;
    std::vector<MapSnapshot> snapshots;
    splitRect(Rect{1, 1, width - 1, height - 1}, 0, rng, leaves);

    for (const Rect& leaf : leaves) {
        const Rect room = leafRoom(leaf);
        if (room.width() < 3 || room.height() < 3) {
            continue;
        }

        [[maybe_unused]] const auto carved = carveRectRoom(map, room);
        rooms.push_back(room);
        recordSnapshot(snapshots,
                       map,
                       "bsp interior room " + std::to_string(rooms.size()),
                       options.recordSnapshots);
    }

    if (rooms.size() < 2) {
        throw std::runtime_error(
            "BSP interior generation failed to place enough rooms");
    }

    std::sort(
        rooms.begin(), rooms.end(), [](const Rect& left, const Rect& right) {
            if (left.x1 == right.x1) {
                return left.y1 < right.y1;
            }
            return left.x1 < right.x1;
        });

    std::vector<CorridorPath> corridorPaths;
    for (std::size_t index = 1; index < rooms.size(); ++index) {
        auto path =
            carveCorridor(map,
                          randomInteriorPointInRoom(rooms[index - 1], rng),
                          randomInteriorPointInRoom(rooms[index], rng),
                          rng,
                          CorridorStrategy::LShaped);
        corridorPaths.push_back(std::move(path));
        recordSnapshot(snapshots,
                       map,
                       "bsp interior corridor " + std::to_string(index),
                       options.recordSnapshots);
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
    addBuildStep(result, "connector: sorted rooms with random interior points");

    validateBuildOrThrow(result,
                         "BSP interior generation produced an invalid map");

    return result;
}

} // namespace rl
