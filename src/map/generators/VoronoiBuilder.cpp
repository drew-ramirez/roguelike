/**
 * **************************************************************************
 * @file     VoronoiBuilder.cpp
 * @brief    Builds cell-like hive maps from Voronoi regions
 * **************************************************************************
 */

#include "map/generators/VoronoiBuilder.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

[[nodiscard]] int
distanceFor(Point left, Point right, VoronoiDistance algorithm) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    switch (algorithm) {
    case VoronoiDistance::Pythagoras:
        // Squared Euclidean distance preserves nearest-seed ordering without
        // sqrt, so Voronoi membership stays integer and deterministic
        return (dx * dx) + (dy * dy);
    case VoronoiDistance::Manhattan:
        // Taxicab distance forms diamond-shaped cells on a square grid
        return std::abs(dx) + std::abs(dy);
    case VoronoiDistance::Chebyshev:
        // King-move distance forms squarer cells by taking the dominant axis
        return std::max(std::abs(dx), std::abs(dy));
    }
    return (dx * dx) + (dy * dy);
}

[[nodiscard]] int closestSeedIndex(Point point,
                                   const std::vector<Point>& seeds,
                                   VoronoiDistance algorithm)
{
    int closest = std::numeric_limits<int>::max();
    int closestIndex = 0;
    int index = 0;
    for (const Point seed : seeds) {
        const int distance = distanceFor(point, seed, algorithm);
        if (distance < closest) {
            closest = distance;
            closestIndex = index;
        }
        ++index;
    }
    return closestIndex;
}

[[nodiscard]] bool isVoronoiBoundary(Point point,
                                     const std::vector<Point>& seeds,
                                     VoronoiDistance algorithm)
{
    const int mySeed = closestSeedIndex(point, seeds, algorithm);
    // A tile is a cell boundary when any cardinal neighbor belongs to a
    // different nearest seed. Boundaries remain walls; cell interiors become
    // floors.
    for (const Point neighbor : cardinalNeighbors(point)) {
        if (closestSeedIndex(neighbor, seeds, algorithm) != mySeed) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string settingLabel(const VoronoiSettings& settings)
{
    std::string label = std::to_string(settings.seedCount) + " seeds ";
    switch (settings.distance) {
    case VoronoiDistance::Pythagoras:
        label += "pythagoras";
        break;
    case VoronoiDistance::Manhattan:
        label += "manhattan";
        break;
    case VoronoiDistance::Chebyshev:
        label += "chebyshev";
        break;
    }
    return label;
}

} // namespace

MapBuildResult
VoronoiBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    return buildWithSettings(width, height, rng, VoronoiSettings{}, options);
}

MapBuildResult
VoronoiBuilder::buildWithSettings(int width,
                                  int height,
                                  Rng& rng,
                                  const VoronoiSettings& settings,
                                  MapBuildOptions options)
{
    validateBuilderDimensions("VoronoiBuilder", width, height);
    if (settings.seedCount <= 0) {
        throw std::invalid_argument("VoronoiBuilder requires positive seeds");
    }

    std::vector<Point> seeds;
    seeds.reserve(static_cast<std::size_t>(settings.seedCount));
    while (static_cast<int>(seeds.size()) < settings.seedCount) {
        const Point candidate{rng.rangeInclusive(1, width - 2),
                              rng.rangeInclusive(1, height - 2)};
        if (std::find(seeds.begin(), seeds.end(), candidate) == seeds.end()) {
            seeds.push_back(candidate);
        }
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "voronoi";
    addBuildStep(result, "base: voronoi hive");
    addBuildStep(result, "settings: " + settingLabel(settings));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            result.map.setTile(
                x,
                y,
                isVoronoiBoundary(Point{x, y}, seeds, settings.distance)
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

    validateBuildOrThrow(result, "Voronoi generation produced an invalid map");

    return result;
}

} // namespace rl
