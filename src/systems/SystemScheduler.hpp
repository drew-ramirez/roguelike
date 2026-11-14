/**
 * **************************************************************************
 * @file     SystemScheduler.hpp
 * @brief    Declares deterministic single-threaded turn phase dispatch
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/RunState.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;
class Rng;

void resolveTurnSystems(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        RunState& state,
                        bool runMonsters);

} // namespace rl
