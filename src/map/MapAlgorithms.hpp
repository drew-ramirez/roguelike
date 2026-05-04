/**
 * **************************************************************************
 * @file     MapAlgorithms.hpp
 * @brief    Declares reusable map generation and connectivity helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"
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
[[nodiscard]] std::vector<ConnectedRegion> findConnectedRegions(const Map& map);
[[nodiscard]] DistanceMap buildDistanceMap(const Map& map, Point start);
[[nodiscard]] std::optional<Point> closestReachableFloorTo(const Map& map,
                                                           Point target);
[[nodiscard]] std::optional<Point> farthestReachableFloor(const Map& map,
                                                          Point start);

int cullUnreachable(Map& map, Point start);
int repairConnectivity(Map& map, Point anchor, Rng& rng);
void sealMapEdges(Map& map);
void carveLine(Map& map, Point from, Point to, Rng& rng);
void setDistantStairs(Map& map, Point start, Point& stairsPosition);
void recordSnapshot(std::vector<MapSnapshot>& snapshots,
                    const Map& map,
                    std::string label,
                    bool enabled);
void addBuildStep(MapBuildResult& result, std::string step);

[[nodiscard]] std::vector<SpawnRegion>
spawnRegionsFromConnectedFloors(const Map& map, SpawnRegionKind kind);
[[nodiscard]] SpawnRegion spawnRegionFromTiles(std::vector<Point> tiles,
                                               SpawnRegionKind kind);

} // namespace rl
