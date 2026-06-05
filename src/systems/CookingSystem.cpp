/**
 * **************************************************************************
 * @file     CookingSystem.cpp
 * @brief    Resolves camp cooking from raw recipes and inventory ingredients
 * **************************************************************************
 */

#include "systems/CookingSystem.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "systems/ItemSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

[[nodiscard]] bool containsString(const std::vector<std::string>& values,
                                  const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

[[nodiscard]] std::vector<std::string>
placeTagsForCamp(const entt::registry& registry,
                 const GameResources& resources,
                 entt::entity camp)
{
    std::vector<std::string> tags;
    if (const auto* region = currentWorldRegion(resources.world);
        region != nullptr) {
        tags.push_back(std::string{biomeVariantId(region->biome)});
    }
    if (registry.valid(camp) && registry.all_of<CampSite>(camp)) {
        tags.push_back(registry.get<CampSite>(camp).campType);
    }
    if (registry.valid(camp) && registry.all_of<CookingStation>(camp)) {
        const auto& station = registry.get<CookingStation>(camp);
        tags.insert(
            tags.end(), station.placeTags.begin(), station.placeTags.end());
    }
    return tags;
}

[[nodiscard]] bool recipePlaceMatches(const RawRecipe& recipe,
                                      const std::vector<std::string>& placeTags)
{
    if (recipe.placeTags.empty()) {
        return true;
    }
    return std::any_of(
        recipe.placeTags.begin(),
        recipe.placeTags.end(),
        [&](const std::string& tag) { return containsString(placeTags, tag); });
}

[[nodiscard]] bool recipeMethodMatches(const entt::registry& registry,
                                       entt::entity camp,
                                       const RawRecipe& recipe)
{
    if (recipe.method.empty()) {
        return true;
    }
    if (!registry.valid(camp) || !registry.all_of<CookingStation>(camp)) {
        return false;
    }

    const auto& station = registry.get<CookingStation>(camp);
    return containsString(station.methods, recipe.method);
}

[[nodiscard]] bool recipeStationMatches(const entt::registry& registry,
                                        entt::entity camp,
                                        const RawRecipe& recipe)
{
    if (recipe.station.empty()) {
        return true;
    }
    if (!registry.valid(camp) || !registry.all_of<CookingStation>(camp)) {
        return false;
    }

    const auto& station = registry.get<CookingStation>(camp);
    return recipe.station == station.stationId ||
           containsString(station.placeTags, recipe.station);
}

[[nodiscard]] std::vector<entt::entity> matchingIngredientsForRecipe(
    const entt::registry& registry, entt::entity owner, const RawRecipe& recipe)
{
    const auto ingredients = carriedIngredients(registry, owner);
    std::vector<entt::entity> selected;

    for (const auto& amount : recipe.ingredients) {
        int foundCount = 0;
        for (const auto item : ingredients) {
            if (std::find(selected.begin(), selected.end(), item) !=
                selected.end()) {
                continue;
            }
            if (registry.all_of<RawIdentity>(item) &&
                registry.get<RawIdentity>(item).templateName == amount.id) {
                selected.push_back(item);
                ++foundCount;
                if (foundCount == amount.count) {
                    break;
                }
            }
        }
        if (foundCount != amount.count) {
            return {};
        }
    }

    return selected;
}

[[nodiscard]] bool createRecipeOutputs(entt::registry& registry,
                                       entt::entity owner,
                                       const RawRecipe& recipe)
{
    bool created = false;
    for (const auto& output : recipe.outputs) {
        for (int index = 0; index < output.count; ++index) {
            const auto item = spawnRawEntity(
                registry, defaultRawDatabase(), output.id, Point{0, 0});
            if (!item.has_value()) {
                continue;
            }
            if (registry.all_of<Position>(*item)) {
                registry.remove<Position>(*item);
            }
            registry.emplace_or_replace<InBackpack>(*item, InBackpack{owner});
            registry.emplace_or_replace<Meal>(*item, Meal{recipe.id});
            if (registry.all_of<ProvidesNutrition>(*item)) {
                registry.get<ProvidesNutrition>(*item).amount =
                    recipe.nutrition;
            }
            created = true;
        }
    }
    return created;
}

} // namespace

std::vector<entt::entity> carriedIngredients(const entt::registry& registry,
                                             entt::entity owner)
{
    std::vector<entt::entity> result;
    for (const auto item : inventoryItems(registry, owner)) {
        if (registry.all_of<Ingredient>(item)) {
            result.push_back(item);
        }
    }
    return result;
}

std::optional<const RawRecipe*>
matchingRecipeForCamp(const entt::registry& registry,
                      const GameResources& resources,
                      entt::entity camp)
{
    const auto placeTags = placeTagsForCamp(registry, resources, camp);
    for (const auto* recipe : defaultRawDatabase().recipes()) {
        if (recipe == nullptr ||
            !recipeStationMatches(registry, camp, *recipe) ||
            !recipeMethodMatches(registry, camp, *recipe) ||
            !recipePlaceMatches(*recipe, placeTags)) {
            continue;
        }
        if (!matchingIngredientsForRecipe(registry, resources.player, *recipe)
                 .empty()) {
            return recipe;
        }
    }
    return std::nullopt;
}

bool cookAtCamp(entt::registry& registry,
                GameResources& resources,
                entt::entity camp)
{
    const auto recipe = matchingRecipeForCamp(registry, resources, camp);
    if (!recipe.has_value()) {
        resources.log.add("You do not have ingredients for a camp meal.");
        return false;
    }

    const auto selected =
        matchingIngredientsForRecipe(registry, resources.player, **recipe);
    for (const auto item : selected) {
        if (registry.valid(item)) {
            registry.destroy(item);
        }
    }

    if (!createRecipeOutputs(registry, resources.player, **recipe)) {
        resources.log.add("The recipe fails to produce anything useful.");
        return false;
    }

    resources.log.add("You cook " + (*recipe)->name + ".");
    return true;
}

} // namespace rl
