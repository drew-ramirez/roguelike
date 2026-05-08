/**
 * **************************************************************************
 * @file     BystanderAI.hpp
 * @brief    Declares neutral bystander turn behavior
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

void runBystanderAi(entt::registry& registry,
                    GameResources& resources,
                    const Map& map);

} // namespace rl
