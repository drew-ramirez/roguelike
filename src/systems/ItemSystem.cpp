#include "systems/ItemSystem.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
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

void consumeIfNeeded(entt::registry& registry, entt::entity item)
{
    if (registry.valid(item) && registry.all_of<Consumable>(item)) {
        registry.destroy(item);
    }
}

[[nodiscard]] bool healOwner(entt::registry& registry,
                             GameResources& resources,
                             entt::entity owner,
                             entt::entity item)
{
    if (!registry.all_of<ProvidesHealing>(item) ||
        !registry.all_of<CombatStats>(owner)) {
        return false;
    }

    auto& stats = registry.get<CombatStats>(owner);
    if (stats.hp >= stats.maxHp) {
        resources.log.add("You are already at full health.");
        return false;
    }

    const int amount = registry.get<ProvidesHealing>(item).amount;
    stats.hp = std::min(stats.maxHp, stats.hp + amount);
    resources.log.add("You drink " + entityName(registry, item) + ".");
    consumeIfNeeded(registry, item);
    return true;
}

[[nodiscard]] bool itemOwnedBy(const entt::registry& registry,
                               entt::entity owner,
                               entt::entity item)
{
    if (!registry.valid(item)) {
        return false;
    }
    if (registry.all_of<InBackpack>(item) &&
        registry.get<InBackpack>(item).owner == owner) {
        return true;
    }
    return registry.all_of<Equipped>(item) &&
           registry.get<Equipped>(item).owner == owner;
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

std::optional<entt::entity> itemAtPosition(const entt::registry& registry,
                                           Point position)
{
    const auto view = registry.view<const Item, const Position>();
    for (const auto item : view) {
        const auto& itemPosition = view.get<const Position>(item);
        if (itemPosition.x == position.x && itemPosition.y == position.y) {
            return item;
        }
    }
    return std::nullopt;
}

bool queuePickupItemAtPlayer(entt::registry& registry, GameResources& resources)
{
    const auto playerPosition = entityPosition(registry, resources.player);
    if (!playerPosition.has_value()) {
        return false;
    }

    const auto item = itemAtPosition(registry, *playerPosition);
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

    removeCarriedOwnership(registry, item);
    registry.emplace_or_replace<Position>(
        item, Position{ownerPosition->x, ownerPosition->y});
    resources.log.add("You drop " + entityName(registry, item) + ".");
    return true;
}

bool useInventoryItem(entt::registry& registry,
                      GameResources& resources,
                      const Map& map,
                      entt::entity owner,
                      entt::entity item)
{
    if (!itemOwnedBy(registry, owner, item)) {
        return false;
    }

    if (registry.all_of<ProvidesHealing>(item)) {
        return healOwner(registry, resources, owner, item);
    }

    resources.log.add("Nothing happens.");
    (void)map;
    return false;
}

} // namespace rl
