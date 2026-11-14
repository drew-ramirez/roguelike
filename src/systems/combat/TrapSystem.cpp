/**
 * **************************************************************************
 * @file     TrapSystem.cpp
 * @brief    Reveals and disables spike traps stepped on by the player
 * **************************************************************************
 */

#include "systems/combat/TrapSystem.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "systems/combat/DamageSystem.hpp"
#include "systems/rendering/ParticleSystem.hpp"

namespace rl {

void processPlayerTraps(entt::registry& registry, GameResources& resources)
{
    const auto player = resources.player;
    if (player == entt::null || !registry.valid(player) ||
        !registry.all_of<Position>(player)) {
        return;
    }

    const auto& playerPosition = registry.get<Position>(player);
    const Point playerPoint{playerPosition.x, playerPosition.y};
    const auto traps = registry.view<Position, Trap>();
    for (const auto trapEntity : traps) {
        auto& trapPosition = traps.get<Position>(trapEntity);
        auto& trap = traps.get<Trap>(trapEntity);
        if (trap.triggered || trapPosition.x != playerPoint.x ||
            trapPosition.y != playerPoint.y) {
            continue;
        }

        trap.triggered = true;
        if (registry.all_of<Hidden>(trapEntity)) {
            registry.remove<Hidden>(trapEntity);
        }
        resources.log.add("A hidden spike trap snaps shut!");
        requestTrapParticle(resources, playerPoint);
        queueDamage(registry, player, trap.damage);
    }
}

} // namespace rl
