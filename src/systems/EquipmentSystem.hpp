/**
 * **************************************************************************
 * @file     EquipmentSystem.hpp
 * @brief    Declares equipment ownership and derived combat stat helpers
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

[[nodiscard]] int meleePowerWithEquipment(const entt::registry& registry,
                                          entt::entity entity);

[[nodiscard]] int defenseWithEquipment(const entt::registry& registry,
                                       entt::entity entity);

} // namespace rl
