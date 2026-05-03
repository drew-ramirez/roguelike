/**
 * **************************************************************************
 * @file     DamageSystem.cpp
 * @brief    Applies queued damage and removes defeated entities
 * **************************************************************************
 */

#include "systems/DamageSystem.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"

namespace rl {
namespace {

std::string entityName(const entt::registry& registry, entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<Name>(entity)) {
        return registry.get<Name>(entity).value;
    }
    return "Something";
}

} // namespace

void queueDamage(entt::registry& registry, entt::entity target, int amount)
{
    if (amount <= 0 || target == entt::null || !registry.valid(target)) {
        return;
    }

    if (registry.all_of<SufferDamage>(target)) {
        registry.patch<SufferDamage>(target, [amount](SufferDamage& damage) {
            damage.amount += amount;
        });
        return;
    }

    registry.emplace<SufferDamage>(target, SufferDamage{amount});
}

void applyDamageAndCleanup(entt::registry& registry,
                           GameResources& resources,
                           RunState& state)
{
    std::vector<entt::entity> dead;
    const auto view = registry.view<CombatStats, const SufferDamage>();
    for (const auto entity : view) {
        auto& stats = view.get<CombatStats>(entity);
        const auto& damage = view.get<const SufferDamage>(entity);
        stats.hp = std::max(0, stats.hp - damage.amount);

        if (stats.hp <= 0) {
            dead.push_back(entity);
        } else {
            registry.remove<SufferDamage>(entity);
        }
    }

    for (const auto entity : dead) {
        if (!registry.valid(entity)) {
            continue;
        }

        if (entity == resources.player) {
            resources.log.add("You died.");
            state = RunState::GameOver;
            registry.remove<SufferDamage>(entity);
            continue;
        }

        resources.log.add(entityName(registry, entity) + " dies.");
        registry.destroy(entity);
    }
}

} // namespace rl
