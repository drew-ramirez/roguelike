/**
 * **************************************************************************
 * @file     SpatialIndexSystem.hpp
 * @brief    Declares tile-content queries across map and ECS entities
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

class Map;

struct TileContent {
    std::vector<entt::entity> entities;
    std::optional<entt::entity> blocker;
    std::optional<entt::entity> item;
    std::optional<entt::entity> trap;
};

[[nodiscard]] TileContent tileContentAt(const entt::registry& registry,
                                        Point position,
                                        entt::entity ignored = entt::null);

[[nodiscard]] std::vector<entt::entity>
entitiesAt(const entt::registry& registry,
           Point position,
           entt::entity ignored = entt::null);

[[nodiscard]] std::optional<entt::entity>
blockingEntityAt(const entt::registry& registry,
                 Point position,
                 entt::entity ignored = entt::null);

[[nodiscard]] std::optional<entt::entity>
itemAt(const entt::registry& registry,
       Point position,
       entt::entity ignored = entt::null);

[[nodiscard]] std::optional<entt::entity>
trapAt(const entt::registry& registry,
       Point position,
       entt::entity ignored = entt::null);

[[nodiscard]] bool isBlocked(const Map& map,
                             const entt::registry& registry,
                             Point position,
                             entt::entity ignored = entt::null);

} // namespace rl
