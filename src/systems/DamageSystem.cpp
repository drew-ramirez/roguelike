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
#include "ecs/EntityState.hpp"
#include "map/Map.hpp"
#include "progression/ExperienceSystem.hpp"
#include "systems/ParticleSystem.hpp"

namespace rl {
namespace {

void markBlood(Map* map, const entt::registry& registry, entt::entity entity)
{
    if (map == nullptr || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return;
    }

    const auto& position = registry.get<Position>(entity);
    if (map->isWalkable(position.x, position.y)) {
        map->setBloodstain(position.x, position.y);
    }
}

void requestDamageAt(GameResources& resources,
                     const entt::registry& registry,
                     entt::entity entity)
{
    if (!registry.valid(entity) || !registry.all_of<Position>(entity)) {
        return;
    }

    const auto& position = registry.get<Position>(entity);
    requestDamageParticle(resources, Point{position.x, position.y});
}

void claimArtifact(GameResources& resources)
{
    resources.progress.artifactClaimed = true;
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
                           RunState& state,
                           Map* map)
{
    std::vector<entt::entity> dead;
    const auto view = registry.view<CombatStats, const SufferDamage>();
    for (const auto entity : view) {
        auto& stats = view.get<CombatStats>(entity);
        const auto& damage = view.get<const SufferDamage>(entity);
        markBlood(map, registry, entity);
        requestDamageAt(resources, registry, entity);
        stats.hp = std::max(0, stats.hp - damage.amount);
        if (registry.all_of<Pools>(entity)) {
            auto& pools = registry.get<Pools>(entity);
            pools.maxHitPoints = stats.maxHp;
            pools.hitPoints = stats.hp;
        }

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

        markBlood(map, registry, entity);
        resources.log.add(entityName(registry, entity) + " dies.");
        resources.log.incrementCounter("kills");
        if (registry.all_of<GrantsWin>(entity)) {
            claimArtifact(resources);
            resources.log.add("The artifact is claimed.");
            resources.log.incrementCounter("artifact_claimed");
            state = RunState::GameOver;
        }
        awardExperienceForKill(registry, resources, entity);
        registry.destroy(entity);
    }
}

} // namespace rl
