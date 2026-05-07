/**
 * **************************************************************************
 * @file     MapConnectivity.hpp
 * @brief    Declares map passability, reachability, and repair helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/TileType.hpp"

namespace rl {

class Map;

struct ConnectedRegion {
    std::vector<Point> tiles;
};

struct DistanceMap {
    int width = 0;
    int height = 0;
    std::vector<int> distances;

    [[nodiscard]] int distanceAt(Point point) const;
};

[[nodiscard]] bool isConnectivityPassable(TileType tile) noexcept;
[[nodiscard]] bool isConnectivityPassable(const Map& map, Point point);
[[nodiscard]] std::vector<Point> cardinalNeighbors(Point point);
[[nodiscard]] std::vector<Point> floorTiles(const Map& map);
[[nodiscard]] int countFloorTiles(const Map& map);
[[nodiscard]] int countConnectivityPassableTiles(const Map& map);
[[nodiscard]] std::vector<ConnectedRegion> findConnectedRegions(const Map& map);
[[nodiscard]] DistanceMap buildDistanceMap(const Map& map, Point start);
[[nodiscard]] int countReachableTiles(const Map& map, Point start);
[[nodiscard]] std::optional<Point> closestReachableFloorTo(const Map& map,
                                                           Point target);
[[nodiscard]] std::optional<Point> farthestReachableFloor(const Map& map,
                                                          Point start);

int cullUnreachable(Map& map, Point start);
int repairConnectivity(Map& map, Point anchor, Rng& rng);

} // namespace rl
