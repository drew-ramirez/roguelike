/**
 * **************************************************************************
 * @file     BspDungeonBuilder.cpp
 * @brief    Builds connected dungeons from BSP-style random sub-rectangles
 * **************************************************************************
 */

#include "map/generators/BspDungeonBuilder.hpp"

#include <algorithm>
#include <array>
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

constexpr int minLeafSize = 9;
constexpr int maxSplitDepth = 5;
constexpr int minRoomSize = 3;

[[nodiscard]] Rect randomRoomInLeaf(const Rect& leaf, Rng& rng)
{
    const int left = rng.rangeInclusive(1, std::min(3, leaf.width() - 4));
    const int right =
        rng.rangeInclusive(1, std::min(3, leaf.width() - left - 3));
    const int top = rng.rangeInclusive(1, std::min(3, leaf.height() - 4));
    const int bottom =
        rng.rangeInclusive(1, std::min(3, leaf.height() - top - 3));
    return Rect{
        leaf.x1 + left, leaf.y1 + top, leaf.x2 - right, leaf.y2 - bottom};
}

[[nodiscard]] std::optional<Point>
carveSubtree(Map& map,
             const Rect& bounds,
             int depth,
             Rng& rng,
             std::vector<Rect>& rooms,
             std::vector<CorridorPath>& corridors,
             std::vector<MapSnapshot>& snapshots,
             bool recordSnapshots)
{
    const bool canSplitHorizontal = bounds.height() >= minLeafSize * 2;
    const bool canSplitVertical = bounds.width() >= minLeafSize * 2;
    if (depth < maxSplitDepth && (canSplitHorizontal || canSplitVertical)) {
        bool horizontal = rng.coinFlip();
        if (!canSplitHorizontal ||
            (canSplitVertical && bounds.width() > bounds.height() * 3 / 2)) {
            horizontal = false;
        } else if (!canSplitVertical ||
                   bounds.height() > bounds.width() * 3 / 2) {
            horizontal = true;
        }

        Rect first = bounds;
        Rect second = bounds;
        if (horizontal) {
            const int split = rng.rangeInclusive(bounds.y1 + minLeafSize,
                                                 bounds.y2 - minLeafSize);
            first.y2 = split;
            second.y1 = split;
        } else {
            const int split = rng.rangeInclusive(bounds.x1 + minLeafSize,
                                                 bounds.x2 - minLeafSize);
            first.x2 = split;
            second.x1 = split;
        }

        const auto firstPoint = carveSubtree(map,
                                             first,
                                             depth + 1,
                                             rng,
                                             rooms,
                                             corridors,
                                             snapshots,
                                             recordSnapshots);
        const auto secondPoint = carveSubtree(map,
                                              second,
                                              depth + 1,
                                              rng,
                                              rooms,
                                              corridors,
                                              snapshots,
                                              recordSnapshots);
        if (firstPoint.has_value() && secondPoint.has_value()) {
            corridors.push_back(carveCorridor(map,
                                              *firstPoint,
                                              *secondPoint,
                                              rng,
                                              CorridorStrategy::LShaped));
            recordSnapshot(snapshots,
                           map,
                           "bsp sibling connection " +
                               std::to_string(corridors.size()),
                           recordSnapshots);
            return rng.coinFlip() ? firstPoint : secondPoint;
        }
        return firstPoint.has_value() ? firstPoint : secondPoint;
    }

    const Rect room = randomRoomInLeaf(bounds, rng);
    if (room.width() < minRoomSize || room.height() < minRoomSize) {
        return std::nullopt;
    }
    [[maybe_unused]] const auto carved = carveRectRoom(map, room);
    rooms.push_back(room);
    recordSnapshot(snapshots,
                   map,
                   "bsp leaf room " + std::to_string(rooms.size()),
                   recordSnapshots);
    return randomInteriorPointInRoom(room, rng);
}

[[nodiscard]] SpawnRegion roomSpawnRegion(const Rect& room)
{
    return spawnRegionFromTiles(roomTiles(room), SpawnRegionKind::Room);
}

} // namespace

MapBuildResult BspDungeonBuilder::build(int width,
                                        int height,
                                        Rng& rng,
                                        MapBuildOptions options)
{
    validateBuilderDimensions("BspDungeonBuilder", width, height);

    Map map{width, height, TileType::Wall};
    std::vector<Rect> rooms;
    std::vector<MapSnapshot> snapshots;
    std::vector<CorridorPath> corridorPaths;
    [[maybe_unused]] const auto representative =
        carveSubtree(map,
                     Rect{1, 1, width - 1, height - 1},
                     0,
                     rng,
                     rooms,
                     corridorPaths,
                     snapshots,
                     options.recordSnapshots);

    if (rooms.size() < 2) {
        throw std::runtime_error("BSP generation failed to place enough rooms");
    }

    std::vector<SpawnRegion> spawnRegions;
    spawnRegions.reserve(rooms.size());
    for (const Rect& room : rooms) {
        spawnRegions.push_back(roomSpawnRegion(room));
    }

    const Point playerStart = rooms.front().center();
    const Point stairsPosition = rooms.back().center();
    map.setTile(stairsPosition.x, stairsPosition.y, TileType::DownStairs);
    recordSnapshot(snapshots, map, "bsp stairs", options.recordSnapshots);

    MapBuildResult result{std::move(map),
                          playerStart,
                          stairsPosition,
                          std::move(rooms),
                          std::move(spawnRegions),
                          std::move(snapshots),
                          "bsp",
                          std::move(corridorPaths)};

    validateBuildOrThrow(result, "BSP generation produced an invalid map");

    return result;
}

} // namespace rl
