/**
 * **************************************************************************
 * @file     VoronoiBuilder.cpp
 * @brief    Builds cell-like hive maps from Voronoi regions
 * **************************************************************************
 */

#include "map/generators/VoronoiBuilder.hpp"

#include <algorithm>
#include <cstddef>
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

constexpr std::size_t maxFloatingWallFragmentSize = 8;
constexpr int minimumSeedSpacingSquared = 16;

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

[[nodiscard]] bool touchesMapEdge(const Map& map, Point point) noexcept
{
    return point.x == 0 || point.y == 0 || point.x == map.width() - 1 ||
           point.y == map.height() - 1;
}

[[nodiscard]] bool isWall(const Map& map, Point point)
{
    return map.inBounds(point.x, point.y) &&
           map.tileAt(point.x, point.y) == TileType::Rock;
}

int removeFloatingWallFragments(Map& map)
{
    std::vector<bool> visited(static_cast<std::size_t>(map.tileCount()), false);
    int removed = 0;

    // Voronoi boundaries can leave tiny enclosed wall islands. Keep large or
    // edge-connected walls, but flatten small interior components so they do
    // not interfere with spawn regions and distant-stairs selection.
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point start{x, y};
            const auto startIndex =
                static_cast<std::size_t>(map.xyIdx(start.x, start.y));
            if (visited[startIndex] || !isWall(map, start)) {
                continue;
            }

            bool touchesEdge = false;
            std::vector<Point> component;
            std::vector<Point> frontier{start};
            visited[startIndex] = true;

            while (!frontier.empty()) {
                const Point current = frontier.back();
                frontier.pop_back();
                component.push_back(current);
                touchesEdge = touchesEdge || touchesMapEdge(map, current);

                for (const Point next : cardinalNeighbors(current)) {
                    if (!isWall(map, next)) {
                        continue;
                    }

                    const auto nextIndex =
                        static_cast<std::size_t>(map.xyIdx(next.x, next.y));
                    if (visited[nextIndex]) {
                        continue;
                    }

                    visited[nextIndex] = true;
                    frontier.push_back(next);
                }
            }

            if (!touchesEdge &&
                component.size() <= maxFloatingWallFragmentSize) {
                for (const Point point : component) {
                    map.setTile(point.x, point.y, TileType::Floor);
                    ++removed;
                }
            }
        }
    }

    return removed;
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

    const int readableSeedCount =
        std::min(settings.seedCount, std::max(8, (width * height) / 100));
    std::vector<Point> seeds;
    seeds.reserve(static_cast<std::size_t>(readableSeedCount));
    int placementAttempts = 0;
    const int maximumPlacementAttempts = width * height * 4;
    while (static_cast<int>(seeds.size()) < readableSeedCount &&
           placementAttempts < maximumPlacementAttempts) {
        const Point candidate{rng.rangeInclusive(1, width - 2),
                              rng.rangeInclusive(1, height - 2)};
        const bool spaced = std::all_of(
            seeds.begin(), seeds.end(), [candidate](Point existing) {
                const int dx = existing.x - candidate.x;
                const int dy = existing.y - candidate.y;
                return (dx * dx) + (dy * dy) >= minimumSeedSpacingSquared;
            });
        if (spaced) {
            seeds.push_back(candidate);
        }
        ++placementAttempts;
    }
    if (seeds.size() < 2U) {
        throw std::runtime_error("VoronoiBuilder could not space its seeds");
    }

    MapBuildResult result{Map{width, height, TileType::Rock}, {}, {}};
    result.builderName = "voronoi";
    addBuildStep(result, "base: voronoi hive");
    addBuildStep(result, "settings: " + settingLabel(settings));
    addBuildStep(result,
                 "readable spaced seeds: " + std::to_string(seeds.size()));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            result.map.setTile(
                x,
                y,
                isVoronoiBoundary(Point{x, y}, seeds, settings.distance)
                    ? TileType::Rock
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
    sealMapEdges(result.map, TileType::Rock);
    if (removeFloatingWallFragments(result.map) > 0) {
        addBuildStep(result, "modifier: floating wall cleanup");
    }
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    recordSnapshot(
        result.snapshots, result.map, "voronoi hive", options.recordSnapshots);
    addBuildStep(result, "modifier: deliberate nearest-boundary gates");
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "Voronoi generation produced an invalid map");

    return result;
}

} // namespace rl
