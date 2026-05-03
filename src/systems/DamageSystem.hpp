/**
 * **************************************************************************
 * @file     DamageSystem.hpp
 * @brief    Declares queued damage cleanup and death handling
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/RunState.hpp"

namespace rl {

struct GameResources;

void applyDamageAndCleanup(entt::registry& registry,
                           GameResources& resources,
                           RunState& state);

} // namespace rl
