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
class Rng;

void processMeleeCombat(entt::registry& registry,
                        GameLog& log,
                        Rng* rng = nullptr);

} // namespace rl
