#include "systems/MapTransitionSystem.hpp"

#include <optional>
#include <string>
#include <utility>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MasterDungeonMap.hpp"
#include "map/TileType.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/VendorSystem.hpp"
#include "systems/VisibilitySystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

[[nodiscard]] bool playerIsOnTile(const entt::registry& registry,
                                  const GameResources& resources,
                                  const Map& map,
                                  TileType tile)
{
    if (resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Position>(resources.player)) {
        return false;
    }

    const auto& position = registry.get<Position>(resources.player);
    return map.tileAt(position.x, position.y) == tile;
}

[[nodiscard]] std::optional<Point> findTile(const Map& map, TileType tile)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.tileAt(x, y) == tile) {
                return Point{x, y};
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] MapBuildResult
buildTargetLevel(GameResources& resources, Map& map, int targetDepth)
{
    if (resources.mapBuilderId == "normal") {
        return buildLevelForDepth(targetDepth,
                                  map.width(),
                                  map.height(),
                                  resources.rng,
                                  MapBuildOptions{false, targetDepth});
    }
    return buildMapById(resources.mapBuilderId,
                        map.width(),
                        map.height(),
                        resources.rng,
                        MapBuildOptions{false, targetDepth});
}

void spawnGeneratedLevelEntities(entt::registry& registry,
                                 GameResources& resources,
                                 Map& map,
                                 const MapBuildResult& build)
{
    [[maybe_unused]] const auto requested =
        spawnRequestedEntities(registry,
                               build.spawnRequests,
                               map,
                               build.playerStart,
                               build.stairsPosition);
    populateAllVendorStock(registry, defaultRawDatabase());
    if (resources.mapBuilderId == "normal" && resources.depth == 1) {
        return;
    }
    [[maybe_unused]] const auto monsters = spawnMonsters(registry,
                                                         build.spawnRegions,
                                                         map,
                                                         resources.rng,
                                                         build.playerStart,
                                                         resources.depth);
    [[maybe_unused]] const auto items = spawnItems(registry,
                                                   build.spawnRegions,
                                                   map,
                                                   resources.rng,
                                                   build.playerStart,
                                                   resources.depth);
    [[maybe_unused]] const auto traps = spawnTraps(registry,
                                                   build.spawnRegions,
                                                   map,
                                                   resources.rng,
                                                   build.playerStart,
                                                   build.stairsPosition,
                                                   resources.depth);
}

[[nodiscard]] Point arrivalPosition(const Map& map,
                                    const MapBuildResult* generated,
                                    LevelArrivalMode arrivalMode)
{
    if (arrivalMode == LevelArrivalMode::UpStairs) {
        if (const auto up = findTile(map, TileType::UpStairs)) {
            return *up;
        }
        return generated != nullptr ? generated->playerStart : Point{1, 1};
    }
    if (arrivalMode == LevelArrivalMode::DownStairs) {
        if (const auto down = findTile(map, TileType::DownStairs)) {
            return *down;
        }
        return generated != nullptr ? generated->stairsPosition : Point{1, 1};
    }
    return generated != nullptr ? generated->playerStart : Point{1, 1};
}

[[nodiscard]] Rng::Seed surfaceScreenSeed(const WorldCellDescriptor& cell,
                                          Point screen,
                                          bool dangerEncounter) noexcept
{
    return cell.localSeed + static_cast<Rng::Seed>((screen.y * 31) + screen.x) +
           (dangerEncounter ? 9000U : 0U);
}

[[nodiscard]] std::string currentSurfaceBuilderId(const WorldMapState& world)
{
    return describeLocalScreen(world,
                               world.currentMacro,
                               world.currentScreen,
                               world.currentDangerEncounter)
        .builderId;
}

void storeCurrentSurfaceScreen(entt::registry& registry,
                               GameResources& resources,
                               const Map& map)
{
    if (!resources.world.active || !resources.world.onSurface) {
        return;
    }

    SurfaceScreenState state;
    state.key = SurfaceScreenKey{resources.world.currentMacro,
                                 resources.world.currentScreen,
                                 resources.world.currentDangerEncounter};
    state.builderId = currentSurfaceBuilderId(resources.world);
    state.map = map;
    state.entities = captureLevelLocalEntities(registry, resources.player);
    resources.world.surfaceScreens[state.key] = std::move(state);
}

[[nodiscard]] MapBuildOptions
surfaceBuildOptions(const WorldCellDescriptor& cell,
                    const LocalScreenDescriptor& descriptor)
{
    MapBuildOptions options;
    options.biomeVariant = cell.biome;
    options.screenRole = descriptor.role;
    options.worldFields = cell.fields;
    options.exitMask = descriptor.exitMask;
    options.landmarkId = descriptor.landmarkId;
    options.dangerEncounter = descriptor.dangerEncounter;
    options.allowDownStairs = descriptor.role == LocalScreenRole::CaveMouth;
    return options;
}

} // namespace

bool playerIsOnDownStairs(const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    return playerIsOnTile(registry, resources, map, TileType::DownStairs);
}

bool playerIsOnUpStairs(const entt::registry& registry,
                        const GameResources& resources,
                        const Map& map)
{
    return playerIsOnTile(registry, resources, map, TileType::UpStairs);
}

bool tryChangeLevel(entt::registry& registry,
                    GameResources& resources,
                    Map& map,
                    IFovProvider& fovProvider,
                    int targetDepth,
                    LevelArrivalMode arrivalMode)
{
    if (targetDepth < 1) {
        resources.log.add("There is no way up here.");
        return false;
    }

    resources.dungeon.storeLevel(resources.depth,
                                 resources.mapBuilderId,
                                 map,
                                 registry,
                                 resources.player);
    destroyLevelLocalEntities(registry, resources.player);
    resources.depth = targetDepth;

    std::optional<MapBuildResult> generated;
    if (resources.dungeon.hasLevel(targetDepth)) {
        resources.dungeon.restoreLevel(targetDepth, map, registry);
    } else {
        generated = buildTargetLevel(resources, map, targetDepth);
        map = std::move(generated->map);
        spawnGeneratedLevelEntities(registry, resources, map, *generated);
    }

    const Point arrival = arrivalPosition(
        map, generated.has_value() ? &*generated : nullptr, arrivalMode);
    auto& position = registry.get<Position>(resources.player);
    position.x = arrival.x;
    position.y = arrival.y;
    if (registry.all_of<Viewshed>(resources.player)) {
        registry.patch<Viewshed>(resources.player, [](Viewshed& viewshed) {
            viewshed.dirty = true;
        });
    }

    resources.log.add("You arrive at depth " + std::to_string(resources.depth) +
                      ".");
    updateVisibility(registry, map, resources.player, fovProvider);
    return true;
}

bool tryDescendToNextLevel(entt::registry& registry,
                           GameResources& resources,
                           Map& map,
                           IFovProvider& fovProvider)
{
    if (!playerIsOnDownStairs(registry, resources, map)) {
        resources.log.add("There is no way down here.");
        return false;
    }
    return tryChangeLevel(registry,
                          resources,
                          map,
                          fovProvider,
                          resources.depth + 1,
                          LevelArrivalMode::UpStairs);
}

bool tryAscendToPreviousLevel(entt::registry& registry,
                              GameResources& resources,
                              Map& map,
                              IFovProvider& fovProvider)
{
    if (!playerIsOnUpStairs(registry, resources, map)) {
        resources.log.add("There is no way up here.");
        return false;
    }
    return tryChangeLevel(registry,
                          resources,
                          map,
                          fovProvider,
                          resources.depth - 1,
                          LevelArrivalMode::DownStairs);
}

bool tryEnterSurfaceScreen(entt::registry& registry,
                           GameResources& resources,
                           Map& map,
                           IFovProvider& fovProvider,
                           Point macro,
                           Point screen,
                           bool dangerEncounter)
{
    if (!resources.world.active || !worldPointInBounds(macro) ||
        !localScreenInBounds(screen)) {
        resources.log.add("The world map is not available.");
        return false;
    }

    const auto* cell = worldCellAt(resources.world, macro);
    if (cell == nullptr) {
        resources.log.add("The selected region is missing.");
        return false;
    }

    storeCurrentSurfaceScreen(registry, resources, map);
    destroyLevelLocalEntities(registry, resources.player);

    resources.world.onSurface = true;
    resources.world.currentMacro = macro;
    resources.world.currentScreen = screen;
    resources.world.currentDangerEncounter = dangerEncounter;
    resources.world.cursor = macro;

    const auto descriptor =
        describeLocalScreen(resources.world, macro, screen, dangerEncounter);
    const SurfaceScreenKey key{macro, screen, dangerEncounter};
    const auto cached = resources.world.surfaceScreens.find(key);
    Point arrival{map.width() / 2, map.height() / 2};
    if (cached != resources.world.surfaceScreens.end()) {
        map = cached->second.map;
        arrival = Point{map.width() / 2, map.height() / 2};
        restoreStoredEntities(registry, cached->second.entities);
    } else {
        const auto* targetCell = worldCellAt(resources.world, macro);
        if (targetCell == nullptr) {
            return false;
        }
        Rng localRng{surfaceScreenSeed(*targetCell, screen, dangerEncounter)};
        auto build = buildMapById(descriptor.builderId,
                                  map.width(),
                                  map.height(),
                                  localRng,
                                  surfaceBuildOptions(*targetCell, descriptor));
        arrival = build.playerStart;
        map = std::move(build.map);
        [[maybe_unused]] const auto requested =
            spawnRequestedEntities(registry,
                                   build.spawnRequests,
                                   map,
                                   build.playerStart,
                                   build.stairsPosition);
        populateAllVendorStock(registry, defaultRawDatabase());
    }

    if (resources.player != entt::null && registry.valid(resources.player) &&
        registry.all_of<Position>(resources.player)) {
        auto& position = registry.get<Position>(resources.player);
        position.x = arrival.x;
        position.y = arrival.y;
        if (registry.all_of<Viewshed>(resources.player)) {
            registry.patch<Viewshed>(resources.player, [](Viewshed& viewshed) {
                viewshed.dirty = true;
            });
        }
    }

    const auto* entered = worldCellAt(resources.world, macro);
    const bool landmarkWasUnlocked =
        entered != nullptr && entered->landmark.has_value() &&
        entered->landmark->fastTravelUnlocked;
    updateCurrentWorldRegion(resources.world);
    if (!landmarkWasUnlocked &&
        unlockCurrentLandmarkFastTravel(resources.world)) {
        resources.log.add("Landmark mapped.");
    }
    if (entered != nullptr) {
        resources.log.add("Entered " + entered->name + ".");
    }
    updateVisibility(registry, map, resources.player, fovProvider);
    return true;
}

} // namespace rl
