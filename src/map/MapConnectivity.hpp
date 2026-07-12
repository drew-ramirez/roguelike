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

/** A cardinally connected set of passable map tiles. */
struct ConnectedRegion {
    std::vector<Point> tiles;
};

/** Cardinal step distance from one source, with unreachable sentinels. */
struct DistanceMap {
    int width = 0;
    int height = 0;
    std::vector<int> distances;

    [[nodiscard]] int distanceAt(Point point) const;
};

/** Selects repair width and route shape appropriate to the terrain. */
enum class ConnectivityStyle { Architectural, Cave, Surface };

[[nodiscard]] bool isConnectivityPassable(TileType tile) noexcept;
[[nodiscard]] bool isConnectivityPassable(const Map& map, Point point);
[[nodiscard]] std::vector<Point> cardinalNeighbors(Point point);
[[nodiscard]] std::vector<Point> floorTiles(const Map& map);
[[nodiscard]] int countFloorTiles(const Map& map);
[[nodiscard]] int countConnectivityPassableTiles(const Map& map);
[[nodiscard]] std::vector<ConnectedRegion> findConnectedRegions(const Map& map);
/** Builds a reachability field for endpoint selection and later analysis. */
[[nodiscard]] DistanceMap buildDistanceMap(const Map& map, Point start);
[[nodiscard]] int countReachableTiles(const Map& map, Point start);
[[nodiscard]] std::optional<Point> closestReachableFloorTo(const Map& map,
                                                           Point target);
[[nodiscard]] std::optional<Point> farthestReachableFloor(const Map& map,
                                                          Point start);

/** Replaces passable islands that cannot be reached from the anchor. */
int cullUnreachable(Map& map,
                    Point start,
                    TileType replacement = TileType::Wall);
/** Replaces connected passable regions at or below the requested size. */
int cullSmallRegions(Map& map,
                     int maximumSize,
                     TileType replacement = TileType::Wall);
/** Removes small isolated rock, tree, or reed fragments. */
int cullSmallNaturalBlockerComponents(Map& map, int maximumSize);
/** Joins disconnected regions using terrain-appropriate narrow routes. */
int repairConnectivity(
    Map& map,
    Point anchor,
    Rng& rng,
    ConnectivityStyle style = ConnectivityStyle::Architectural);

} // namespace rl
