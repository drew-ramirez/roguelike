/**
 * **************************************************************************
 * @file     RangedCombatSystem.hpp
 * @brief    Declares no-ammo ranged attacks and breath powers
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

[[nodiscard]] bool processRangedCombat(entt::registry& registry,
                                       GameResources& resources,
                                       const Map& map);

} // namespace rl
