#pragma once

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

[[nodiscard]] entt::entity spawnPlayer(entt::registry& registry, Point position);

} // namespace rl
