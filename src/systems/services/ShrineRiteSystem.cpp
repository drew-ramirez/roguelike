/**
 * **************************************************************************
 * @file     ShrineRiteSystem.cpp
 * @brief    Resolves shrine offering interactions
 * **************************************************************************
 */

#include "systems/services/ShrineRiteSystem.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "systems/inventory/ItemSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

[[nodiscard]] bool hasAnyTag(const Ingredient& ingredient,
                             const std::vector<std::string>& tags)
{
    return std::any_of(tags.begin(), tags.end(), [&](const std::string& tag) {
        return std::find(ingredient.tags.begin(), ingredient.tags.end(), tag) !=
               ingredient.tags.end();
    });
}

[[nodiscard]] entt::entity findOffering(const entt::registry& registry,
                                        entt::entity owner,
                                        const Shrine& shrine)
{
    for (const auto item : inventoryItems(registry, owner)) {
        if (registry.all_of<Ingredient>(item) &&
            hasAnyTag(registry.get<Ingredient>(item),
                      shrine.acceptedOfferingTags)) {
            return item;
        }
    }
    return entt::null;
}

void applyBlessing(entt::registry& registry,
                   GameResources& resources,
                   const Shrine& shrine)
{
    if (shrine.blessingHp <= 0 || resources.player == entt::null ||
        !registry.valid(resources.player) ||
        !registry.all_of<CombatStats>(resources.player)) {
        return;
    }

    auto& stats = registry.get<CombatStats>(resources.player);
    stats.hp = std::min(stats.maxHp, stats.hp + shrine.blessingHp);
    if (registry.all_of<Pools>(resources.player)) {
        auto& pools = registry.get<Pools>(resources.player);
        pools.hitPoints = stats.hp;
        pools.maxHitPoints = stats.maxHp;
    }
}

[[nodiscard]] bool localScreenSupportsShrine(const GameResources& resources)
{
    if (!resources.world.active || resources.world.currentDangerEncounter) {
        return false;
    }

    const auto* region = currentWorldRegion(resources.world);
    if (region == nullptr || !region->hasShrine ||
        (!region->discovered && !region->visited)) {
        return false;
    }

    const auto screen = describeLocalScreen(resources.world,
                                            resources.world.currentMacro,
                                            resources.world.currentScreen,
                                            false);
    return screen.role == LocalScreenRole::CenterLandmark ||
           screen.role == LocalScreenRole::CampShrine;
}

} // namespace

bool shrineAvailable(const GameResources& resources,
                     const entt::registry& registry,
                     entt::entity shrineEntity)
{
    return registry.valid(shrineEntity) &&
           registry.all_of<Shrine>(shrineEntity) &&
           localScreenSupportsShrine(resources);
}

bool offerAtShrine(entt::registry& registry,
                   GameResources& resources,
                   entt::entity shrineEntity)
{
    if (!shrineAvailable(resources, registry, shrineEntity)) {
        resources.log.add("No shrine answers here.");
        return false;
    }

    const auto& shrine = registry.get<Shrine>(shrineEntity);
    const auto offering = findOffering(registry, resources.player, shrine);
    if (offering == entt::null) {
        resources.log.add("You have no fitting offering.");
        return false;
    }

    const std::string name = registry.all_of<Name>(offering)
                                 ? registry.get<Name>(offering).value
                                 : std::string{"offering"};
    registry.destroy(offering);
    applyBlessing(registry, resources, shrine);
    resources.log.add(LogCategory::Progression,
                      "You offer " + name + " at the shrine.");
    return true;
}

} // namespace rl
