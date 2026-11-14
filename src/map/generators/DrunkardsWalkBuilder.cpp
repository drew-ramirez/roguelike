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
#include <utility>
#include <vector>

#include "map/MapBrush.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"
#include "map/generators/RandomWalk.hpp"

namespace rl {
namespace {

[[nodiscard]] Point
startDigger(const Map& map, Point playerStart, DrunkSpawnMode mode, Rng& rng)
{
    if (mode == DrunkSpawnMode::StartingPoint) {
        return playerStart;
    }

    std::vector<Point> frontierFloors;
    for (const Point floor : floorTiles(map)) {
        bool touchesRock = false;
        for (const Point neighbor : cardinalNeighbors(floor)) {
            if (map.inBounds(neighbor.x, neighbor.y) &&
                tileTraits(map.tileAt(neighbor.x, neighbor.y)).blocksMovement) {
                touchesRock = true;
                break;
            }
        }
        if (touchesRock) {
            frontierFloors.push_back(floor);
        }
    }
    const auto starts =
        frontierFloors.empty() ? floorTiles(map) : std::move(frontierFloors);
    return starts[static_cast<std::size_t>(
        rng.rangeInclusive(0, static_cast<int>(starts.size()) - 1))];
}

void staggerPersistent(
    Point& point, Point& heading, int width, int height, Rng& rng)
{
    if (heading == Point{} || rng.rangeInclusive(1, 100) <= 28) {
        switch (rng.rangeInclusive(0, 3)) {
        case 0:
            heading = Point{1, 0};
            break;
        case 1:
            heading = Point{-1, 0};
            break;
        case 2:
            heading = Point{0, 1};
            break;
        default:
            heading = Point{0, -1};
            break;
        }
    }
    point.x = std::clamp(point.x + heading.x, 1, width - 2);
    point.y = std::clamp(point.y + heading.y, 1, height - 2);
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
        60,
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
        Symmetry::Quad,
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

    MapBuildResult result{Map{width, height, TileType::Rock}, {}, {}};
    result.builderName = "drunkard";
    result.playerStart = Point{width / 2, height / 2};
    result.map.setTile(
        result.playerStart.x, result.playerStart.y, TileType::Floor);

    const int desiredFloors = static_cast<int>(
        static_cast<double>(width * height) * settings.floorPercent);
    const int diggerLimit = width * height * 4;
    int diggerCount = 0;
    int activeDiggerCount = 0;
    while (countFloorTiles(result.map) < desiredFloors &&
           diggerCount < diggerLimit) {
        bool didSomething = false;
        Point digger = startDigger(
            result.map, result.playerStart, settings.spawnMode, rng);
        Point heading;
        const bool winding = settings.spawnMode == DrunkSpawnMode::Random &&
                             settings.drunkenLifetime <= 100;

        for (int life = settings.drunkenLifetime; life > 0; --life) {
            if (tileTraits(result.map.tileAt(digger.x, digger.y))
                    .blocksMovement) {
                didSomething = true;
            }
            paintBrushSize(result.map,
                           digger,
                           TileType::Floor,
                           settings.brushSize,
                           settings.symmetry);
            if (countFloorTiles(result.map) >= desiredFloors) {
                break;
            }
            if (winding) {
                staggerPersistent(digger, heading, width, height, rng);
            } else {
                staggerCardinal(digger, width, height, rng);
            }
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

    cullUnreachable(result.map, result.playerStart, TileType::Rock);
    sealMapEdges(result.map, TileType::Rock);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);

    validateBuildOrThrow(result, "Drunkard generation produced an invalid map");

    return result;
}

} // namespace rl
