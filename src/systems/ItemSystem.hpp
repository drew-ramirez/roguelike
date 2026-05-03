/**
 * **************************************************************************
 * @file     ItemSystem.hpp
 * @brief    Declares item lookup and pickup systems
 * **************************************************************************
 */

#pragma once

#include <optional>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;

[[nodiscard]] std::optional<entt::entity>
itemAtPosition(const entt::registry& registry, Point position);

[[nodiscard]] bool queuePickupItemAtPlayer(entt::registry& registry,
                                           GameResources& resources);

void processPickupItems(entt::registry& registry, GameResources& resources);

} // namespace rl
