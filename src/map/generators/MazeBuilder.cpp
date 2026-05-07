/**
 * **************************************************************************
 * @file     MazeBuilder.cpp
 * @brief    Builds perfect mazes with a backtracking carver
 * **************************************************************************
 */

#include "map/generators/MazeBuilder.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

#include "map/MapBuilderCommon.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

[[nodiscard]] Point twoStep(Point point, Point delta)
{
    // Maze cells live on every other tile, the skipped tile between them
    // remains available as the corridor wall we carve through
    return Point{point.x + (delta.x * 2), point.y + (delta.y * 2)};
}

[[nodiscard]] bool canCarve(const Map& map, Point point)
{
    return point.x > 0 && point.y > 0 && point.x < map.width() - 1 &&
           point.y < map.height() - 1 &&
           map.tileAt(point.x, point.y) == TileType::Wall;
}

void shuffleDirections(std::array<Point, 4>& directions, Rng& rng)
{
    for (int index = static_cast<int>(directions.size()) - 1; index > 0;
         --index) {
        const int swapIndex = rng.rangeInclusive(0, index);
        std::swap(directions[static_cast<std::size_t>(index)],
                  directions[static_cast<std::size_t>(swapIndex)]);
    }
}

} // namespace

MapBuildResult
MazeBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    validateBuilderDimensions("MazeBuilder", width, height);

    MapBuildResult result{Map{width, height, TileType::Wall}, {}, {}};
    result.builderName = "maze";
    result.playerStart = Point{1, 1};
    result.map.setTile(1, 1, TileType::Floor);
    addBuildStep(result, "base: recursive backtracker maze");

    std::vector<Point> stack{result.playerStart};
    constexpr std::array<Point, 4> baseDirections{
        Point{1, 0},
        Point{-1, 0},
        Point{0, 1},
        Point{0, -1},
    };

    while (!stack.empty()) {
        const Point current = stack.back();
        auto directions = baseDirections;
        shuffleDirections(directions, rng);

        bool carved = false;
        for (const Point delta : directions) {
            const Point next = twoStep(current, delta);
            if (!canCarve(result.map, next)) {
                continue;
            }

            // Carve the wall between the current cell and the destination,
            // then carve the destination, preserves one-tile corridors
            result.map.setTile(
                current.x + delta.x, current.y + delta.y, TileType::Floor);
            result.map.setTile(next.x, next.y, TileType::Floor);
            stack.push_back(next);
            carved = true;
            break;
        }

        if (!carved) {
            stack.pop_back();
        }
    }

    recordSnapshot(
        result.snapshots, result.map, "carved maze", options.recordSnapshots);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Corridor);
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "Maze generation produced an invalid map");

    return result;
}

} // namespace rl
