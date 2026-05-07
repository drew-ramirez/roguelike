/**
 * **************************************************************************
 * @file     ItemSpawner.hpp
 * @brief    Declares item and equipment entity factory functions
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"
#include "map/Rect.hpp"

namespace rl {

[[nodiscard]] entt::entity
spawnHealingPotion(entt::registry& registry, Point position, int amount = 8);
[[nodiscard]] entt::entity spawnFoodRation(entt::registry& registry,
                                           Point position);
[[nodiscard]] entt::entity spawnFireballScroll(entt::registry& registry,
                                               Point position);
[[nodiscard]] entt::entity spawnMagicMappingScroll(entt::registry& registry,
                                                   Point position);
[[nodiscard]] entt::entity spawnDagger(entt::registry& registry,
                                       Point position);
[[nodiscard]] entt::entity spawnSword(entt::registry& registry, Point position);
[[nodiscard]] entt::entity spawnChainMail(entt::registry& registry,
                                          Point position);

[[nodiscard]] std::size_t spawnItems(entt::registry& registry,
                                     const std::vector<Rect>& rooms,
                                     Rng& rng,
                                     Point playerStart,
                                     int depth);
[[nodiscard]] std::size_t spawnItems(entt::registry& registry,
                                     const std::vector<SpawnRegion>& regions,
                                     const Map& map,
                                     Rng& rng,
                                     Point playerStart,
                                     int depth);

} // namespace rl
