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

constexpr int maxRoomAttempts = 240;
constexpr int maxRooms = 20;
constexpr int minRoomSize = 3;
constexpr int maxRoomSize = 10;

void addSubRects(const Rect& rect, std::vector<Rect>& rects)
{
    if (rect.width() < minRoomSize * 2 || rect.height() < minRoomSize * 2) {
        return;
    }

    const int halfWidth = std::max(1, rect.width() / 2);
    const int halfHeight = std::max(1, rect.height() / 2);
    const int midX = rect.x1 + halfWidth;
    const int midY = rect.y1 + halfHeight;

    const std::array<Rect, 4> children{
        Rect{rect.x1, rect.y1, midX, midY},
        Rect{midX, rect.y1, rect.x2, midY},
        Rect{rect.x1, midY, midX, rect.y2},
        Rect{midX, midY, rect.x2, rect.y2},
    };
    for (const Rect& child : children) {
        if (child.width() >= minRoomSize && child.height() >= minRoomSize) {
            rects.push_back(child);
        }
    }
}

[[nodiscard]] std::optional<Rect> randomRoomInRect(const Rect& rect, Rng& rng)
{
    if (rect.width() < minRoomSize || rect.height() < minRoomSize) {
        return std::nullopt;
    }

    const int maxWidth = std::min(maxRoomSize, rect.width());
    const int maxHeight = std::min(maxRoomSize, rect.height());
    const int width = rng.rangeInclusive(minRoomSize, maxWidth);
    const int height = rng.rangeInclusive(minRoomSize, maxHeight);
    const int x = rng.rangeInclusive(rect.x1, rect.x2 - width);
    const int y = rng.rangeInclusive(rect.y1, rect.y2 - height);
    return Rect{x, y, x + width, y + height};
}

[[nodiscard]] bool
roomFits(const Map& map, const Rect& room, const std::vector<Rect>& rooms)
{
    if (room.x1 < 1 || room.y1 < 1 || room.x2 > map.width() - 1 ||
        room.y2 > map.height() - 1) {
        return false;
    }

    const Rect buffered{room.x1 - 1, room.y1 - 1, room.x2 + 1, room.y2 + 1};
    return std::none_of(rooms.begin(), rooms.end(), [&buffered](const Rect& r) {
        return buffered.intersects(r);
    });
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
    std::vector<Rect> rects{Rect{2, 2, width - 2, height - 2}};
    std::vector<Rect> rooms;
    std::vector<MapSnapshot> snapshots;
    addSubRects(rects.front(), rects);

    for (int attempt = 0;
         attempt < maxRoomAttempts && rooms.size() < maxRooms && !rects.empty();
         ++attempt) {
        const Rect partition = rects[static_cast<std::size_t>(
            rng.rangeInclusive(0, static_cast<int>(rects.size()) - 1))];
        const auto candidate = randomRoomInRect(partition, rng);
        if (!candidate.has_value() || !roomFits(map, *candidate, rooms)) {
            continue;
        }

        [[maybe_unused]] const auto carved = carveRectRoom(map, *candidate);
        rooms.push_back(*candidate);
        addSubRects(*candidate, rects);
        recordSnapshot(snapshots,
                       map,
                       "bsp room " + std::to_string(rooms.size()),
                       options.recordSnapshots);
    }

    if (rooms.size() < 2) {
        throw std::runtime_error("BSP generation failed to place enough rooms");
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
                       "bsp corridor " + std::to_string(index),
                       options.recordSnapshots);
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
    addBuildStep(result, "base: bsp random subrects");
    addBuildStep(result, "connector: sorted rooms with random interior points");

    validateBuildOrThrow(result, "BSP generation produced an invalid map");

    return result;
}

} // namespace rl
