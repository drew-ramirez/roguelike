/**
 * **************************************************************************
 * @file     EquipmentSystem.hpp
 * @brief    Declares equipment ownership helpers
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

class GameLog;

[[nodiscard]] bool equipOrUnequipItem(entt::registry& registry,
                                      GameLog& log,
                                      entt::entity owner,
                                      entt::entity item);

} // namespace rl
