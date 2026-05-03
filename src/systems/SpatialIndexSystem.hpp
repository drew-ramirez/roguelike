#pragma once

#include <optional>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

class Map;

[[nodiscard]] std::optional<entt::entity> blockingEntityAt(
    const entt::registry& registry,
    Point position,
    entt::entity ignored = entt::null);

[[nodiscard]] bool isBlocked(
    const Map& map,
    const entt::registry& registry,
    Point position,
    entt::entity ignored = entt::null);

} // namespace rl