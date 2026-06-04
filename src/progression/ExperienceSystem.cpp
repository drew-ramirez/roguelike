/**
 * **************************************************************************
 * @file     ExperienceSystem.cpp
 * @brief    Applies monster XP rewards and player level advancement
 * **************************************************************************
 */

#include "progression/ExperienceSystem.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"

namespace rl {
namespace {

constexpr int xpPerLevel = 1000;
constexpr int hitPointsPerLevel = 5;

void syncCombatHitPoints(entt::registry& registry,
                         entt::entity entity,
                         const Pools& pools)
{
    if (!registry.valid(entity) || !registry.all_of<CombatStats>(entity)) {
        return;
    }

    auto& stats = registry.get<CombatStats>(entity);
    stats.maxHp = pools.maxHitPoints;
    stats.hp = pools.hitPoints;
}

void advancePlayerLevel(entt::registry& registry,
                        GameResources& resources,
                        Pools& pools)
{
    ++pools.level;
    pools.maxHitPoints += hitPointsPerLevel;
    pools.hitPoints = pools.maxHitPoints;
    syncCombatHitPoints(registry, resources.player, pools);
    resources.log.add("You advance to level " + std::to_string(pools.level) +
                      ".");
}

} // namespace

int xpThresholdForLevel(int level) noexcept
{
    return std::max(1, level) * xpPerLevel;
}

void awardExperienceForKill(entt::registry& registry,
                            GameResources& resources,
                            entt::entity defeated)
{
    if (!registry.valid(defeated) || !registry.all_of<XpReward>(defeated) ||
        resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Pools>(resources.player)) {
        return;
    }

    const int reward = registry.get<XpReward>(defeated).amount;
    if (reward <= 0) {
        return;
    }

    auto& pools = registry.get<Pools>(resources.player);
    pools.level = std::max(1, pools.level);
    pools.xp += reward;
    resources.log.add("You gain " + std::to_string(reward) + " xp.");

    while (pools.xp >= xpThresholdForLevel(pools.level)) {
        pools.xp -= xpThresholdForLevel(pools.level);
        advancePlayerLevel(registry, resources, pools);
    }
}

} // namespace rl
