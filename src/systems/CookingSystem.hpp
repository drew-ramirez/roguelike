/**
 * **************************************************************************
 * @file     CookingSystem.hpp
 * @brief    Declares camp recipe matching and cooking helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
struct RawRecipe;

[[nodiscard]] std::vector<entt::entity>
carriedIngredients(const entt::registry& registry, entt::entity owner);

[[nodiscard]] std::optional<const RawRecipe*>
matchingRecipeForCamp(const entt::registry& registry,
                      const GameResources& resources,
                      entt::entity camp);

[[nodiscard]] bool cookAtCamp(entt::registry& registry,
                              GameResources& resources,
                              entt::entity camp);

} // namespace rl
