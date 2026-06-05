/**
 * **************************************************************************
 * @file     CampSystem.cpp
 * @brief    Resolves camp rest actions for surface survival
 * **************************************************************************
 */

#include "systems/CampSystem.hpp"

#include <algorithm>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "systems/HungerSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {

bool canRestAtCamp(const GameResources& resources,
                   const entt::registry& registry,
                   entt::entity camp)
{
    const auto* region = currentWorldRegion(resources.world);
    if (region == nullptr || !registry.valid(camp) ||
        !registry.all_of<CampSite>(camp)) {
        return false;
    }

    const auto& campSite = registry.get<CampSite>(camp);
    const int safety =
        region->baseSafety + campSite.safetyBonus - region->dangerTier;
    return safety >= 0;
}

bool restAtCamp(entt::registry& registry,
                GameResources& resources,
                entt::entity camp)
{
    if (!canRestAtCamp(resources, registry, camp)) {
        resources.log.add("This camp does not feel safe enough to rest.");
        return false;
    }
    if (resources.player == entt::null || !registry.valid(resources.player)) {
        return false;
    }

    if (registry.all_of<CombatStats>(resources.player)) {
        auto& stats = registry.get<CombatStats>(resources.player);
        stats.hp = std::min(stats.maxHp, stats.hp + 4);
        if (registry.all_of<Pools>(resources.player)) {
            auto& pools = registry.get<Pools>(resources.player);
            pools.hitPoints = stats.hp;
            pools.maxHitPoints = stats.maxHp;
        }
    }

    consumeHunger(registry, resources, 20);
    resources.log.add("You rest at camp.");
    return true;
}

} // namespace rl
