#include "systems/MeleeCombatSystem.hpp"

#include <algorithm>
#include <string>

#include "core/GameLog.hpp"
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

void addDamage(entt::registry& registry, entt::entity target, int amount)
{
    if (registry.all_of<SufferDamage>(target)) {
        registry.patch<SufferDamage>(target, [amount](SufferDamage& damage) {
            damage.amount += amount;
        });
        return;
    }

    registry.emplace<SufferDamage>(target, SufferDamage{amount});
}

} // namespace

void processMeleeCombat(entt::registry& registry, GameLog& log)
{
    const auto view = registry.view<const WantsToMelee, const CombatStats>();
    for (const auto attacker : view) {
        const auto& melee = view.get<const WantsToMelee>(attacker);
        if (!registry.valid(melee.target) ||
            !registry.all_of<CombatStats>(melee.target)) {
            continue;
        }

        const auto& attackerStats = view.get<const CombatStats>(attacker);
        const auto& defenderStats = registry.get<CombatStats>(melee.target);
        const int damage =
            std::max(0, attackerStats.power - defenderStats.defense);
        const std::string attackerName = entityName(registry, attacker);
        const std::string targetName = entityName(registry, melee.target);

        if (damage <= 0) {
            log.add(attackerName + " cannot hurt " + targetName + ".");
            continue;
        }

        log.add(attackerName + " hits " + targetName + " for " +
                std::to_string(damage) + ".");
        addDamage(registry, melee.target, damage);
    }

    registry.clear<WantsToMelee>();
}

} // namespace rl
