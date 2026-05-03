/**
 * **************************************************************************
 * @file     MonsterAI.hpp
 * @brief    Declares visible-monster movement and attack decisions
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

void runMonsterAi(entt::registry& registry,
                  const GameResources& resources,
                  const Map& map);

} // namespace rl
