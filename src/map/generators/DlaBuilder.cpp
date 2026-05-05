/**
 * **************************************************************************
 * @file     DlaBuilder.cpp
 * @brief    Builds organic caves with diffusion-limited aggregation variants
 * **************************************************************************
 */

#include "map/generators/DlaBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/MapGeometry.hpp"
#include "map/TileType.hpp"
#include "map/generators/RandomWalk.hpp"

namespace rl {
namespace {

void paint(Map& map, Point point, const DlaSettings& settings)
{
    paintBrushSize(
        map, point, TileType::Floor, settings.brushSize, settings.symmetry);
}

void carveSeed(Map& map, Point center)
{
    map.setTile(center.x, center.y, TileType::Floor);
    map.setTile(center.x - 1, center.y, TileType::Floor);
    map.setTile(center.x + 1, center.y, TileType::Floor);
    map.setTile(center.x, center.y - 1, TileType::Floor);
    map.setTile(center.x, center.y + 1, TileType::Floor);
}

bool walkInwards(Map& map, const DlaSettings& settings, Rng& rng)
{
    Point digger = randomInteriorPoint(map.width(), map.height(), rng);
    Point previous = digger;
    int guard = map.width() * map.height() * 40;
    // Walk through solid rock until we hit the existing cave mass, then paint
    // the previous wall. That grows the boundary from the outside inward.
    while (map.tileAt(digger.x, digger.y) == TileType::Wall && guard > 0) {
        previous = digger;
        staggerCardinal(digger, map.width(), map.height(), rng);
        --guard;
    }
    if (guard <= 0) {
        return false;
    }
    paint(map, previous, settings);
    return true;
}

void walkOutwards(Map& map, Point start, const DlaSettings& settings, Rng& rng)
{
    Point digger = start;
    int guard = map.width() * map.height();
    // Inverse of walk-inwards: start inside the cave and random-walk until the
    // first wall, painting that wall expands the frontier outward
    while (map.tileAt(digger.x, digger.y) == TileType::Floor && guard > 0) {
        staggerCardinal(digger, map.width(), map.height(), rng);
        --guard;
    }
    paint(map, digger, settings);
}

void centralAttractor(Map& map,
                      Point start,
                      const DlaSettings& settings,
                      Rng& rng)
{
    Point digger = randomInteriorPoint(map.width(), map.height(), rng);
    Point previous = digger;
    // Pull random points toward the seed along a straight integer line. The
    // last wall before existing floor becomes the new aggregate cell.
    for (const Point next :
         bresenhamLine(digger, start, LineStartPolicy::ExcludeStart)) {
        if (map.tileAt(digger.x, digger.y) != TileType::Wall) {
            break;
        }
        previous = digger;
        digger = next;
    }
    paint(map, previous, settings);
}

[[nodiscard]] std::string settingLabel(const DlaSettings& settings)
{
    std::string label;
    switch (settings.algorithm) {
    case DlaAlgorithm::WalkInwards:
        label = "walk-inwards";
        break;
    case DlaAlgorithm::WalkOutwards:
        label = "walk-outwards";
        break;
    case DlaAlgorithm::CentralAttractor:
        label = "central-attractor";
        break;
    }
    label += " brush " + std::to_string(settings.brushSize);
    label += " floor " + std::to_string(settings.floorPercent);
    return label;
}

} // namespace

DlaSettings DlaBuilder::walkInwardsSettings() noexcept
{
    return DlaSettings{
        DlaAlgorithm::WalkInwards,
        1,
        Symmetry::None,
        0.25,
    };
}

DlaSettings DlaBuilder::walkOutwardsSettings() noexcept
{
    return DlaSettings{
        DlaAlgorithm::WalkOutwards,
        2,
        Symmetry::None,
        0.25,
    };
}

DlaSettings DlaBuilder::centralAttractorSettings() noexcept
{
    return DlaSettings{
        DlaAlgorithm::CentralAttractor,
        2,
        Symmetry::None,
        0.25,
    };
}

DlaSettings DlaBuilder::insectoidSettings() noexcept
{
    return DlaSettings{
        DlaAlgorithm::CentralAttractor,
        2,
        Symmetry::Horizontal,
        0.25,
    };
}

MapBuildResult
DlaBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    return buildWithSettings(
        width, height, rng, walkInwardsSettings(), options);
}

MapBuildResult DlaBuilder::buildWithSettings(int width,
                                             int height,
                                             Rng& rng,
                                             const DlaSettings& settings,
                                             MapBuildOptions options)
{
    validateBuilderDimensions("DlaBuilder", width, height);
    if (settings.brushSize <= 0 || settings.floorPercent <= 0.0 ||
        settings.floorPercent >= 1.0) {
        throw std::invalid_argument("Invalid DLA settings");
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "dla";
    result.playerStart = Point{width / 2, height / 2};
    carveSeed(result.map, result.playerStart);
    addBuildStep(result, "base: diffusion-limited aggregation");
    addBuildStep(result, "settings: " + settingLabel(settings));
    recordSnapshot(
        result.snapshots, result.map, "seed dla", options.recordSnapshots);

    const int targetFloors = static_cast<int>(
        static_cast<double>(width * height) * settings.floorPercent);
    const int maxIterations = width * height * 20;
    int iterations = 0;
    while (countFloorTiles(result.map) < targetFloors &&
           iterations < maxIterations) {
        switch (settings.algorithm) {
        case DlaAlgorithm::WalkInwards: {
            [[maybe_unused]] const bool grew =
                walkInwards(result.map, settings, rng);
            break;
        }
        case DlaAlgorithm::WalkOutwards:
            walkOutwards(result.map, result.playerStart, settings, rng);
            break;
        case DlaAlgorithm::CentralAttractor:
            centralAttractor(result.map, result.playerStart, settings, rng);
            break;
        }
        ++iterations;

        if (options.recordSnapshots && iterations % 25 == 0) {
            recordSnapshot(result.snapshots,
                           result.map,
                           "dla iteration " + std::to_string(iterations),
                           true);
        }
    }

    recordSnapshot(
        result.snapshots, result.map, "grown dla", options.recordSnapshots);
    cullUnreachable(result.map, result.playerStart);
    sealMapEdges(result.map);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    addBuildStep(result,
                 "iterations: " + std::to_string(iterations) + " of " +
                     std::to_string(maxIterations));
    addBuildStep(result, "modifier: unreachable cull");
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "DLA generation produced an invalid map");

    return result;
}

} // namespace rl
