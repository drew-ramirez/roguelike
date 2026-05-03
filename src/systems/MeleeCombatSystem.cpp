/**
 * **************************************************************************
 * @file     MeleeCombatSystem.cpp
 * @brief    Converts melee intents into damage and log messages
 * **************************************************************************
 */

#include "systems/MeleeCombatSystem.hpp"

#include <algorithm>
#include <string>

#include "core/GameLog.hpp"
#include "ecs/Components.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/EquipmentSystem.hpp"

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

void processMeleeCombat(entt::registry& registry, GameLog& log)
{
    const auto view = registry.view<const WantsToMelee, const CombatStats>();
    for (const auto attacker : view) {
        const auto& melee = view.get<const WantsToMelee>(attacker);
        if (!registry.valid(melee.target) ||
            !registry.all_of<CombatStats>(melee.target)) {
            continue;
        }

        const int damage =
            std::max(0,
                     meleePowerWithEquipment(registry, attacker) -
                         defenseWithEquipment(registry, melee.target));
        const std::string attackerName = entityName(registry, attacker);
        const std::string targetName = entityName(registry, melee.target);

        if (damage <= 0) {
            log.add(attackerName + " cannot hurt " + targetName + ".");
            continue;
        }

        log.add(attackerName + " hits " + targetName + " for " +
                std::to_string(damage) + ".");
        queueDamage(registry, melee.target, damage);
    }

    registry.clear<WantsToMelee>();
}

} // namespace rl
