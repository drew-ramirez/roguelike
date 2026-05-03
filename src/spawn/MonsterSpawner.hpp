#pragma once

#include <cstddef>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/Rect.hpp"

namespace rl {

[[nodiscard]] entt::entity spawnGoblin(entt::registry& registry,
                                       Point position);
[[nodiscard]] entt::entity spawnOrc(entt::registry& registry, Point position);
[[nodiscard]] entt::entity spawnTroll(entt::registry& registry, Point position);
[[nodiscard]] std::size_t spawnMonsters(entt::registry& registry,
                                        const std::vector<Rect>& rooms,
                                        Rng& rng,
                                        Point playerStart,
                                        int depth = 1);

} // namespace rl
