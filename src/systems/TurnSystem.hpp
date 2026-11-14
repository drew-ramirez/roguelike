/**
 * **************************************************************************
 * @file     TurnSystem.hpp
 * @brief    Declares deterministic gameplay turn resolution
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/RunState.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

void resolveTurnSystems(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        RunState& state,
                        bool runMonsters);

} // namespace rl
