#include "map/generators/SimpleMapBuilder.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

#include "map/Map.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int minMapWidth = 20;
constexpr int minMapHeight = 15;
constexpr int maxRooms = 30;
constexpr int minRoomSize = 6;
constexpr int maxRoomSize = 10;
constexpr int minAcceptedRooms = 2;
constexpr int minFloorTiles = 20;

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

int countWalkableTiles(const Map& map)
{
    int count = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.isWalkable(x, y)) {
                ++count;
            }
        }
    }
    return count;
}

bool allWalkableTilesReachable(const Map& map, Point start)
{
    if (!map.isWalkable(start.x, start.y)) {
        return false;
    }

    const int totalWalkable = countWalkableTiles(map);
    int reached = 0;
    std::vector<bool> visited(
        static_cast<std::size_t>(map.width() * map.height()), false);
    std::vector<Point> frontier{start};
    visited[static_cast<std::size_t>(map.xyIdx(start.x, start.y))] = true;

    constexpr std::array<Point, 4> neighbors{
        Point{1, 0},
        Point{-1, 0},
        Point{0, 1},
        Point{0, -1},
    };

    while (!frontier.empty()) {
        const Point current = frontier.back();
        frontier.pop_back();
        ++reached;

        for (const Point delta : neighbors) {
            const Point next{current.x + delta.x, current.y + delta.y};
            if (!map.isWalkable(next.x, next.y)) {
                continue;
            }

            const auto index =
                static_cast<std::size_t>(map.xyIdx(next.x, next.y));
            if (!visited[index]) {
                visited[index] = true;
                frontier.push_back(next);
            }
        }
    }

    return reached == totalWalkable;
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

bool intersectsAnyRoom(const Rect& candidate, const std::vector<Rect>& rooms)
{
    return std::any_of(
        rooms.begin(), rooms.end(), [&candidate](const Rect& room) {
            return candidate.intersects(room);
        });
}

void validateGeneratedMap(const Map& map,
                          Point playerStart,
                          const std::vector<Rect>& rooms)
{
    if (static_cast<int>(rooms.size()) < minAcceptedRooms) {
        throw std::runtime_error("Map generation failed to place enough rooms");
    }
    if (countWalkableTiles(map) < minFloorTiles) {
        throw std::runtime_error(
            "Map generation failed to carve enough floor tiles");
    }
    if (!map.isWalkable(playerStart.x, playerStart.y)) {
        throw std::runtime_error(
            "Map generation placed the player start in a wall");
    }
    if (!allWalkableTilesReachable(map, playerStart)) {
        throw std::runtime_error(
            "Map generation produced isolated floor tiles");
    }
}

} // namespace

MapBuildResult SimpleMapBuilder::build(int width, int height, Rng& rng)
{
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument(
            "SimpleMapBuilder requires at least a 20x15 map");
    }

    Map map{width, height, TileType::Wall};
    std::vector<Rect> rooms;
    rooms.reserve(maxRooms);

    for (int attempt = 0; attempt < maxRooms; ++attempt) {
        const int roomWidth = rng.rangeInclusive(minRoomSize, maxRoomSize);
        const int roomHeight = rng.rangeInclusive(minRoomSize, maxRoomSize);
        const int x = rng.rangeInclusive(1, width - roomWidth - 1);
        const int y = rng.rangeInclusive(1, height - roomHeight - 1);
        const Rect room{x, y, x + roomWidth, y + roomHeight};

        if (intersectsAnyRoom(room, rooms)) {
            continue;
        }

        carveRoom(map, room);
        if (!rooms.empty()) {
            connectRooms(map, rooms.back().center(), room.center(), rng);
        }
        rooms.push_back(room);
    }

    const Point playerStart = rooms.empty() ? Point{} : rooms.front().center();
    validateGeneratedMap(map, playerStart, rooms);
    return MapBuildResult{std::move(map), playerStart, std::move(rooms)};
}

} // namespace rl
