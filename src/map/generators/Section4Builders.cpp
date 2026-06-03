/**
 * **************************************************************************
 * @file     Section4Builders.cpp
 * @brief    Builds authored gameplay region maps
 * **************************************************************************
 */

#include "map/generators/Section4Builders.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/Rng.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/Rect.hpp"
#include "map/TileType.hpp"
#include "map/generators/BuilderChain.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"

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
    const bool occupied = std::any_of(result.spawnRequests.begin(),
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

[[nodiscard]] int manhattanDistance(Point lhs, Point rhs) noexcept
{
    const int dx = lhs.x > rhs.x ? lhs.x - rhs.x : rhs.x - lhs.x;
    const int dy = lhs.y > rhs.y ? lhs.y - rhs.y : rhs.y - lhs.y;
    return dx + dy;
}

[[nodiscard]] std::vector<SpawnRegion>
partitionCavernSpawnRegions(const Map& map, Point start, Point stairs)
{
    constexpr int protectedEntranceDistance = 6;
    std::array<std::vector<Point>, 5> groups;
    const int midX = map.width() / 2;
    const int midY = map.height() / 2;

    for (int y = 1; y < map.height() - 1; ++y) {
        for (int x = 1; x < map.width() - 1; ++x) {
            const Point point{x, y};
            if (!map.isWalkable(x, y) || point == stairs) {
                continue;
            }

            std::size_t group = 0;
            if (manhattanDistance(point, start) > protectedEntranceDistance) {
                group = 1;
                if (x >= midX) {
                    group += 1;
                }
                if (y >= midY) {
                    group += 2;
                }
            }
            groups[group].push_back(point);
        }
    }

    std::vector<SpawnRegion> regions;
    for (auto& group : groups) {
        if (!group.empty()) {
            regions.push_back(
                spawnRegionFromTiles(std::move(group), SpawnRegionKind::Room));
        }
    }
    return regions;
}

bool addSpawnRequestFromDistantFloor(MapBuildResult& result,
                                     std::string id,
                                     std::size_t offset)
{
    std::vector<Point> candidates;
    for (std::size_t index = 1; index < result.spawnRegions.size(); ++index) {
        candidates.insert(candidates.end(),
                          result.spawnRegions[index].tiles.begin(),
                          result.spawnRegions[index].tiles.end());
    }
    std::sort(candidates.begin(), candidates.end(), [&](Point lhs, Point rhs) {
        const int lhsDistance = manhattanDistance(lhs, result.playerStart);
        const int rhsDistance = manhattanDistance(rhs, result.playerStart);
        if (lhsDistance != rhsDistance) {
            return lhsDistance > rhsDistance;
        }
        if (lhs.y != rhs.y) {
            return lhs.y < rhs.y;
        }
        return lhs.x < rhs.x;
    });

    for (std::size_t attempt = 0; attempt < candidates.size(); ++attempt) {
        const Point point = candidates[(offset + attempt) % candidates.size()];
        if (addSpawnRequestIfOpen(result, id, point)) {
            return true;
        }
    }
    return false;
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

MapBuildResult Section4Builders::buildLimestoneCaverns(int width,
                                                       int height,
                                                       Rng& rng,
                                                       MapBuildOptions options)
{
    validateBuilderDimensions("limestone-caverns", width, height, 30, 20);

    auto result = CellularAutomataBuilder::build(width, height, rng, options);
    result.builderName = "limestone-caverns";
    result.spawnRegions = partitionCavernSpawnRegions(
        result.map, result.playerStart, result.stairsPosition);

    constexpr std::size_t koboldOffset = 0;
    constexpr std::size_t batOffset = 5;
    constexpr std::size_t columnOffset = 10;
    constexpr std::size_t supplyOffset = 15;
    addSpawnRequestFromDistantFloor(result, "kobold", koboldOffset);
    addSpawnRequestFromDistantFloor(result, "cave_bat", batOffset);
    addSpawnRequestFromDistantFloor(result, "limestone_column", columnOffset);
    addSpawnRequestFromDistantFloor(result, "healing_potion", supplyOffset);
    addBuildStep(result, "theme: limestone caverns");
    addBuildStep(result, "content: cavern denizens and formations");
    recordSnapshot(result.snapshots,
                   result.map,
                   "limestone region",
                   options.recordSnapshots);
    validateBuildOrThrow(result,
                         "Limestone cavern generation produced an invalid map");
    return result;
}

MapBuildResult Section4Builders::buildDeepCaverns(int width,
                                                  int height,
                                                  Rng& rng,
                                                  MapBuildOptions options)
{
    auto result = BuilderChain::buildCavernVaults(width, height, rng, options);
    result.builderName = "deep-caverns";
    result.spawnRegions = partitionCavernSpawnRegions(
        result.map, result.playerStart, result.stairsPosition);
    addSpawnRequestFromDistantFloor(result, "deep_kobold", 0);
    addSpawnRequestFromDistantFloor(result, "stone_golem", 6);
    addSpawnRequestFromDistantFloor(result, "deep_limestone_column", 12);
    addSpawnRequestFromDistantFloor(result, "town_portal_scroll", 18);
    addSpawnRequestFromDistantFloor(result, "food_ration", 24);
    addBuildStep(result, "theme: deep caverns");
    addBuildStep(result, "content: deep denizens and portal supplies");
    recordSnapshot(result.snapshots,
                   result.map,
                   "deep cavern region",
                   options.recordSnapshots);
    validateBuildOrThrow(result,
                         "Deep cavern generation produced an invalid map");
    return result;
}

MapBuildResult Section4Builders::buildDwarfTransition(int width,
                                                      int height,
                                                      Rng& rng,
                                                      MapBuildOptions options)
{
    auto result = BuilderChain::buildRuinedRooms(width, height, rng, options);
    result.builderName = "dwarf-transition";
    addSpawnRequestFromDistantFloor(result, "dwarf_sentry", 0);
    addSpawnRequestFromDistantFloor(result, "dwarf_rubble", 5);
    addSpawnRequestFromDistantFloor(result, "chain_mail", 10);
    addSpawnRequestFromDistantFloor(result, "remove_curse_scroll", 15);
    addSpawnRequestFromDistantFloor(result, "food_ration", 20);
    addBuildStep(result, "theme: cavern to dwarf fort transition");
    addBuildStep(result, "content: ruined dwarf defenses");
    recordSnapshot(result.snapshots,
                   result.map,
                   "dwarf transition region",
                   options.recordSnapshots);
    validateBuildOrThrow(result,
                         "Dwarf transition generation produced an invalid map");
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
    addScatterFloors(result.map, rng, (width * height) / 18, 'p');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    result.spawnRequests.push_back(
        SpawnRequest{"dragon", Point{width / 2, height - 8}});
    addSpawnRequestIfOpen(
        result, "dwarf_rubble", Point{width / 2 - 4, height - 8});
    addSpawnRequestIfOpen(
        result, "gauntlets_of_might", Point{width / 2 + 4, height - 8});
    addSpawnRequestIfOpen(result, "food_ration", Point{width / 2, height - 5});
    finalizeRegion(result, "dragon-lair", options.recordSnapshots);
    addBuildStep(result, "content: dragon and dwarf fortress treasure");
    return result;
}

MapBuildResult Section4Builders::buildMushroomEntrance(int width,
                                                       int height,
                                                       Rng& rng,
                                                       MapBuildOptions options)
{
    validateBuilderDimensions("mushroom-entrance", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    result.playerStart = Point{width / 2, 2};
    carveVertical(result.map, width / 2, 2, height - 3);
    carveHorizontal(result.map, height / 2, width / 2 - 10, width / 2 + 10);
    addScatterFloors(result.map, rng, (width * height) / 18, 'c');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    finalizeRegion(result, "mushroom-entrance", options.recordSnapshots);
    addSpawnRequestIfOpen(
        result, "mushroom_patch", Point{width / 2 - 6, height / 2});
    addSpawnRequestIfOpen(
        result, "sporeling", Point{width / 2 + 6, height / 2});
    addSpawnRequestIfOpen(
        result, "food_ration", Point{width / 2, height / 2 + 2});
    addBuildStep(result, "theme: mushroom entrance");
    addBuildStep(result, "content: first fungal growths");
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
    addScatterFloors(result.map, rng, (width * height) / 20, 'c');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    finalizeRegion(result, "mushroom-forest", options.recordSnapshots);
    result.spawnRegions = partitionCavernSpawnRegions(
        result.map, result.playerStart, result.stairsPosition);
    addSpawnRequestFromDistantFloor(result, "sporeling", 0);
    addSpawnRequestFromDistantFloor(result, "fungal_giant", 6);
    addSpawnRequestFromDistantFloor(result, "mushroom_patch", 12);
    addSpawnRequestFromDistantFloor(result, "confusion_spore", 18);
    addSpawnRequestFromDistantFloor(result, "food_ration", 24);
    addBuildStep(result, "content: fungal forest encounters");
    return result;
}

MapBuildResult Section4Builders::buildMushroomExit(int width,
                                                   int height,
                                                   Rng& rng,
                                                   MapBuildOptions options)
{
    validateBuilderDimensions("mushroom-exit", width, height, 30, 20);
    MapBuildResult result{Map{width, height, TileType::Wall}};
    result.playerStart = Point{width / 2, 2};
    carveVertical(result.map, width / 2, 2, height - 3);
    carveHorizontal(result.map, height - 7, width / 2 - 12, width / 2 + 12);
    carveRect(result.map,
              Rect{width / 2 - 6, height - 11, width / 2 + 7, height - 4});
    addScatterFloors(result.map, rng, (width * height) / 18, 'c');
    repairConnectivity(result.map, result.playerStart, rng);
    cullUnreachable(result.map, result.playerStart);
    finalizeRegion(result, "mushroom-exit", options.recordSnapshots);
    addSpawnRequestIfOpen(
        result, "dark_elf_scout_a", Point{width / 2 - 5, height - 8});
    addSpawnRequestIfOpen(
        result, "sporeling", Point{width / 2 + 5, height - 8});
    addSpawnRequestIfOpen(
        result, "hand_crossbow", Point{width / 2, height - 8});
    addSpawnRequestIfOpen(result, "food_ration", Point{width / 2, height - 6});
    addBuildStep(result, "theme: mushroom exit toward dark elf city");
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
    addSpawnRequestIfOpen(result, "dark_elf_marksman_a", Point{midX - 6, midY});
    addSpawnRequestIfOpen(result, "dark_elf_blade_b", Point{midX + 6, midY});
    addSpawnRequestIfOpen(result, "dark_elf_marksman_c", Point{midX, midY + 4});
    addSpawnRequestIfOpen(result, "scimitar", Point{midX - 4, midY + 2});
    addSpawnRequestIfOpen(result, "buckler", Point{midX + 4, midY + 2});
    finalizeRegion(result, "dark-elf-city", options.recordSnapshots, false);
    addBuildStep(result, "content: rival dark elf clans");
    return result;
}
} // namespace rl
