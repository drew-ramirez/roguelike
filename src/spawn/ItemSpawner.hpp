/**
 * **************************************************************************
 * @file     ItemSpawner.hpp
 * @brief    Declares item and equipment entity factory functions
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

[[nodiscard]] entt::entity
spawnHealingPotion(entt::registry& registry, Point position, int amount = 8);
[[nodiscard]] entt::entity spawnFireballScroll(entt::registry& registry,
                                               Point position);
[[nodiscard]] entt::entity spawnDagger(entt::registry& registry,
                                       Point position);
[[nodiscard]] entt::entity spawnSword(entt::registry& registry, Point position);
[[nodiscard]] entt::entity spawnChainMail(entt::registry& registry,
                                          Point position);

} // namespace rl
