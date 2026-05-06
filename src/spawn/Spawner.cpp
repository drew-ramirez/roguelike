#include "spawn/Spawner.hpp"

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool canUseRequestPoint(const entt::registry& registry,
                                      const Map& map,
                                      Point point,
                                      Point playerStart,
                                      Point stairsPosition)
{
    return point != playerStart && point != stairsPosition &&
           map.isWalkable(point.x, point.y) &&
           !blockingEntityAt(registry, point).has_value() &&
           !itemAtPosition(registry, point).has_value();
}

} // namespace

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

        if (request.id == "goblin") {
            [[maybe_unused]] const auto entity =
                spawnGoblin(registry, request.position);
        } else if (request.id == "orc") {
            [[maybe_unused]] const auto entity =
                spawnOrc(registry, request.position);
        } else if (request.id == "healing_potion") {
            [[maybe_unused]] const auto entity =
                spawnHealingPotion(registry, request.position);
        } else if (request.id == "food_ration") {
            [[maybe_unused]] const auto entity =
                spawnFoodRation(registry, request.position);
        } else if (request.id == "spike_trap") {
            [[maybe_unused]] const auto entity =
                spawnSpikeTrap(registry, request.position);
        } else {
            continue;
        }
        ++spawned;
    }
    return spawned;
}

} // namespace rl
