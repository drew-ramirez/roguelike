/**
 * **************************************************************************
 * @file     MapConnectivity.cpp
 * @brief    Implements map passability, reachability, and repair helpers
 * **************************************************************************
 */

#include "map/MapConnectivity.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <queue>
#include <utility>

#include "map/Map.hpp"
#include "map/MapBrush.hpp"
#include "map/MapGeometry.hpp"

namespace rl {
namespace {

constexpr int unreachable = -1;

[[nodiscard]] std::size_t mapIndex(const Map& map, Point point)
{
    return static_cast<std::size_t>(map.xyIdx(point.x, point.y));
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

struct RegionConnection {
    Point from;
    Point to;
    std::size_t regionIndex = 0;
    int distance = std::numeric_limits<int>::max();
};

[[nodiscard]] RegionConnection
nearestRegionConnection(const ConnectedRegion& main,
                        const std::vector<ConnectedRegion>& regions)
{
    RegionConnection best;
    for (std::size_t index = 1; index < regions.size(); ++index) {
        for (const Point from : main.tiles) {
            for (const Point to : regions[index].tiles) {
                const int distance = distanceSquared(from, to);
                if (distance < best.distance) {
                    best = RegionConnection{from, to, index, distance};
                }
            }
        }
    }
    return best;
}

void carveArchitecturalConnection(Map& map, Point from, Point to, Rng& rng)
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

void carveWideLine(Map& map, Point from, Point to)
{
    for (const Point point :
         bresenhamLine(from, to, LineStartPolicy::IncludeStart)) {
        paintBrush(
            map, point, TileType::Floor, BrushShape::Square, 1, Symmetry::None);
    }
}

void carveNaturalConnection(
    Map& map, Point from, Point to, Rng& rng, ConnectivityStyle style)
{
    if (style == ConnectivityStyle::Surface) {
        carveWideLine(map, from, to);
        return;
    }

    const int distance =
        std::max(1, std::abs(to.x - from.x) + std::abs(to.y - from.y));
    const int jitter = std::min(4, distance / 4);
    Point midpoint{(from.x + to.x) / 2, (from.y + to.y) / 2};
    midpoint.x = std::clamp(
        midpoint.x + rng.rangeInclusive(-jitter, jitter), 1, map.width() - 2);
    midpoint.y = std::clamp(
        midpoint.y + rng.rangeInclusive(-jitter, jitter), 1, map.height() - 2);
    carveWideLine(map, from, midpoint);
    carveWideLine(map, midpoint, to);
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
    return tileTraits(tile).connectivityPassable;
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

int cullUnreachable(Map& map, Point start, TileType replacement)
{
    const auto distances = buildDistanceMap(map, start);
    int culled = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point point{x, y};
            if (isConnectivityPassable(map, point) &&
                distances.distanceAt(point) == unreachable) {
                map.setTile(x, y, replacement);
                ++culled;
            }
        }
    }
    return culled;
}

int cullSmallRegions(Map& map, int maximumSize, TileType replacement)
{
    if (maximumSize < 1) {
        return 0;
    }

    int culled = 0;
    for (const auto& region : findConnectedRegions(map)) {
        if (static_cast<int>(region.tiles.size()) > maximumSize) {
            continue;
        }
        for (const Point point : region.tiles) {
            map.setTile(point.x, point.y, replacement);
            ++culled;
        }
    }
    return culled;
}

int cullSmallNaturalBlockerComponents(Map& map, int maximumSize)
{
    if (maximumSize < 1) {
        return 0;
    }

    std::vector<bool> visited(
        static_cast<std::size_t>(map.width() * map.height()), false);
    int culled = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point start{x, y};
            const auto startIndex = mapIndex(map, start);
            if (visited[startIndex] || !isNaturalBlocker(map.tileAt(x, y))) {
                continue;
            }

            std::vector<Point> component;
            std::vector<Point> frontier{start};
            visited[startIndex] = true;
            while (!frontier.empty()) {
                const Point current = frontier.back();
                frontier.pop_back();
                component.push_back(current);
                for (const Point next : cardinalNeighbors(current)) {
                    if (!map.inBounds(next.x, next.y) ||
                        !isNaturalBlocker(map.tileAt(next.x, next.y))) {
                        continue;
                    }
                    const auto index = mapIndex(map, next);
                    if (!visited[index]) {
                        visited[index] = true;
                        frontier.push_back(next);
                    }
                }
            }
            if (static_cast<int>(component.size()) <= maximumSize) {
                for (const Point point : component) {
                    map.setTile(point.x, point.y, TileType::Floor);
                    ++culled;
                }
            }
        }
    }
    return culled;
}

int repairConnectivity(Map& map,
                       Point anchor,
                       Rng& rng,
                       ConnectivityStyle style)
{
    auto regions = regionsSortedBySize(findConnectedRegions(map));
    if (regions.empty()) {
        return 0;
    }

    int repaired = 0;
    if (isConnectivityPassable(map, anchor)) {
        const auto anchorRegion = std::find_if(
            regions.begin(), regions.end(), [anchor](const auto& region) {
                return std::find(region.tiles.begin(),
                                 region.tiles.end(),
                                 anchor) != region.tiles.end();
            });
        if (anchorRegion != regions.end()) {
            std::iter_swap(regions.begin(), anchorRegion);
        }
    }

    while (regions.size() > 1) {
        const auto connection =
            nearestRegionConnection(regions.front(), regions);
        if (connection.regionIndex == 0) {
            break;
        }
        if (style == ConnectivityStyle::Architectural) {
            carveArchitecturalConnection(
                map, connection.from, connection.to, rng);
        } else {
            carveNaturalConnection(
                map, connection.from, connection.to, rng, style);
        }
        ++repaired;
        regions = regionsSortedBySize(findConnectedRegions(map));
        if (isConnectivityPassable(map, anchor)) {
            const auto anchorRegion = std::find_if(
                regions.begin(), regions.end(), [anchor](const auto& region) {
                    return std::find(region.tiles.begin(),
                                     region.tiles.end(),
                                     anchor) != region.tiles.end();
                });
            if (anchorRegion != regions.end()) {
                std::iter_swap(regions.begin(), anchorRegion);
            }
        }
    }

    return repaired;
}

} // namespace rl
