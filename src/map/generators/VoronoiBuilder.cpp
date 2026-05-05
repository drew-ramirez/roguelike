/**
 * **************************************************************************
 * @file     VoronoiBuilder.cpp
 * @brief    Builds cell-like hive maps from Voronoi regions
 * **************************************************************************
 */

#include "map/generators/VoronoiBuilder.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

#include "map/MapAlgorithms.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int minMapWidth = 20;
constexpr int minMapHeight = 15;
constexpr int seedCount = 18;

[[nodiscard]] int distanceSquared(Point left, Point right) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    return (dx * dx) + (dy * dy);
}

[[nodiscard]] bool isVoronoiWall(Point point, const std::vector<Point>& seeds)
{
    int closest = std::numeric_limits<int>::max();
    int second = std::numeric_limits<int>::max();
    for (const Point seed : seeds) {
        const int distance = distanceSquared(point, seed);
        if (distance < closest) {
            second = closest;
            closest = distance;
        } else if (distance < second) {
            second = distance;
        }
    }
    return second - closest <= 8;
}

} // namespace

MapBuildResult
VoronoiBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    if (width < minMapWidth || height < minMapHeight) {
        throw std::invalid_argument(
            "VoronoiBuilder requires at least a 20x15 map");
    }

    std::vector<Point> seeds;
    seeds.reserve(seedCount);
    for (int index = 0; index < seedCount; ++index) {
        seeds.push_back(Point{rng.rangeInclusive(2, width - 3),
                              rng.rangeInclusive(2, height - 3)});
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "voronoi";
    addBuildStep(result, "base: voronoi hive");

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            result.map.setTile(x,
                               y,
                               isVoronoiWall(Point{x, y}, seeds)
                                   ? TileType::Wall
                                   : TileType::Floor);
        }
    }

    auto start =
        closestReachableFloorTo(result.map, Point{width / 2, height / 2});
    if (!start.has_value()) {
        result.map.setTile(width / 2, height / 2, TileType::Floor);
        start = Point{width / 2, height / 2};
    }
    result.playerStart = *start;
    repairConnectivity(result.map, result.playerStart, rng);
    sealMapEdges(result.map);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    recordSnapshot(
        result.snapshots, result.map, "voronoi hive", options.recordSnapshots);
    addBuildStep(result, "modifier: connectivity repair");
    addBuildStep(result, "modifier: distant exit");

    const auto validation =
        validateMapBuild(result.map, result.playerStart, result.stairsPosition);
    if (!validation.valid) {
        throw std::runtime_error("Voronoi generation produced an invalid map");
    }

    return result;
}

} // namespace rl
