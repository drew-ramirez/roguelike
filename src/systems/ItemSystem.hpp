/**
 * **************************************************************************
 * @file     ItemSystem.hpp
 * @brief    Declares pickup, inventory, use, drop, and targeting systems
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
                                    Map& map,
                                    entt::entity owner,
                                    entt::entity item);

[[nodiscard]] std::vector<entt::entity> itemEffectTargets(
    const entt::registry& registry, entt::entity owner, entt::entity source);

[[nodiscard]] bool useInventoryItemOnTargetItem(entt::registry& registry,
                                                GameResources& resources,
                                                Map& map,
                                                entt::entity owner,
                                                entt::entity source,
                                                entt::entity target);

[[nodiscard]] bool
processItemUses(entt::registry& registry, GameResources& resources, Map& map);

[[nodiscard]] bool isValidTarget(const entt::registry& registry,
                                 const GameResources& resources,
                                 const Map& map,
                                 Point target);

[[nodiscard]] bool confirmPendingTarget(entt::registry& registry,
                                        GameResources& resources,
                                        const Map& map);

void cancelPendingTarget(GameResources& resources);

void moveTargetCursor(GameResources& resources, Point delta);

} // namespace rl
