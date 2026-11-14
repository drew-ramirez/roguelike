/**
 * **************************************************************************
 * @file     RangedCombatSystem.cpp
 * @brief    Resolves line-of-sight ranged weapon and breath attacks
 * **************************************************************************
 */

#include "systems/RangedCombatSystem.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "core/Dice.hpp"
#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "map/Map.hpp"
#include "map/MapGeometry.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::optional<RangedWeapon> rangedWeaponForIntent(
    const entt::registry& registry, entt::entity shooter, entt::entity weapon)
{
    if (registry.valid(weapon) && registry.all_of<RangedWeapon>(weapon)) {
        return registry.get<RangedWeapon>(weapon);
    }
    if (registry.valid(shooter) && registry.all_of<RangedWeapon>(shooter)) {
        return registry.get<RangedWeapon>(shooter);
    }
    return std::nullopt;
}

[[nodiscard]] bool hasLineOfSight(const Map& map, Point from, Point to)
{
    const auto line = bresenhamLine(from, to, LineStartPolicy::ExcludeStart);
    for (std::size_t index = 0; index < line.size(); ++index) {
        const Point point = line[index];
        if (point == to) {
            return true;
        }
        if (map.blocksSight(point.x, point.y)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::optional<entt::entity> combatTargetAt(
    const entt::registry& registry, Point point, entt::entity shooter)
{
    for (const auto entity : entitiesAt(registry, point, shooter)) {
        if (registry.all_of<CombatStats>(entity)) {
            return entity;
        }
    }
    return std::nullopt;
}

} // namespace

bool processRangedCombat(entt::registry& registry,
                         GameResources& resources,
                         const Map& map)
{
    bool fired = false;
    const auto view = registry.view<const WantsToShoot>();
    for (const auto shooter : view) {
        const auto& intent = view.get<const WantsToShoot>(shooter);
        const auto shooterPosition = entityPosition(registry, shooter);
        if (!shooterPosition.has_value() || !intent.target.has_value()) {
            continue;
        }

        if (intent.breath) {
            const int range = registry.all_of<BreathWeapon>(shooter)
                                  ? registry.get<BreathWeapon>(shooter).range
                                  : 0;
            if (range <= 0 ||
                distanceSquared(*shooterPosition, *intent.target) >
                    range * range ||
                !hasLineOfSight(map, *shooterPosition, *intent.target)) {
                continue;
            }
            if (const auto target =
                    combatTargetAt(registry, *intent.target, shooter)) {
                queueDamage(registry, *target, intent.damage);
                resources.log.add(entityName(registry, shooter) +
                                  " breathes fire at " +
                                  entityName(registry, *target) + ".");
                fired = true;
            }
            continue;
        }

        const auto weapon =
            rangedWeaponForIntent(registry, shooter, intent.weapon);
        if (!weapon.has_value() ||
            distanceSquared(*shooterPosition, *intent.target) >
                weapon->range * weapon->range ||
            !map.isVisible(intent.target->x, intent.target->y) ||
            !hasLineOfSight(map, *shooterPosition, *intent.target)) {
            resources.log.add("No clear shot.");
            continue;
        }

        const auto target = combatTargetAt(registry, *intent.target, shooter);
        if (!target.has_value()) {
            resources.log.add("There is nothing to shoot there.");
            continue;
        }

        const int attack = rollDice(parseDice(weapon->damage), resources.rng) +
                           weapon->hitBonus +
                           attributeModifierWithEquipment(
                               registry, shooter, weapon->attribute);
        const int damage =
            std::max(0, attack - defenseWithEquipment(registry, *target));
        if (damage <= 0) {
            resources.log.add(entityName(registry, shooter) + " cannot hurt " +
                              entityName(registry, *target) + ".");
            fired = true;
            continue;
        }

        queueDamage(registry, *target, damage);
        resources.log.add(entityName(registry, shooter) + " shoots " +
                          entityName(registry, *target) + " for " +
                          std::to_string(damage) + ".");
        fired = true;
    }

    registry.clear<WantsToShoot>();
    return fired;
}

} // namespace rl
