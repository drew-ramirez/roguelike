/**
 * **************************************************************************
 * @file     TrapSystem.hpp
 * @brief    Declares player-only hidden trap trigger behavior
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

void processPlayerTraps(entt::registry& registry, GameResources& resources);

} // namespace rl
