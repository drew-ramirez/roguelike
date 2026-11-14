/**
 * **************************************************************************
 * @file     MeleeCombatSystem.cpp
 * @brief    Converts melee intents into damage and log messages
 * **************************************************************************
 */

#include "systems/combat/MeleeCombatSystem.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "core/Dice.hpp"
#include "core/GameLog.hpp"
#include "core/Rng.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "systems/combat/DamageSystem.hpp"
#include "systems/inventory/EquipmentSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::optional<WeaponStats>
equippedMeleeWeapon(const entt::registry& registry, entt::entity owner)
{
    const auto view = registry.view<const Equipped, const WeaponStats>();
    for (const auto item : view) {
        const auto& equipped = view.get<const Equipped>(item);
        if (equipped.owner == owner && equipped.slot == EquipmentSlot::Weapon) {
            return view.get<const WeaponStats>(item);
        }
    }
    return std::nullopt;
}

[[nodiscard]] int statBasedAttackPower(const entt::registry& registry,
                                       entt::entity attacker,
                                       Rng& rng)
{
    if (attacker == entt::null || !registry.valid(attacker) ||
        !registry.all_of<CombatStats>(attacker)) {
        return 0;
    }

    const auto weapon = equippedMeleeWeapon(registry, attacker);
    if (!weapon.has_value()) {
        return meleePowerWithEquipment(registry, attacker) +
               attributeModifierWithEquipment(registry, attacker, "might");
    }

    return registry.get<CombatStats>(attacker).power +
           rollDice(parseDice(weapon->damage), rng) + weapon->hitBonus +
           attributeModifierWithEquipment(
               registry, attacker, weapon->attribute);
}

} // namespace

void processMeleeCombat(entt::registry& registry, GameLog& log, Rng* rng)
{
    const auto view = registry.view<const WantsToMelee, const CombatStats>();
    for (const auto attacker : view) {
        const auto& melee = view.get<const WantsToMelee>(attacker);
        if (!registry.valid(melee.target) ||
            !registry.all_of<CombatStats>(melee.target)) {
            continue;
        }

        const int attackPower =
            rng == nullptr ? meleePowerWithEquipment(registry, attacker)
                           : statBasedAttackPower(registry, attacker, *rng);
        const int damage = std::max(
            0, attackPower - defenseWithEquipment(registry, melee.target));
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
