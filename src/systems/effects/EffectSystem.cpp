/**
 * **************************************************************************
 * @file     EffectSystem.cpp
 * @brief    Resolves generic gameplay effects from items, spells, and powers
 * **************************************************************************
 */

#include "systems/effects/EffectSystem.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "systems/combat/DamageSystem.hpp"
#include "systems/rendering/ParticleSystem.hpp"
#include "systems/survival/HungerSystem.hpp"
#include "systems/travel/PortalSystem.hpp"

namespace rl {
namespace {

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
    resources.log.add(entityName(registry, actor) + " recovers " +
                      std::to_string(amount) + " health.");
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
    resources.log.add(entityName(registry, actor) + " eats.");
    return true;
}

[[nodiscard]] bool revealMap(GameResources& resources, Map& map)
{
    map.revealAll();
    resources.log.add("The map is revealed.");
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
            resources.log.add(entityName(registry, entity) + " is hit for " +
                              std::to_string(amount) + ".");
            ++affected;
        }
    }

    if (affected == 0) {
        resources.log.add("The magic fizzles.");
    }
    return true;
}

[[nodiscard]] bool identifyItem(entt::registry& registry,
                                GameResources& resources,
                                entt::entity actor,
                                entt::entity source,
                                entt::entity target)
{
    if (target != entt::null && registry.valid(target) && target != source &&
        itemOwnedBy(registry, actor, target) &&
        registry.all_of<MagicItem>(target)) {
        auto& magic = registry.get<MagicItem>(target);
        if (!magic.identified) {
            magic.identified = true;
            registry.emplace_or_replace<IdentifiedItem>(target);
            resources.log.add("You identify " + entityName(registry, target) +
                              ".");
            return true;
        }
    }

    const auto view = registry.view<MagicItem>();
    for (const auto item : view) {
        if (item == source) {
            continue;
        }
        auto& magic = view.get<MagicItem>(item);
        if (itemOwnedBy(registry, actor, item) && !magic.identified) {
            magic.identified = true;
            registry.emplace_or_replace<IdentifiedItem>(item);
            resources.log.add("You identify " + entityName(registry, item) +
                              ".");
            return true;
        }
    }

    resources.log.add("You have nothing unidentified.");
    return false;
}

[[nodiscard]] bool removeCurse(entt::registry& registry,
                               GameResources& resources,
                               entt::entity actor,
                               entt::entity target)
{
    if (target != entt::null && registry.valid(target) &&
        itemOwnedBy(registry, actor, target) &&
        registry.all_of<CursedItem>(target)) {
        registry.remove<CursedItem>(target);
        resources.log.add("A curse fades from " + entityName(registry, target) +
                          ".");
        return true;
    }

    const auto view = registry.view<const CursedItem>();
    for (const auto item : view) {
        if (itemOwnedBy(registry, actor, item)) {
            registry.remove<CursedItem>(item);
            resources.log.add("A curse fades from " +
                              entityName(registry, item) + ".");
            return true;
        }
    }

    resources.log.add("No curses respond.");
    return false;
}

[[nodiscard]] bool confuseTargets(entt::registry& registry,
                                  GameResources& resources,
                                  const Map& map,
                                  Point target,
                                  int turns)
{
    if (!map.inBounds(target.x, target.y) ||
        !map.isVisible(target.x, target.y)) {
        resources.log.add("You cannot target there.");
        return false;
    }

    bool confused = false;
    const auto view = registry.view<const Position, const CombatStats>();
    for (const auto entity : view) {
        const auto& position = view.get<const Position>(entity);
        if (position.x == target.x && position.y == target.y) {
            registry.emplace_or_replace<Confusion>(entity, Confusion{turns});
            resources.log.add(entityName(registry, entity) +
                              " looks confused.");
            confused = true;
        }
    }
    if (!confused) {
        resources.log.add("There is no mind there to confuse.");
    }
    return confused;
}

} // namespace

void EffectsQueue::enqueue(EffectRequest request)
{
    requests_.push_back(std::move(request));
}

EffectsQueue EffectsQueue::forRawEffects(const RawEffects& effects,
                                         entt::entity actor,
                                         entt::entity source,
                                         std::optional<Point> target)
{
    EffectsQueue queue;
    auto& requests = queue.requests_;
    if (effects.providesHealing.has_value()) {
        requests.push_back(EffectRequest{
            EffectKind::Heal, actor, source, target, *effects.providesHealing});
    }
    if (effects.providesNutrition.has_value()) {
        requests.push_back(EffectRequest{EffectKind::Nutrition,
                                         actor,
                                         source,
                                         target,
                                         *effects.providesNutrition});
    }
    if (effects.magicMapper) {
        requests.push_back(
            EffectRequest{EffectKind::RevealMap, actor, source, target});
    }
    if (effects.inflictsDamage.has_value()) {
        requests.push_back(EffectRequest{EffectKind::Damage,
                                         actor,
                                         source,
                                         target,
                                         *effects.inflictsDamage,
                                         effects.areaOfEffect.value_or(0)});
    }
    if (effects.identify) {
        requests.push_back(
            EffectRequest{EffectKind::Identify, actor, source, target});
    }
    if (effects.removeCurse) {
        requests.push_back(
            EffectRequest{EffectKind::RemoveCurse, actor, source, target});
    }
    if (effects.townPortal) {
        requests.push_back(
            EffectRequest{EffectKind::TownPortal, actor, source, target});
    }
    if (effects.confusionTurns.has_value()) {
        requests.push_back(EffectRequest{EffectKind::Confusion,
                                         actor,
                                         source,
                                         target,
                                         *effects.confusionTurns});
    }
    return queue;
}

EffectsQueue EffectsQueue::forItem(const entt::registry& registry,
                                   entt::entity actor,
                                   entt::entity item,
                                   std::optional<Point> target,
                                   entt::entity targetEntity)
{
    EffectsQueue queue;
    auto& requests = queue.requests_;
    if (!registry.valid(item)) {
        return queue;
    }
    if (registry.all_of<ProvidesHealing>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::Heal,
                          actor,
                          item,
                          target,
                          registry.get<ProvidesHealing>(item).amount});
    }
    if (registry.all_of<ProvidesNutrition>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::Nutrition,
                          actor,
                          item,
                          target,
                          registry.get<ProvidesNutrition>(item).amount});
    }
    if (registry.all_of<MagicMapper>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::RevealMap, actor, item, target});
    }
    if (registry.all_of<InflictsDamage>(item)) {
        const int radius = registry.all_of<AreaOfEffect>(item)
                               ? registry.get<AreaOfEffect>(item).radius
                               : 0;
        requests.push_back(
            EffectRequest{EffectKind::Damage,
                          actor,
                          item,
                          target,
                          registry.get<InflictsDamage>(item).amount,
                          radius});
    }
    if (registry.all_of<ProvidesIdentify>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::Identify, actor, item, target});
    }
    if (registry.all_of<ProvidesRemoveCurse>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::RemoveCurse, actor, item, target});
    }
    if (registry.all_of<TownPortal>(item) && registry.all_of<Item>(item)) {
        requests.push_back(
            EffectRequest{EffectKind::TownPortal, actor, item, target});
    }
    if (registry.all_of<Confusion>(item) && registry.all_of<Item>(item)) {
        requests.push_back(EffectRequest{EffectKind::Confusion,
                                         actor,
                                         item,
                                         target,
                                         registry.get<Confusion>(item).turns});
    }
    for (auto& request : requests) {
        request.targetEntity = targetEntity;
    }
    return queue;
}

bool EffectsQueue::resolve(entt::registry& registry,
                           GameResources& resources,
                           Map& map,
                           IFovProvider* fovProvider)
{
    bool anyResolved = false;
    for (const auto& request : requests_) {
        bool resolved = false;
        switch (request.kind) {
        case EffectKind::Heal:
            resolved = heal(registry, resources, request.actor, request.amount);
            break;
        case EffectKind::Nutrition:
            resolved = feed(registry, resources, request.actor, request.amount);
            break;
        case EffectKind::Damage:
            if (request.target.has_value()) {
                resolved = damageTargets(registry,
                                         resources,
                                         map,
                                         request.actor,
                                         *request.target,
                                         request.amount,
                                         request.radius);
            }
            break;
        case EffectKind::RevealMap:
            resolved = revealMap(resources, map);
            break;
        case EffectKind::Identify:
            resolved = identifyItem(registry,
                                    resources,
                                    request.actor,
                                    request.source,
                                    request.targetEntity);
            break;
        case EffectKind::RemoveCurse:
            resolved = removeCurse(
                registry, resources, request.actor, request.targetEntity);
            break;
        case EffectKind::TownPortal:
            if (fovProvider != nullptr) {
                resolved = tryUseTownPortal(
                    registry, resources, map, *fovProvider, request.actor);
            } else {
                resources.log.add("The portal magic fizzles.");
            }
            break;
        case EffectKind::Confusion:
            if (request.target.has_value()) {
                resolved = confuseTargets(
                    registry, resources, map, *request.target, request.amount);
            }
            break;
        }
        anyResolved = resolved || anyResolved;
    }
    requests_.clear();
    return anyResolved;
}

bool applyItemEffects(entt::registry& registry,
                      GameResources& resources,
                      Map& map,
                      entt::entity actor,
                      entt::entity item,
                      std::optional<Point> target,
                      IFovProvider* fovProvider)
{
    auto queue = EffectsQueue::forItem(registry, actor, item, target);
    const bool resolved = queue.resolve(registry, resources, map, fovProvider);
    if (resolved) {
        consumeIfNeeded(registry, item);
    }
    return resolved;
}

} // namespace rl
