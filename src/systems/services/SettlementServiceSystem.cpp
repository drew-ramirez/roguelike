/**
 * **************************************************************************
 * @file     SettlementServiceSystem.cpp
 * @brief    Resolves landmark settlement service actions
 * **************************************************************************
 */

#include "systems/services/SettlementServiceSystem.hpp"

#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "systems/survival/HungerSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

constexpr int teachRecipeHungerCost = 5;

[[nodiscard]] bool localScreenSupportsService(const GameResources& resources)
{
    if (!resources.world.active || resources.world.currentDangerEncounter) {
        return false;
    }

    const auto* region = currentWorldRegion(resources.world);
    if (region == nullptr || (!region->discovered && !region->visited)) {
        return false;
    }
    if (region->hasSettlement) {
        return true;
    }

    const auto screen = describeLocalScreen(resources.world,
                                            resources.world.currentMacro,
                                            resources.world.currentScreen,
                                            false);
    return screen.role == LocalScreenRole::CenterLandmark ||
           screen.role == LocalScreenRole::TownOutskirts ||
           screen.role == LocalScreenRole::CampShrine;
}

} // namespace

bool settlementServiceAvailable(const GameResources& resources,
                                const entt::registry& registry,
                                entt::entity provider)
{
    return registry.valid(provider) &&
           registry.all_of<SettlementService>(provider) &&
           localScreenSupportsService(resources);
}

bool useSettlementService(entt::registry& registry,
                          GameResources& resources,
                          entt::entity provider)
{
    if (!settlementServiceAvailable(resources, registry, provider)) {
        resources.log.add("No settlement service is available here.");
        return false;
    }

    const auto& service = registry.get<SettlementService>(provider);
    if (service.serviceId != "teach_recipe") {
        resources.log.add("This service is not ready yet.");
        return false;
    }

    const auto* recipe = defaultRawDatabase().recipe(service.recipeId);
    if (recipe == nullptr) {
        resources.log.add("The lesson names an unknown recipe.");
        return false;
    }

    consumeHunger(registry, resources, teachRecipeHungerCost);
    resources.log.add(LogCategory::Progression,
                      "You learn " + recipe->name + ".");
    return true;
}

} // namespace rl
