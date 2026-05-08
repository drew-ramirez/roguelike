/**
 * **************************************************************************
 * @file     MonsterAI.hpp
 * @brief    Declares hostile monster movement and attack decisions
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

void runHostileMonsterAi(entt::registry& registry,
                         GameResources& resources,
                         const Map& map);

void runMonsterAi(entt::registry& registry,
                  GameResources& resources,
                  const Map& map);

} // namespace rl
