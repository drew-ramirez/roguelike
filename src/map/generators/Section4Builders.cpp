/**
 * **************************************************************************
 * @file     Section4Builders.cpp
 * @brief    Builds authored gameplay region maps
 * **************************************************************************
 */

#include "map/generators/Section4Builders.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/Rng.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/Rect.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

void carveRect(Map& map, const Rect& rect)
{
    for (int y = rect.y1; y < rect.y2; ++y) {
        for (int x = rect.x1; x < rect.x2; ++x) {
            if (map.inBounds(x, y)) {
                map.setTile(x, y, TileType::Floor);
            }
        }
    }
}

void carveHorizontal(Map& map, int y, int x1, int x2)
{
    const int begin = std::max(1, std::min(x1, x2));
    const int end = std::min(map.width() - 2, std::max(x1, x2));
    for (int x = begin; x <= end; ++x) {
        map.setTile(x, y, TileType::Floor);
    }
}

void carveVertical(Map& map, int x, int y1, int y2)
{
    const int begin = std::max(1, std::min(y1, y2));
    const int end = std::min(map.height() - 2, std::max(y1, y2));
    for (int y = begin; y <= end; ++y) {
        map.setTile(x, y, TileType::Floor);
    }
}

void addScatterFloors(Map& map, Rng& rng, int count, char mode)
{
    for (int i = 0; i < count; ++i) {
        const int x = rng.rangeInclusive(1, map.width() - 2);
        const int y = rng.rangeInclusive(1, map.height() - 2);
        if (mode == 'c') {
            for (int yy = std::max(1, y - 1);
                 yy <= std::min(map.height() - 2, y + 1);
                 ++yy) {
                for (int xx = std::max(1, x - 1);
                     xx <= std::min(map.width() - 2, x + 1);
                     ++xx) {
                    map.setTile(xx, yy, TileType::Floor);
                }
            }
        } else {
            map.setTile(x, y, TileType::Floor);
        }
    }
}

void finalizeRegion(MapBuildResult& result,
                    std::string builderName,
                    bool snapshots,
                    bool hasDownStairs = true)
{
    result.builderName = std::move(builderName);
    if (hasDownStairs) {
        setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    } else {
        result.stairsPosition = result.playerStart;
    }
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    recordSnapshot(result.snapshots, result.map, "region final", snapshots);
    if (!result.map.isWalkable(result.playerStart.x, result.playerStart.y) ||
        !result.map.isWalkable(result.stairsPosition.x,
                               result.stairsPosition.y)) {
        throw std::runtime_error("Region builder failed validation");
    }
}

bool addSpawnRequestIfOpen(MapBuildResult& result, std::string id, Point point)
{
    if (point == result.playerStart || point == result.stairsPosition ||
        !result.map.isWalkable(point.x, point.y)) {
        return false;
    }
    const bool occupied =
        std::any_of(result.spawnRequests.begin(),
                    result.spawnRequests.end(),
                    [point](const SpawnRequest& request) {
                        return request.position == point;
                    });
    if (occupied) {
        return false;
    }

    result.spawnRequests.push_back(SpawnRequest{std::move(id), point});
    return true;
}

} // namespace

MapBuildResult Section4Builders::buildTown(int width,
                                           int height,
                                           [[maybe_unused]] Rng& rng,
                                           MapBuildOptions options)
{
    validateBuilderDimensions("town", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    const int midX = width / 2;
    const int midY = height / 2;

    carveHorizontal(result.map, midY, 1, width - 2);
    carveVertical(result.map, midX, 1, height - 2);
    carveRect(result.map, Rect{3, 3, 12, 9});
    carveRect(result.map, Rect{width - 13, 3, width - 4, 9});
    carveRect(result.map, Rect{3, height - 10, 13, height - 4});
    carveRect(result.map, Rect{width - 15, height - 10, width - 4, height - 4});
    carveRect(result.map, Rect{midX - 4, midY - 3, midX + 5, midY + 4});
    carveHorizontal(result.map, 6, 11, midX);
    carveHorizontal(result.map, 6, midX, width - 13);
    carveHorizontal(result.map, height - 7, 12, midX);
    carveHorizontal(result.map, height - 7, midX, width - 15);
    sealMapEdges(result.map);

    result.playerStart = Point{midX, midY};
    result.spawnRequests.push_back(SpawnRequest{"blacksmith", Point{7, 5}});
    result.spawnRequests.push_back(
        SpawnRequest{"alchemist", Point{width - 8, 5}});
    result.spawnRequests.push_back(
        SpawnRequest{"townsfolk", Point{6, height - 7}});
    result.spawnRequests.push_back(
        SpawnRequest{"townsfolk", Point{width - 8, height - 7}});
    result.spawnRequests.push_back(
        SpawnRequest{"chair", Point{midX - 2, midY - 1}});

    finalizeRegion(result, "town", options.recordSnapshots);
    return result;
}

MapBuildResult Section4Builders::buildWoods(int width,
                                            int height,
                                            Rng& rng,
                                            MapBuildOptions options)
{
    validateBuilderDimensions("woods", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    const int roadX = width / 2;
    carveVertical(result.map, roadX, 1, height - 2);
    carveHorizontal(result.map, height / 3, roadX - 8, roadX + 8);
    addScatterFloors(result.map, rng, (width * height) / 6, 'c');
    repairConnectivity(result.map, Point{roadX, 2}, rng);
    cullUnreachable(result.map, Point{roadX, 2});
    result.playerStart = Point{roadX, 2};
    finalizeRegion(result, "woods", options.recordSnapshots);
    const int clearingY = height / 3;
    addSpawnRequestIfOpen(result, "wolf", Point{roadX - 6, clearingY});
    addSpawnRequestIfOpen(result, "wild_boar", Point{roadX + 6, clearingY});
    addSpawnRequestIfOpen(result, "food_ration", Point{roadX - 2, clearingY});
    addSpawnRequestIfOpen(
        result, "healing_potion", Point{roadX + 2, clearingY});
    addBuildStep(result, "content: woods wildlife and supplies");
    return result;
}

MapBuildResult Section4Builders::buildDragonLair(int width,
                                                 int height,
                                                 Rng& rng,
                                                 MapBuildOptions options)
{
    validateBuilderDimensions("dragon-lair", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    result.playerStart = Point{width / 2, 2};
    carveVertical(result.map, width / 2, 2, height - 6);
    carveRect(result.map,
              Rect{width / 2 - 8, height - 12, width / 2 + 9, height - 3});
    addScatterFloors(result.map, rng, (width * height) / 12, 'p');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    result.spawnRequests.push_back(
        SpawnRequest{"dragon", Point{width / 2, height - 8}});
    finalizeRegion(result, "dragon-lair", options.recordSnapshots);
    return result;
}

MapBuildResult Section4Builders::buildMushroomForest(int width,
                                                     int height,
                                                     Rng& rng,
                                                     MapBuildOptions options)
{
    validateBuilderDimensions("mushroom-forest", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    result.playerStart = Point{width / 2, 2};
    carveVertical(result.map, width / 2, 2, height - 3);
    addScatterFloors(result.map, rng, (width * height) / 4, 'c');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    finalizeRegion(result, "mushroom-forest", options.recordSnapshots);
    return result;
}

MapBuildResult Section4Builders::buildDarkElfCity(int width,
                                                  int height,
                                                  [[maybe_unused]] Rng& rng,
                                                  MapBuildOptions options)
{
    validateBuilderDimensions("dark-elf-city", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    const int midX = width / 2;
    const int midY = height / 2;
    carveHorizontal(result.map, midY, 1, width - 2);
    carveVertical(result.map, midX, 1, height - 2);
    for (int y = 3; y < height - 6; y += 8) {
        for (int x = 3; x < width - 10; x += 12) {
            carveRect(result.map, Rect{x, y, x + 7, y + 5});
            carveHorizontal(result.map, y + 2, x + 6, midX);
        }
    }
    result.playerStart = Point{midX, 2};
    result.spawnRequests.push_back(
        SpawnRequest{"dark_elf_archer", Point{midX - 5, midY}});
    result.spawnRequests.push_back(
        SpawnRequest{"dark_elf_archer", Point{midX + 5, midY}});
    finalizeRegion(result, "dark-elf-city", options.recordSnapshots, false);
    return result;
}

} // namespace rl
