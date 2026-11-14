#include "spawn/Spawner.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/SpatialIndexSystem.hpp"
#include "systems/VendorSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool canUseRequestPoint(const entt::registry& registry,
                                      const Map& map,
                                      Point point,
                                      Point playerStart,
                                      Point stairsPosition)
{
    const auto tileContent = tileContentAt(registry, point);
    return point != playerStart && point != stairsPosition &&
           map.isWalkable(point.x, point.y) &&
           !tileContent.blocker.has_value() && !tileContent.item.has_value();
}

} // namespace

std::size_t populateGeneratedLevel(entt::registry& registry,
                                   GameResources& resources,
                                   const Map& map,
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
        return 0;
    }
    const auto monsters = spawnMonsters(registry,
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
    return monsters;
}

entt::entity spawnPlayer(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'@', Color::white(), Color::black(), 10});
    registry.emplace<Name>(entity, Name{"Player"});
    registry.emplace<Player>(entity);
    registry.emplace<BlocksTile>(entity);
    registry.emplace<Viewshed>(entity);
    registry.emplace<CombatStats>(entity, CombatStats{30, 30, 2, 5});
    registry.emplace<Attributes>(entity, Attributes{11, 11, 11, 11});
    registry.emplace<Pools>(entity, Pools{30, 30, 6, 6, 1, 0, 0});
    registry.emplace<HungerClock>(entity);
    return entity;
}

std::size_t spawnRequestedEntities(entt::registry& registry,
                                   const std::vector<SpawnRequest>& requests,
                                   const Map& map,
                                   Point playerStart,
                                   Point stairsPosition)
{
    std::size_t spawned = 0;
    for (const auto& request : requests) {
        if (!canUseRequestPoint(
                registry, map, request.position, playerStart, stairsPosition)) {
            continue;
        }

        if (const auto entity = spawnRawEntity(
                registry, defaultRawDatabase(), request.id, request.position);
            !entity.has_value()) {
            continue;
        }
        ++spawned;
    }
    return spawned;
}

} // namespace rl
