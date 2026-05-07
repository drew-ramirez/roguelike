/**
 * **************************************************************************
 * @file     DrunkardsWalkBuilder.cpp
 * @brief    Builds cave maps with constrained random walkers
 * **************************************************************************
 */

#include "map/generators/DrunkardsWalkBuilder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "map/MapBrush.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"
#include "map/generators/RandomWalk.hpp"

namespace rl {
namespace {

[[nodiscard]] Point startDigger(
    Point playerStart, int width, int height, DrunkSpawnMode mode, Rng& rng)
{
    if (mode == DrunkSpawnMode::StartingPoint) {
        return playerStart;
    }

    return randomInteriorPoint(width, height, rng);
}

[[nodiscard]] std::string settingLabel(const DrunkardSettings& settings)
{
    std::string label = settings.spawnMode == DrunkSpawnMode::StartingPoint
                            ? "starting-point"
                            : "random";
    label += " lifetime " + std::to_string(settings.drunkenLifetime);
    label += " floor " + std::to_string(settings.floorPercent);
    label += " brush " + std::to_string(settings.brushSize);
    return label;
}

} // namespace

DrunkardSettings DrunkardsWalkBuilder::openAreaSettings() noexcept
{
    return DrunkardSettings{
        DrunkSpawnMode::StartingPoint,
        400,
        0.5,
        1,
        Symmetry::None,
    };
}

DrunkardSettings DrunkardsWalkBuilder::openHallsSettings() noexcept
{
    return DrunkardSettings{
        DrunkSpawnMode::Random,
        400,
        0.5,
        1,
        Symmetry::None,
    };
}

DrunkardSettings DrunkardsWalkBuilder::windingPassagesSettings() noexcept
{
    return DrunkardSettings{
        DrunkSpawnMode::Random,
        100,
        0.4,
        1,
        Symmetry::None,
    };
}

DrunkardSettings DrunkardsWalkBuilder::fatPassagesSettings() noexcept
{
    return DrunkardSettings{
        DrunkSpawnMode::Random,
        100,
        0.4,
        2,
        Symmetry::None,
    };
}

DrunkardSettings DrunkardsWalkBuilder::fearfulSymmetrySettings() noexcept
{
    return DrunkardSettings{
        DrunkSpawnMode::Random,
        100,
        0.4,
        1,
        Symmetry::Both,
    };
}

MapBuildResult DrunkardsWalkBuilder::build(int width,
                                           int height,
                                           Rng& rng,
                                           MapBuildOptions options)
{
    return buildWithSettings(
        width, height, rng, windingPassagesSettings(), options);
}

MapBuildResult
DrunkardsWalkBuilder::buildWithSettings(int width,
                                        int height,
                                        Rng& rng,
                                        const DrunkardSettings& settings,
                                        MapBuildOptions options)
{
    validateBuilderDimensions("DrunkardsWalkBuilder", width, height);
    if (settings.drunkenLifetime <= 0 || settings.floorPercent <= 0.0 ||
        settings.floorPercent >= 1.0 || settings.brushSize <= 0) {
        throw std::invalid_argument("Invalid drunkard walk settings");
    }

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "drunkard";
    result.playerStart = Point{width / 2, height / 2};
    result.map.setTile(
        result.playerStart.x, result.playerStart.y, TileType::Floor);
    addBuildStep(result, "base: drunkard walk");
    addBuildStep(result, "settings: " + settingLabel(settings));

    const int desiredFloors = static_cast<int>(
        static_cast<double>(width * height) * settings.floorPercent);
    const int diggerLimit = width * height * 4;
    int diggerCount = 0;
    int activeDiggerCount = 0;
    while (countFloorTiles(result.map) < desiredFloors &&
           diggerCount < diggerLimit) {
        bool didSomething = false;
        Point digger = startDigger(
            result.playerStart, width, height, settings.spawnMode, rng);

        for (int life = settings.drunkenLifetime; life > 0; --life) {
            if (result.map.tileAt(digger.x, digger.y) == TileType::Wall) {
                didSomething = true;
            }
            paintBrushSize(result.map,
                           digger,
                           TileType::Floor,
                           settings.brushSize,
                           settings.symmetry);
            staggerCardinal(digger, width, height, rng);
        }

        if (didSomething) {
            ++activeDiggerCount;
            recordSnapshot(result.snapshots,
                           result.map,
                           "digger " + std::to_string(activeDiggerCount),
                           options.recordSnapshots);
        }
        ++diggerCount;
    }

    cullUnreachable(result.map, result.playerStart);
    sealMapEdges(result.map);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    addBuildStep(result,
                 "digger count: " + std::to_string(diggerCount) + " total, " +
                     std::to_string(activeDiggerCount) + " active");
    addBuildStep(result, "modifier: unreachable cull");
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "Drunkard generation produced an invalid map");

    return result;
}

} // namespace rl
