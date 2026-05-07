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
void sealMapEdges(Map& map);
void setDistantStairs(Map& map, Point start, Point& stairsPosition);
void recordSnapshot(std::vector<MapSnapshot>& snapshots,
                    const Map& map,
                    std::string label,
                    bool enabled);
void addBuildStep(MapBuildResult& result, std::string step);
void validateBuildOrThrow(const MapBuildResult& result,
                          std::string_view failureMessage);

[[nodiscard]] std::vector<SpawnRegion>
spawnRegionsFromConnectedFloors(const Map& map, SpawnRegionKind kind);
[[nodiscard]] SpawnRegion spawnRegionFromTiles(std::vector<Point> tiles,
                                               SpawnRegionKind kind);

} // namespace rl
