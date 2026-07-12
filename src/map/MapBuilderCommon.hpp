/**
 * **************************************************************************
 * @file     MapBuilderCommon.hpp
 * @brief    Declares shared map builder finalization helpers
 * **************************************************************************
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/Point.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class Map;

void validateBuilderDimensions(std::string_view builderName,
                               int width,
                               int height,
                               int minWidth = 20,
                               int minHeight = 15);
/** Seals the perimeter after every topology-changing generation step. */
void sealMapEdges(Map& map, TileType tile = TileType::Wall);
/** Places progression on the most distant reachable floor from the start. */
void setDistantStairs(Map& map, Point start, Point& stairsPosition);
void setUpStairsAtStart(Map& map, Point start, Point& upStairsPosition);
void recordSnapshot(std::vector<MapSnapshot>& snapshots,
                    const Map& map,
                    std::string label,
                    bool enabled);
void addBuildStep(MapBuildResult& result, std::string step);
/** Rejects results whose start, stairs, or connectivity are not playable. */
void validateBuildOrThrow(const MapBuildResult& result,
                          std::string_view failureMessage);

/** Rebuilds spawn metadata from final connected walkable topology. */
[[nodiscard]] std::vector<SpawnRegion>
spawnRegionsFromConnectedFloors(const Map& map, SpawnRegionKind kind);
[[nodiscard]] SpawnRegion spawnRegionFromTiles(std::vector<Point> tiles,
                                               SpawnRegionKind kind);

} // namespace rl
