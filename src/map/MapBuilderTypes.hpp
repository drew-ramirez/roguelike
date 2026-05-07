#pragma once

#include <string>
#include <vector>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/Rect.hpp"

namespace rl {

struct MapBuildOptions {
    bool recordSnapshots = false;
    int depth = 1;
    bool placeUpStairs = false;
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
    std::vector<std::string> buildSteps;
    std::vector<SpawnRequest> spawnRequests;
    Point upStairsPosition;
};

using BuilderMap = MapBuildResult;

} // namespace rl
