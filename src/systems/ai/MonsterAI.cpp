/**
 * **************************************************************************
 * @file     MonsterAI.cpp
 * @brief    Moves visible hostile monsters and queues adjacent melee attacks
 * **************************************************************************
 */

#include "systems/ai/MonsterAI.hpp"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ai/BystanderAI.hpp"
#include "systems/spatial/SpatialIndexSystem.hpp"

namespace rl {
namespace {

int sign(int value) noexcept
{
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

Point chooseMonsterStep(Point monsterPosition,
                        Point playerPosition,
                        const Map& map,
                        const entt::registry& registry,
                        entt::entity monster)
{
    const int deltaX = playerPosition.x - monsterPosition.x;
    const int deltaY = playerPosition.y - monsterPosition.y;
    const Point primary = std::abs(deltaX) >= std::abs(deltaY)
                              ? Point{sign(deltaX), 0}
                              : Point{0, sign(deltaY)};
    const Point secondary =
        primary.x != 0 ? Point{0, sign(deltaY)} : Point{sign(deltaX), 0};

    // Try the longer axis first, then slide along the other axis if blocked
    const Point primaryTarget = monsterPosition + primary;
    if ((primary.x != 0 || primary.y != 0) &&
        !isBlocked(map, registry, primaryTarget, monster)) {
        return primary;
    }

    const Point secondaryTarget = monsterPosition + secondary;
    if ((secondary.x != 0 || secondary.y != 0) &&
        !isBlocked(map, registry, secondaryTarget, monster)) {
        return secondary;
    }

    return Point{};
}

[[nodiscard]] bool isHostileAiActor(const entt::registry& registry,
                                    entt::entity entity)
{
    return !registry.all_of<Neutral>(entity) &&
           !registry.all_of<Bystander>(entity) &&
           !registry.all_of<Vendor>(entity);
}

[[nodiscard]] int manhattanDistance(Point left, Point right) noexcept
{
    return std::abs(left.x - right.x) + std::abs(left.y - right.y);
}

[[nodiscard]] bool factionHostileTo(std::string_view factionId,
                                    std::string_view targetId)
{
    const auto* faction = defaultRawDatabase().faction(factionId);
    if (faction == nullptr) {
        return false;
    }
    return std::find(faction->hostileTo.begin(),
                     faction->hostileTo.end(),
                     targetId) != faction->hostileTo.end();
}

[[nodiscard]] bool isHostileTo(const entt::registry& registry,
                               entt::entity actor,
                               entt::entity target)
{
    if (target == entt::null || !registry.valid(target) ||
        registry.all_of<Neutral>(target) || registry.all_of<Prop>(target)) {
        return false;
    }
    if (registry.all_of<Player>(target)) {
        if (!registry.all_of<Faction>(actor)) {
            return true;
        }
        return factionHostileTo(registry.get<Faction>(actor).id, "player");
    }
    if (!registry.all_of<Faction>(actor) || !registry.all_of<Faction>(target)) {
        return false;
    }
    return factionHostileTo(registry.get<Faction>(actor).id,
                            registry.get<Faction>(target).id);
}

[[nodiscard]] std::optional<entt::entity> nearestHostileTarget(
    const entt::registry& registry, entt::entity actor, entt::entity player)
{
    const auto actorPosition = entityPosition(registry, actor);
    if (!actorPosition.has_value()) {
        return std::nullopt;
    }

    std::optional<entt::entity> best;
    int bestDistance = 0;
    const auto consider = [&](entt::entity target) {
        const auto targetPosition = entityPosition(registry, target);
        if (!targetPosition.has_value() ||
            !isHostileTo(registry, actor, target)) {
            return;
        }
        const int distance = manhattanDistance(*actorPosition, *targetPosition);
        if (!best.has_value() || distance < bestDistance) {
            best = target;
            bestDistance = distance;
        }
    };

    consider(player);
    const auto view = registry.view<const Monster, const Position>();
    for (const auto target : view) {
        if (target != actor) {
            consider(target);
        }
    }
    return best;
}

[[nodiscard]] std::optional<entt::entity>
equippedRangedWeapon(const entt::registry& registry, entt::entity owner)
{
    const auto view = registry.view<const Equipped, const RangedWeapon>();
    for (const auto item : view) {
        const auto& equipped = view.get<const Equipped>(item);
        if (equipped.owner == owner && equipped.slot == EquipmentSlot::Weapon) {
            return item;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RangedWeapon> rangedWeaponStats(
    const entt::registry& registry, entt::entity actor, entt::entity weapon)
{
    if (registry.valid(weapon) && registry.all_of<RangedWeapon>(weapon)) {
        return registry.get<RangedWeapon>(weapon);
    }
    if (registry.valid(actor) && registry.all_of<RangedWeapon>(actor)) {
        return registry.get<RangedWeapon>(actor);
    }
    return std::nullopt;
}

} // namespace

void runHostileMonsterAi(entt::registry& registry,
                         GameResources& resources,
                         const Map& map)
{
    if (resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Position>(resources.player)) {
        return;
    }

    const auto view = registry.view<const Monster, Position>();
    for (const auto monster : view) {
        if (!isHostileAiActor(registry, monster)) {
            continue;
        }

        if (registry.all_of<Confusion>(monster)) {
            auto& confusion = registry.get<Confusion>(monster);
            --confusion.turns;
            if (confusion.turns <= 0) {
                registry.remove<Confusion>(monster);
            }
            continue;
        }

        auto& monsterPositionComponent = view.get<Position>(monster);
        const Point monsterPosition{monsterPositionComponent.x,
                                    monsterPositionComponent.y};
        const auto target =
            nearestHostileTarget(registry, monster, resources.player);
        if (!target.has_value()) {
            continue;
        }
        const auto targetPosition = entityPosition(registry, *target);
        if (!targetPosition.has_value()) {
            continue;
        }

        const int distance =
            manhattanDistance(monsterPosition, *targetPosition);

        // Adjacent monsters attack, visible ones chase with a greedy step
        if (distance == 1) {
            registry.emplace_or_replace<WantsToMelee>(monster,
                                                      WantsToMelee{*target});
            continue;
        }

        if (!map.isVisible(monsterPosition.x, monsterPosition.y)) {
            continue;
        }

        if (registry.all_of<BreathWeapon>(monster)) {
            auto& breath = registry.get<BreathWeapon>(monster);
            if (breath.cooldown > 0) {
                --breath.cooldown;
            } else if (distanceSquared(monsterPosition, *targetPosition) <=
                       breath.range * breath.range) {
                registry.emplace_or_replace<WantsToShoot>(
                    monster,
                    WantsToShoot{
                        monster, *targetPosition, true, breath.damage});
                breath.cooldown = 3;
                continue;
            }
        }

        const auto equipped = equippedRangedWeapon(registry, monster);
        const auto weapon = equipped.value_or(monster);
        const auto ranged = rangedWeaponStats(registry, monster, weapon);
        if (ranged.has_value() &&
            distanceSquared(monsterPosition, *targetPosition) <=
                ranged->range * ranged->range) {
            registry.emplace_or_replace<WantsToShoot>(
                monster, WantsToShoot{weapon, *targetPosition});
            continue;
        }

        const Point step = chooseMonsterStep(
            monsterPosition, *targetPosition, map, registry, monster);
        if (step.x != 0 || step.y != 0) {
            [[maybe_unused]] const bool moved =
                tryMoveEntity(registry, monster, map, step);
        }
    }
}

void runMonsterAi(entt::registry& registry,
                  GameResources& resources,
                  const Map& map)
{
    runBystanderAi(registry, resources, map);
    runHostileMonsterAi(registry, resources, map);
}

} // namespace rl
