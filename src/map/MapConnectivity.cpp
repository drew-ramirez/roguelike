/**
 * **************************************************************************
 * @file     MapConnectivity.cpp
 * @brief    Implements map passability, reachability, and repair helpers
 * **************************************************************************
 */

#include "map/MapConnectivity.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <utility>

#include "map/Map.hpp"

namespace rl {
namespace {

constexpr int unreachable = -1;

[[nodiscard]] std::size_t mapIndex(const Map& map, Point point)
{
    return static_cast<std::size_t>(map.xyIdx(point.x, point.y));
}

[[nodiscard]] Point regionCenter(const ConnectedRegion& region)
{
    if (region.tiles.empty()) {
        return {};
    }

    // The arithmetic mean is a stable representative point for repair tunnels.
    int xSum = 0;
    int ySum = 0;
    for (const Point point : region.tiles) {
        xSum += point.x;
        ySum += point.y;
    }
    const int count = static_cast<int>(region.tiles.size());
    return Point{xSum / count, ySum / count};
}

[[nodiscard]] int distanceSquared(Point left, Point right) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    return (dx * dx) + (dy * dy);
}

[[nodiscard]] std::vector<ConnectedRegion>
regionsSortedBySize(std::vector<ConnectedRegion> regions)
{
    std::sort(regions.begin(),
              regions.end(),
              [](const ConnectedRegion& left, const ConnectedRegion& right) {
                  return left.tiles.size() > right.tiles.size();
              });
    return regions;
}

void carveLine(Map& map, Point from, Point to, Rng& rng)
{
    const bool horizontalFirst = rng.coinFlip();
    const auto carveHorizontal = [&map](int x1, int x2, int y) {
        const auto [first, last] = std::minmax(x1, x2);
        for (int x = first; x <= last; ++x) {
            if (map.inBounds(x, y)) {
                map.setTile(x, y, TileType::Floor);
            }
        }
    };
    const auto carveVertical = [&map](int y1, int y2, int x) {
        const auto [first, last] = std::minmax(y1, y2);
        for (int y = first; y <= last; ++y) {
            if (map.inBounds(x, y)) {
                map.setTile(x, y, TileType::Floor);
            }
        }
    };

    if (horizontalFirst) {
        carveHorizontal(from.x, to.x, from.y);
        carveVertical(from.y, to.y, to.x);
        return;
    }

    carveVertical(from.y, to.y, from.x);
    carveHorizontal(from.x, to.x, to.y);
}

} // namespace

int DistanceMap::distanceAt(Point point) const
{
    if (point.x < 0 || point.y < 0 || point.x >= width || point.y >= height) {
        return unreachable;
    }
    return distances[static_cast<std::size_t>(point.y * width + point.x)];
}

bool isConnectivityPassable(TileType tile) noexcept
{
    return tile != TileType::Wall;
}

bool isConnectivityPassable(const Map& map, Point point)
{
    return map.inBounds(point.x, point.y) &&
           isConnectivityPassable(map.tileAt(point.x, point.y));
}

std::vector<Point> cardinalNeighbors(Point point)
{
    return {
        Point{point.x + 1, point.y},
        Point{point.x - 1, point.y},
        Point{point.x, point.y + 1},
        Point{point.x, point.y - 1},
    };
}

std::vector<Point> floorTiles(const Map& map)
{
    std::vector<Point> tiles;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.isWalkable(x, y)) {
                tiles.push_back(Point{x, y});
            }
        }
    }
    return tiles;
}

int countFloorTiles(const Map& map)
{
    return static_cast<int>(floorTiles(map).size());
}

int countConnectivityPassableTiles(const Map& map)
{
    int count = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (isConnectivityPassable(map, Point{x, y})) {
                ++count;
            }
        }
    }
    return count;
}

std::vector<ConnectedRegion> findConnectedRegions(const Map& map)
{
    std::vector<ConnectedRegion> regions;
    std::vector<bool> visited(
        static_cast<std::size_t>(map.width() * map.height()), false);

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point start{x, y};
            if (!isConnectivityPassable(map, start) ||
                visited[mapIndex(map, start)]) {
                continue;
            }

            ConnectedRegion region;
            std::vector<Point> frontier{start};
            visited[mapIndex(map, start)] = true;

            while (!frontier.empty()) {
                const Point current = frontier.back();
                frontier.pop_back();
                region.tiles.push_back(current);

                for (const Point next : cardinalNeighbors(current)) {
                    if (!isConnectivityPassable(map, next)) {
                        continue;
                    }
                    const auto index = mapIndex(map, next);
                    if (!visited[index]) {
                        visited[index] = true;
                        frontier.push_back(next);
                    }
                }
            }

            regions.push_back(std::move(region));
        }
    }

    return regions;
}

DistanceMap buildDistanceMap(const Map& map, Point start)
{
    DistanceMap result;
    result.width = map.width();
    result.height = map.height();
    result.distances.assign(
        static_cast<std::size_t>(map.width() * map.height()), unreachable);

    if (!isConnectivityPassable(map, start)) {
        return result;
    }

    std::queue<Point> frontier;
    frontier.push(start);
    result.distances[mapIndex(map, start)] = 0;

    while (!frontier.empty()) {
        const Point current = frontier.front();
        frontier.pop();
        const int currentDistance = result.distanceAt(current);

        for (const Point next : cardinalNeighbors(current)) {
            if (!isConnectivityPassable(map, next)) {
                continue;
            }
            const auto index = mapIndex(map, next);
            if (result.distances[index] == unreachable) {
                result.distances[index] = currentDistance + 1;
                frontier.push(next);
            }
        }
    }

    return result;
}

int countReachableTiles(const Map& map, Point start)
{
    const auto distances = buildDistanceMap(map, start);
    return static_cast<int>(
        std::count_if(distances.distances.begin(),
                      distances.distances.end(),
                      [](int distance) { return distance != unreachable; }));
}

std::optional<Point> closestReachableFloorTo(const Map& map, Point target)
{
    std::optional<Point> best;
    int bestDistance = std::numeric_limits<int>::max();
    for (const Point point : floorTiles(map)) {
        const int distance = distanceSquared(point, target);
        if (distance < bestDistance) {
            best = point;
            bestDistance = distance;
        }
    }
    return best;
}

std::optional<Point> farthestReachableFloor(const Map& map, Point start)
{
    const auto distances = buildDistanceMap(map, start);
    std::optional<Point> best;
    int bestDistance = unreachable;

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point point{x, y};
            if (!map.isWalkable(x, y)) {
                continue;
            }
            const int distance = distances.distanceAt(point);
            if (distance > bestDistance) {
                best = point;
                bestDistance = distance;
            }
        }
    }

    return best;
}

int cullUnreachable(Map& map, Point start)
{
    const auto distances = buildDistanceMap(map, start);
    int culled = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point point{x, y};
            if (isConnectivityPassable(map, point) &&
                distances.distanceAt(point) == unreachable) {
                map.setTile(x, y, TileType::Wall);
                ++culled;
            }
        }
    }
    return culled;
}

int repairConnectivity(Map& map, Point anchor, Rng& rng)
{
    auto regions = regionsSortedBySize(findConnectedRegions(map));
    if (regions.empty()) {
        return 0;
    }

    int repaired = 0;
    Point mainCenter = anchor;
    if (!isConnectivityPassable(map, anchor)) {
        mainCenter = regionCenter(regions.front());
    }

    while (regions.size() > 1) {
        const Point target = regionCenter(regions.back());
        carveLine(map, mainCenter, target, rng);
        ++repaired;
        regions = regionsSortedBySize(findConnectedRegions(map));
        mainCenter = regionCenter(regions.front());
    }

    return repaired;
}

} // namespace rl
