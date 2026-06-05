/**
 * **************************************************************************
 * @file     ForageSystem.hpp
 * @brief    Declares biome resource gathering helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string>

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

[[nodiscard]] std::optional<entt::entity>
spawnIngredientInBackpack(entt::registry& registry,
                          GameResources& resources,
                          const std::string& ingredientId);

[[nodiscard]] bool forageCurrentRegion(entt::registry& registry,
                                       GameResources& resources);

} // namespace rl
