/**
 * **************************************************************************
 * @file     EffectSystem.cpp
 * @brief    Resolves generic gameplay effects from items
 * **************************************************************************
 */

#include "systems/EffectSystem.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/ParticleSystem.hpp"

namespace rl {
namespace {

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

void consumeIfNeeded(entt::registry& registry, entt::entity item)
{
    if (registry.valid(item) && registry.all_of<Consumable>(item)) {
        registry.destroy(item);
    }
}

[[nodiscard]] bool heal(entt::registry& registry,
                        GameResources& resources,
                        entt::entity actor,
                        int amount)
{
    if (!registry.valid(actor) || !registry.all_of<CombatStats>(actor)) {
        return false;
    }

    auto& stats = registry.get<CombatStats>(actor);
    if (stats.hp >= stats.maxHp) {
        resources.log.add("You are already at full health.");
        return false;
    }

    stats.hp = std::min(stats.maxHp, stats.hp + amount);
    if (registry.all_of<Pools>(actor)) {
        auto& pools = registry.get<Pools>(actor);
        pools.maxHitPoints = stats.maxHp;
        pools.hitPoints = stats.hp;
    }
    if (const auto position = entityPosition(registry, actor);
        position.has_value()) {
        requestHealingParticle(resources, *position);
    }
    resources.log.add("You recover " + std::to_string(amount) + " health.");
    return true;
}

[[nodiscard]] bool feed(entt::registry& registry,
                        GameResources& resources,
                        entt::entity actor,
                        int amount)
{
    if (!registry.valid(actor) || !registry.all_of<HungerClock>(actor)) {
        return false;
    }

    constexpr int maximumHunger = 1000;
    auto& clock = registry.get<HungerClock>(actor);
    clock.duration = std::min(maximumHunger, clock.duration + amount);
    clock.state = hungerStateForDuration(clock.duration);
    resources.log.add("You eat.");
    return true;
}

[[nodiscard]] bool damageTargets(entt::registry& registry,
                                 GameResources& resources,
                                 const Map& map,
                                 entt::entity actor,
                                 Point target,
                                 int amount,
                                 int radius)
{
    if (!map.inBounds(target.x, target.y) ||
        !map.isVisible(target.x, target.y)) {
        resources.log.add("You cannot target there.");
        return false;
    }

    int affected = 0;
    const auto view = registry.view<const Position, const CombatStats>();
    for (const auto entity : view) {
        if (entity == actor) {
            continue;
        }
        const auto& position = view.get<const Position>(entity);
        const Point point{position.x, position.y};
        const bool hit = radius > 0
                             ? distanceSquared(point, target) <= radius * radius
                             : point == target;
        if (hit && map.isVisible(point.x, point.y)) {
            queueDamage(registry, entity, amount);
            ++affected;
        }
    }

    if (affected == 0) {
        resources.log.add("The magic fizzles.");
    }
    return true;
}

[[nodiscard]] bool resolveRequest(entt::registry& registry,
                                  GameResources& resources,
                                  Map& map,
                                  const EffectRequest& request)
{
    switch (request.kind) {
    case EffectKind::Heal:
        return heal(registry, resources, request.actor, request.amount);
    case EffectKind::Nutrition:
        return feed(registry, resources, request.actor, request.amount);
    case EffectKind::Damage:
        return request.target.has_value() &&
               damageTargets(registry,
                             resources,
                             map,
                             request.actor,
                             *request.target,
                             request.amount,
                             request.radius);
    case EffectKind::RevealMap:
        map.revealAll();
        resources.log.add("The map is revealed.");
        return true;
    }
    return false;
}

} // namespace

EffectsQueue::EffectsQueue(std::vector<EffectRequest> requests)
    : requests_(std::move(requests))
{
}

void EffectsQueue::push(EffectRequest request)
{
    requests_.push_back(std::move(request));
}


bool EffectsQueue::resolve(entt::registry& registry,
                           GameResources& resources,
                           Map& map)
{
    bool anyResolved = false;
    for (const auto& request : requests_) {
        anyResolved = resolveRequest(registry, resources, map, request) ||
                      anyResolved;
    }
    requests_.clear();
    return anyResolved;
}

std::vector<EffectRequest> effectRequestsForItem(const entt::registry& registry,
                                                 entt::entity actor,
                                                 entt::entity item,
                                                 std::optional<Point> target)
{
    std::vector<EffectRequest> requests;
    if (!registry.valid(item)) {
        return requests;
    }
    if (registry.all_of<ProvidesHealing>(item)) {
        requests.push_back({EffectKind::Heal,
                            actor,
                            target,
                            registry.get<ProvidesHealing>(item).amount});
    }
    if (registry.all_of<ProvidesNutrition>(item)) {
        requests.push_back({EffectKind::Nutrition,
                            actor,
                            target,
                            registry.get<ProvidesNutrition>(item).amount});
    }
    if (registry.all_of<MagicMapper>(item)) {
        requests.push_back({EffectKind::RevealMap, actor, target});
    }
    if (registry.all_of<InflictsDamage>(item)) {
        const int radius = registry.all_of<AreaOfEffect>(item)
                               ? registry.get<AreaOfEffect>(item).radius
                               : 0;
        requests.push_back({EffectKind::Damage,
                            actor,
                            target,
                            registry.get<InflictsDamage>(item).amount,
                            radius});
    }
    return requests;
}

bool applyItemEffects(entt::registry& registry,
                      GameResources& resources,
                      Map& map,
                      entt::entity actor,
                      entt::entity item,
                      std::optional<Point> target)
{
    EffectsQueue queue{effectRequestsForItem(registry, actor, item, target)};
    const bool resolved = queue.resolve(registry, resources, map);
    if (resolved) {
        consumeIfNeeded(registry, item);
    }
    return resolved;
}

} // namespace rl
