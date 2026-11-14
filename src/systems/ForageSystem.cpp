/**
 * **************************************************************************
 * @file     ForageSystem.cpp
 * @brief    Resolves biome resource gathering for surface survival
 * **************************************************************************
 */

#include "systems/ForageSystem.hpp"

#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "systems/HungerSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

[[nodiscard]] std::string_view resourceTableId(BiomeVariant biome) noexcept
{
    if (biome == BiomeVariant::LimestoneRise ||
        biome == BiomeVariant::Sinklands) {
        return "limestone_cave";
    }
    if (biome == BiomeVariant::ReedMarsh) {
        return "reed_marsh";
    }
    return "temperate_forest";
}

[[nodiscard]] int totalWeight(const RawBiomeResourceTable& table)
{
    int total = 0;
    for (const auto& resource : table.resources) {
        total += resource.weight;
    }
    return total;
}

[[nodiscard]] const RawBiomeResourceEntry*
rollResource(const RawBiomeResourceTable& table, Rng& rng)
{
    const int total = totalWeight(table);
    if (total <= 0) {
        return nullptr;
    }

    int roll = rng.rangeInclusive(1, total);
    for (const auto& resource : table.resources) {
        roll -= resource.weight;
        if (roll <= 0) {
            return &resource;
        }
    }
    return nullptr;
}

} // namespace

std::optional<entt::entity>
spawnIngredientInBackpack(entt::registry& registry,
                          GameResources& resources,
                          const std::string& ingredientId)
{
    const auto* ingredient = defaultRawDatabase().ingredient(ingredientId);
    if (ingredient == nullptr || !ingredient->item.has_value()) {
        return std::nullopt;
    }

    const auto item = spawnRawEntity(
        registry, defaultRawDatabase(), *ingredient->item, Point{0, 0});
    if (!item.has_value()) {
        return std::nullopt;
    }

    if (registry.all_of<Position>(*item)) {
        registry.remove<Position>(*item);
    }
    registry.emplace_or_replace<InBackpack>(*item,
                                            InBackpack{resources.player});

    const auto* region = currentWorldRegion(resources.world);
    registry.emplace_or_replace<Ingredient>(
        *item,
        Ingredient{ingredient->tags,
                   region == nullptr
                       ? std::string{}
                       : std::string{biomeVariantId(region->biome)},
                   region == nullptr ? std::string{} : region->id,
                   1,
                   0});
    return item;
}

bool forageCurrentRegion(entt::registry& registry, GameResources& resources)
{
    const auto* region = currentWorldRegion(resources.world);
    if (region == nullptr) {
        resources.log.add("You do not know this region well enough to forage.");
        return false;
    }

    const auto* table =
        defaultRawDatabase().biomeResourceTable(resourceTableId(region->biome));
    if (table == nullptr) {
        resources.log.add("There is nothing obvious to forage here.");
        return false;
    }

    const auto* resource = rollResource(*table, resources.rng);
    if (resource == nullptr) {
        resources.log.add("You find nothing useful.");
        return false;
    }

    const int count =
        resources.rng.rangeInclusive(resource->minCount, resource->maxCount);
    int found = 0;
    for (int index = 0; index < count; ++index) {
        if (spawnIngredientInBackpack(registry, resources, resource->ingredient)
                .has_value()) {
            ++found;
        }
    }

    if (found == 0) {
        resources.log.add("You find signs of resources, but gather nothing.");
        return false;
    }

    const auto* ingredient =
        defaultRawDatabase().ingredient(resource->ingredient);
    resources.log.add(
        "You forage " + std::to_string(found) + " " +
        (ingredient == nullptr ? resource->ingredient : ingredient->name) +
        ".");
    consumeHunger(registry, resources, 10);
    return true;
}

} // namespace rl
