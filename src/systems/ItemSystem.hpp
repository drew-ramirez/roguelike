/**
 * **************************************************************************
 * @file     ItemSystem.hpp
 * @brief    Declares pickup, inventory, use, and drop systems
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;
class Map;

[[nodiscard]] std::vector<entt::entity>
inventoryItems(const entt::registry& registry, entt::entity owner);

[[nodiscard]] std::optional<entt::entity>
itemAtPosition(const entt::registry& registry, Point position);

[[nodiscard]] bool queuePickupItemAtPlayer(entt::registry& registry,
                                           GameResources& resources);

void processPickupItems(entt::registry& registry, GameResources& resources);

[[nodiscard]] bool dropItemAtOwner(entt::registry& registry,
                                   GameResources& resources,
                                   entt::entity owner,
                                   entt::entity item);

[[nodiscard]] bool useInventoryItem(entt::registry& registry,
                                    GameResources& resources,
                                    const Map& map,
                                    entt::entity owner,
                                    entt::entity item);

} // namespace rl
