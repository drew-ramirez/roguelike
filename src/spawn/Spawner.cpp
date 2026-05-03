#include "spawn/Spawner.hpp"

#include "ecs/Components.hpp"

namespace rl {

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
    return entity;
}

} // namespace rl
