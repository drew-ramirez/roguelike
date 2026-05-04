/**
 * **************************************************************************
 * @file     TrapSpawner.hpp
 * @brief    Declares hidden trap entity factories and level placement
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/Rect.hpp"

namespace rl {

[[nodiscard]] entt::entity spawnSpikeTrap(entt::registry& registry,
                                          Point position,
                                          bool hidden = true,
                                          bool triggered = false);

[[nodiscard]] std::size_t spawnTraps(entt::registry& registry,
                                     const std::vector<Rect>& rooms,
                                     Rng& rng,
                                     Point playerStart,
                                     Point stairsPosition,
                                     int depth);

} // namespace rl
