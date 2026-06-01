/**
 * **************************************************************************
 * @file     RangedCombatSystem.cpp
 * @brief    Resolves line-of-sight ranged weapon attacks
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
#include "map/Map.hpp"
#include "map/MapGeometry.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::string entityName(const entt::registry& registry,
                                     entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<Name>(entity)) {
        return registry.get<Name>(entity).value;
    }
    return "Something";
}

[[nodiscard]] std::optional<Point>
entityPosition(const entt::registry& registry, entt::entity entity)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return std::nullopt;
    }
    const auto& position = registry.get<Position>(entity);
    return Point{position.x, position.y};
}

[[nodiscard]] int distanceSquared(Point left, Point right) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    return (dx * dx) + (dy * dy);
}

[[nodiscard]] int attributeValue(const Attributes& attributes,
                                 std::string_view name)
{
    if (name == "fitness") {
        return attributes.fitness;
    }
    if (name == "quickness") {
        return attributes.quickness;
    }
    if (name == "intelligence") {
        return attributes.intelligence;
    }
    return attributes.might;
}

[[nodiscard]] int attributeModifier(int score)
{
    const int delta = score - 10;
    return delta >= 0 ? delta / 2 : -((1 - delta) / 2);
}

[[nodiscard]] int attackerModifier(const entt::registry& registry,
                                   entt::entity attacker,
                                   std::string_view attribute)
{
    if (!registry.valid(attacker) || !registry.all_of<Attributes>(attacker)) {
        return 0;
    }
    return attributeModifier(
        attributeValue(attributesWithEquipment(registry, attacker), attribute));
}

[[nodiscard]] bool hasLineOfSight(const Map& map, Point from, Point to)
{
    const auto line = bresenhamLine(from, to, LineStartPolicy::ExcludeStart);
    for (const auto point : line) {
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
        if (!shooterPosition.has_value() || !intent.target.has_value() ||
            !registry.valid(intent.weapon) ||
            !registry.all_of<RangedWeapon>(intent.weapon)) {
            continue;
        }

        const auto& weapon = registry.get<RangedWeapon>(intent.weapon);
        if (distanceSquared(*shooterPosition, *intent.target) >
                weapon.range * weapon.range ||
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

        const int attack = rollDice(parseDice(weapon.damage), resources.rng) +
                           weapon.hitBonus +
                           attackerModifier(registry, shooter, weapon.attribute);
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
