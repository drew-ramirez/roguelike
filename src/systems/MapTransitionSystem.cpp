#include "systems/MapTransitionSystem.hpp"

#include <string>
#include <utility>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "map/TileType.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/VisibilitySystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool isPersistentEntity(const entt::registry& registry,
                                      const GameResources& resources,
                                      entt::entity entity)
{
    if (entity == resources.player) {
        return true;
    }
    if (registry.all_of<InBackpack>(entity) &&
        registry.get<InBackpack>(entity).owner == resources.player) {
        return true;
    }
    return registry.all_of<Equipped>(entity) &&
           registry.get<Equipped>(entity).owner == resources.player;
}

void destroyLevelLocalEntities(entt::registry& registry,
                               const GameResources& resources)
{
    std::vector<entt::entity> doomed;
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        if (!isPersistentEntity(registry, resources, entity)) {
            doomed.push_back(entity);
        }
    }

    for (const auto entity : doomed) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
}

} // namespace

bool playerIsOnDownStairs(const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    if (resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Position>(resources.player)) {
        return false;
    }

    const auto& position = registry.get<Position>(resources.player);
    return map.tileAt(position.x, position.y) == TileType::DownStairs;
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

    ++resources.depth;
    destroyLevelLocalEntities(registry, resources);

    auto build =
        SimpleMapBuilder::build(map.width(), map.height(), resources.rng);
    map = std::move(build.map);

    auto& position = registry.get<Position>(resources.player);
    position.x = build.playerStart.x;
    position.y = build.playerStart.y;
    if (registry.all_of<Viewshed>(resources.player)) {
        registry.patch<Viewshed>(resources.player, [](Viewshed& viewshed) {
            viewshed.dirty = true;
        });
    }

    [[maybe_unused]] const auto monsters = spawnMonsters(registry,
                                                         build.rooms,
                                                         resources.rng,
                                                         build.playerStart,
                                                         resources.depth);
    [[maybe_unused]] const auto items = spawnItems(registry,
                                                   build.rooms,
                                                   resources.rng,
                                                   build.playerStart,
                                                   resources.depth);
    [[maybe_unused]] const auto traps = spawnTraps(registry,
                                                   build.rooms,
                                                   resources.rng,
                                                   build.playerStart,
                                                   build.stairsPosition,
                                                   resources.depth);
    resources.log.add("You descend to depth " +
                      std::to_string(resources.depth) + ".");
    updateVisibility(registry, map, resources.player, fovProvider);
    return true;
}

} // namespace rl
