/**
 * **************************************************************************
 * @file     MeleeCombatSystem.hpp
 * @brief    Declares melee intent resolution
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

class GameLog;

void processMeleeCombat(entt::registry& registry, GameLog& log);

} // namespace rl
