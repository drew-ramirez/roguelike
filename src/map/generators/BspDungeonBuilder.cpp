/**
 * **************************************************************************
 * @file     BspDungeonBuilder.cpp
 * @brief    Builds connected dungeons from recursively split partitions
 * **************************************************************************
 */

#include "map/generators/BspDungeonBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int minMapWidth = 20;
constexpr int minMapHeight = 15;
constexpr int minLeafSize = 9;
constexpr int maxSplitDepth = 4;

void carveRoom(Map& map, const Rect& room)
{
    for (int y = room.y1; y < room.y2; ++y) {
        for (int x = room.x1; x < room.x2; ++x) {
            map.setTile(x, y, TileType::Floor);
        }
    }
}

void carveHorizontalTunnel(Map& map, int x1, int x2, int y)
{
    const auto [from, to] = std::minmax(x1, x2);
    for (int x = from; x <= to; ++x) {
        map.setTile(x, y, TileType::Floor);
    }
}

void carveVerticalTunnel(Map& map, int y1, int y2, int x)
{
    const auto [from, to] = std::minmax(y1, y2);
    for (int y = from; y <= to; ++y) {
        map.setTile(x, y, TileType::Floor);
    }
}

void connectRooms(Map& map, Point previous, Point next, Rng& rng)
{
    if (rng.coinFlip()) {
        carveHorizontalTunnel(map, previous.x, next.x, previous.y);
        carveVerticalTunnel(map, previous.y, next.y, next.x);
        return;
    }

    carveVerticalTunnel(map, previous.y, next.y, previous.x);
    carveHorizontalTunnel(map, previous.x, next.x, next.y);
}

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

[[nodiscard]] Rect roomInsideLeaf(const Rect& leaf, Rng& rng)
{
    const int maxWidth = std::max(4, leaf.width() - 2);
    const int maxHeight = std::max(4, leaf.height() - 2);
    const int minWidth = std::min(5, maxWidth);
    const int minHeight = std::min(5, maxHeight);
    const int width = rng.rangeInclusive(minWidth, maxWidth);
    const int height = rng.rangeInclusive(minHeight, maxHeight);
    const int x = rng.rangeInclusive(leaf.x1 + 1, leaf.x2 - width - 1);
    const int y = rng.rangeInclusive(leaf.y1 + 1, leaf.y2 - height - 1);
    return Rect{x, y, x + width, y + height};
}

[[nodiscard]] SpawnRegion roomSpawnRegion(const Rect& room)
{
    SpawnRegion region;
    region.kind = SpawnRegionKind::Room;
    for (int y = room.y1; y < room.y2; ++y) {
        for (int x = room.x1; x < room.x2; ++x) {
            region.tiles.push_back(Point{x, y});
        }
    }
    return region;
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

} // namespace

MapBuildResult BspDungeonBuilder::build(int width,
                                        int height,
                                        Rng& rng,
                                        MapBuildOptions options)
{
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument(
            "BspDungeonBuilder requires at least a 20x15 map");
    }

    Map map{width, height, TileType::Wall};
    std::vector<Rect> leaves;
    std::vector<Rect> rooms;
    std::vector<SpawnRegion> spawnRegions;
    std::vector<MapSnapshot> snapshots;
    splitRect(Rect{1, 1, width - 1, height - 1}, 0, rng, leaves);

    for (const auto& leaf : leaves) {
        if (leaf.width() < 5 || leaf.height() < 5) {
            continue;
        }

        const Rect room = roomInsideLeaf(leaf, rng);
        carveRoom(map, room);
        rooms.push_back(room);
        spawnRegions.push_back(roomSpawnRegion(room));
        recordSnapshot(snapshots,
                       map,
                       "bsp room " + std::to_string(rooms.size()),
                       options.recordSnapshots);
    }

    for (std::size_t index = 1; index < rooms.size(); ++index) {
        connectRooms(
            map, rooms[index - 1].center(), rooms[index].center(), rng);
        recordSnapshot(snapshots,
                       map,
                       "bsp corridor " + std::to_string(index),
                       options.recordSnapshots);
    }

    if (rooms.size() < 2) {
        throw std::runtime_error("BSP generation failed to place enough rooms");
    }

    const Point playerStart = rooms.front().center();
    const Point stairsPosition = rooms.back().center();
    map.setTile(stairsPosition.x, stairsPosition.y, TileType::DownStairs);
    recordSnapshot(snapshots, map, "bsp stairs", options.recordSnapshots);

    const auto validation = validateMapBuild(map, playerStart, stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error("BSP generation produced an invalid map");
    }

    return MapBuildResult{std::move(map),
                          playerStart,
                          stairsPosition,
                          std::move(rooms),
                          std::move(spawnRegions),
                          std::move(snapshots),
                          "bsp"};
}

} // namespace rl
