#include "systems/ItemSystem.hpp"

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
}

} // namespace

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

} // namespace rl
