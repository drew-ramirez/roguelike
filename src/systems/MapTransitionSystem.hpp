/**
 * **************************************************************************
 * @file     MapTransitionSystem.hpp
 * @brief    Declares one-way dungeon descent behavior
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

[[nodiscard]] bool playerIsOnDownStairs(const entt::registry& registry,
                                        const GameResources& resources,
                                        const Map& map);

[[nodiscard]] bool tryDescendToNextLevel(entt::registry& registry,
                                         GameResources& resources,
                                         Map& map,
                                         IFovProvider& fovProvider);

} // namespace rl
