#include "systems/EquipmentSystem.hpp"

#include <string>

#include "core/GameLog.hpp"
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

} // namespace

bool equipOrUnequipItem(entt::registry& registry,
                        GameLog& log,
                        entt::entity owner,
                        entt::entity item)
{
    if (owner == entt::null || item == entt::null || !registry.valid(owner) ||
        !registry.valid(item)) {
        return false;
    }

    if (registry.all_of<Equipped>(item)) {
        const auto equipped = registry.get<Equipped>(item);
        if (equipped.owner != owner) {
            return false;
        }

        registry.remove<Equipped>(item);
        registry.emplace_or_replace<InBackpack>(item, InBackpack{owner});
        log.add("You remove " + entityName(registry, item) + ".");
        return true;
    }

    if (!registry.all_of<Equippable>(item) ||
        !registry.all_of<InBackpack>(item) ||
        registry.get<InBackpack>(item).owner != owner) {
        log.add("You cannot equip " + entityName(registry, item) + ".");
        return false;
    }

    const auto slot = registry.get<Equippable>(item).slot;
    const auto equippedView = registry.view<Equipped>();
    for (const auto other : equippedView) {
        const auto& equipped = equippedView.get<Equipped>(other);
        if (equipped.owner == owner && equipped.slot == slot) {
            registry.remove<Equipped>(other);
            registry.emplace_or_replace<InBackpack>(other, InBackpack{owner});
            log.add("You remove " + entityName(registry, other) + ".");
        }
    }

    registry.remove<InBackpack>(item);
    registry.emplace_or_replace<Equipped>(item, Equipped{owner, slot});
    log.add("You equip " + entityName(registry, item) + ".");
    return true;
}

} // namespace rl
