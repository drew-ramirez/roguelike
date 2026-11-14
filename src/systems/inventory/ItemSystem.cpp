#include "systems/inventory/ItemSystem.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "core/GameLog.hpp"
#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "map/Map.hpp"
#include "systems/effects/EffectSystem.hpp"
#include "systems/inventory/EquipmentSystem.hpp"
#include "systems/spatial/SpatialIndexSystem.hpp"

namespace rl {
namespace {

void consumeIfNeeded(entt::registry& registry, entt::entity item)
{
    if (registry.valid(item) && registry.all_of<Consumable>(item)) {
        registry.destroy(item);
    }
}

void removeWorldOwnership(entt::registry& registry, entt::entity item)
{
    if (registry.all_of<Position>(item)) {
        registry.remove<Position>(item);
    }
}

void removeCarriedOwnership(entt::registry& registry, entt::entity item)
{
    if (registry.all_of<InBackpack>(item)) {
        registry.remove<InBackpack>(item);
    }
    if (registry.all_of<Equipped>(item)) {
        registry.remove<Equipped>(item);
    }
}

} // namespace

std::vector<entt::entity> inventoryItems(const entt::registry& registry,
                                         entt::entity owner)
{
    std::vector<entt::entity> items;

    const auto backpackView = registry.view<const Item, const InBackpack>();
    for (const auto item : backpackView) {
        if (backpackView.get<const InBackpack>(item).owner == owner) {
            items.push_back(item);
        }
    }

    const auto equippedView = registry.view<const Item, const Equipped>();
    for (const auto item : equippedView) {
        if (equippedView.get<const Equipped>(item).owner == owner) {
            items.push_back(item);
        }
    }

    std::sort(
        items.begin(), items.end(), [](entt::entity left, entt::entity right) {
            return entt::to_integral(left) < entt::to_integral(right);
        });
    return items;
}

bool queuePickupItemAtPlayer(entt::registry& registry, GameResources& resources)
{
    const auto playerPosition = entityPosition(registry, resources.player);
    if (!playerPosition.has_value()) {
        return false;
    }

    const auto item = itemAt(registry, *playerPosition);
    if (!item.has_value()) {
        resources.log.add("There is nothing here.");
        return false;
    }

    registry.emplace_or_replace<WantsToPickupItem>(resources.player,
                                                   WantsToPickupItem{*item});
    return true;
}

void processPickupItems(entt::registry& registry, GameResources& resources)
{
    const auto view = registry.view<const WantsToPickupItem>();
    for (const auto collector : view) {
        const auto& intent = view.get<const WantsToPickupItem>(collector);
        if (!registry.valid(intent.item) ||
            !registry.all_of<Item>(intent.item)) {
            continue;
        }

        removeWorldOwnership(registry, intent.item);
        removeCarriedOwnership(registry, intent.item);
        registry.emplace_or_replace<InBackpack>(intent.item,
                                                InBackpack{collector});
        resources.log.add("You pick up " + entityName(registry, intent.item) +
                          ".");
    }
    registry.clear<WantsToPickupItem>();
}

bool dropItemAtOwner(entt::registry& registry,
                     GameResources& resources,
                     entt::entity owner,
                     entt::entity item)
{
    const auto ownerPosition = entityPosition(registry, owner);
    if (!ownerPosition.has_value() || !itemOwnedBy(registry, owner, item)) {
        return false;
    }
    if (registry.all_of<Equipped>(item) && registry.all_of<CursedItem>(item)) {
        resources.log.add("You cannot drop cursed " +
                          entityName(registry, item) + ".");
        return false;
    }

    removeCarriedOwnership(registry, item);
    registry.emplace_or_replace<Position>(
        item, Position{ownerPosition->x, ownerPosition->y});
    resources.log.add("You drop " + entityName(registry, item) + ".");
    return true;
}

bool useInventoryItem(entt::registry& registry,
                      GameResources& resources,
                      Map& map,
                      entt::entity owner,
                      entt::entity item,
                      IFovProvider* fovProvider)
{
    if (!itemOwnedBy(registry, owner, item)) {
        return false;
    }

    if (registry.all_of<Ranged>(item)) {
        const auto ownerPosition = entityPosition(registry, owner);
        if (!ownerPosition.has_value()) {
            return false;
        }

        const auto& ranged = registry.get<Ranged>(item);
        std::optional<int> radius;
        if (registry.all_of<AreaOfEffect>(item)) {
            radius = registry.get<AreaOfEffect>(item).radius;
        }
        resources.pendingTargeting = PendingTargeting{TargetingMode::Item,
                                                      item,
                                                      {},
                                                      0,
                                                      ranged.range,
                                                      radius,
                                                      *ownerPosition};
        resources.log.add("Choose a target.");
        return false;
    }

    if (registry.all_of<Spellbook>(item)) {
        const auto spellId = registry.get<Spellbook>(item).spellId;
        auto& known = registry.all_of<KnownSpells>(owner)
                          ? registry.get<KnownSpells>(owner)
                          : registry.emplace<KnownSpells>(owner);
        if (std::find(known.ids.begin(), known.ids.end(), spellId) !=
            known.ids.end()) {
            resources.log.add("You already know that spell.");
            return false;
        }
        known.ids.push_back(spellId);
        resources.log.add("You learn " + spellId + ".");
        return true;
    }

    if (applyItemEffects(
            registry, resources, map, owner, item, std::nullopt, fovProvider)) {
        return true;
    }

    resources.log.add("Nothing happens.");
    return false;
}

std::vector<entt::entity> itemEffectTargets(const entt::registry& registry,
                                            entt::entity owner,
                                            entt::entity source)
{
    std::vector<entt::entity> targets;
    const bool identify =
        registry.valid(source) && registry.all_of<ProvidesIdentify>(source);
    const bool removeCurse =
        registry.valid(source) && registry.all_of<ProvidesRemoveCurse>(source);
    for (const auto item : inventoryItems(registry, owner)) {
        if (item == source) {
            continue;
        }
        if (identify && registry.all_of<MagicItem>(item) &&
            !registry.get<MagicItem>(item).identified) {
            targets.push_back(item);
        } else if (removeCurse && registry.all_of<CursedItem>(item)) {
            targets.push_back(item);
        }
    }
    return targets;
}

bool useInventoryItemOnTargetItem(entt::registry& registry,
                                  GameResources& resources,
                                  Map& map,
                                  entt::entity owner,
                                  entt::entity source,
                                  entt::entity target,
                                  IFovProvider* fovProvider)
{
    if (!itemOwnedBy(registry, owner, source) ||
        !itemOwnedBy(registry, owner, target)) {
        return false;
    }

    auto queue =
        EffectsQueue::forItem(registry, owner, source, std::nullopt, target);
    const bool resolved = queue.resolve(registry, resources, map, fovProvider);
    if (resolved) {
        consumeIfNeeded(registry, source);
    }
    return resolved;
}

bool processItemUses(entt::registry& registry,
                     GameResources& resources,
                     Map& map,
                     IFovProvider* fovProvider)
{
    bool used = false;
    const auto view = registry.view<const WantsToUseItem>();
    for (const auto user : view) {
        const auto& intent = view.get<const WantsToUseItem>(user);
        if (!registry.valid(intent.item)) {
            continue;
        }

        if (intent.target.has_value()) {
            used = applyItemEffects(registry,
                                    resources,
                                    map,
                                    user,
                                    intent.item,
                                    intent.target,
                                    fovProvider) ||
                   used;
            continue;
        }

        used = useInventoryItem(
                   registry, resources, map, user, intent.item, fovProvider) ||
               used;
    }
    registry.clear<WantsToUseItem>();
    return used;
}

bool isValidTarget(const entt::registry& registry,
                   const GameResources& resources,
                   const Map& map,
                   Point target)
{
    if (!resources.pendingTargeting.has_value() ||
        !map.inBounds(target.x, target.y) ||
        !map.isVisible(target.x, target.y)) {
        return false;
    }

    const auto playerPosition = entityPosition(registry, resources.player);
    if (!playerPosition.has_value()) {
        return false;
    }

    const int range = resources.pendingTargeting->range;
    return distanceSquared(*playerPosition, target) <= range * range;
}

bool confirmPendingTarget(entt::registry& registry,
                          GameResources& resources,
                          const Map& map)
{
    if (!resources.pendingTargeting.has_value()) {
        return false;
    }

    const auto pending = *resources.pendingTargeting;
    if (!isValidTarget(registry, resources, map, pending.cursor)) {
        resources.log.add("You cannot target there.");
        return false;
    }

    if (pending.mode == TargetingMode::Spell) {
        registry.emplace_or_replace<WantsToCastSpell>(
            resources.player,
            WantsToCastSpell{pending.spellId, pending.cursor});
    } else if (pending.mode == TargetingMode::Shoot) {
        registry.emplace_or_replace<WantsToShoot>(
            resources.player, WantsToShoot{pending.item, pending.cursor});
    } else {
        registry.emplace_or_replace<WantsToUseItem>(
            resources.player, WantsToUseItem{pending.item, pending.cursor});
    }
    resources.pendingTargeting.reset();
    return true;
}

void cancelPendingTarget(GameResources& resources)
{
    resources.pendingTargeting.reset();
    resources.log.add("Targeting cancelled.");
}

void moveTargetCursor(GameResources& resources, Point delta)
{
    if (!resources.pendingTargeting.has_value()) {
        return;
    }

    auto& cursor = resources.pendingTargeting->cursor;
    cursor = cursor + delta;
}

} // namespace rl
