#pragma once

#include <string>
#include <vector>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/Rect.hpp"
#include "world/WorldTypes.hpp"

namespace rl {

struct MapBuildOptions {
    bool recordSnapshots = false;
    int depth = 1;
    bool placeUpStairs = false;
    bool allowDownStairs = true;
    BiomeVariant biomeVariant = BiomeVariant::DryWoods;
    LocalScreenRole screenRole = LocalScreenRole::CornerWilderness;
    WorldCellFields worldFields;
    int exitMask = 0;
    std::string landmarkId;
    bool dangerEncounter = false;
};

struct MapSnapshot {
    std::string label;
    Map map;
};

enum class SpawnRegionKind { Room, Corridor };

struct SpawnRegion {
    SpawnRegionKind kind = SpawnRegionKind::Room;
    std::vector<Point> tiles;
};

struct CorridorPath {
    std::string strategy;
    std::vector<Point> tiles;
};

struct SpawnRequest {
    std::string id;
    Point position;
};

struct MapBuildResult {
    Map map;
    Point playerStart;
    Point stairsPosition;
    std::vector<Rect> rooms;
    std::vector<SpawnRegion> spawnRegions;
    std::vector<MapSnapshot> snapshots;
    std::string builderName = "simple";
    std::vector<CorridorPath> corridorPaths;
    std::vector<SpawnRequest> spawnRequests;
    Point upStairsPosition;
};

} // namespace rl
