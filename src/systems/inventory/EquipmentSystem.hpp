/**
 * **************************************************************************
 * @file     EquipmentSystem.hpp
 * @brief    Declares equipment ownership and derived combat stat helpers
 * **************************************************************************
 */

#pragma once

#include <string_view>

#include <entt/entt.hpp>

namespace rl {

struct Attributes;
class GameLog;

[[nodiscard]] bool equipOrUnequipItem(entt::registry& registry,
                                      GameLog& log,
                                      entt::entity owner,
                                      entt::entity item);

[[nodiscard]] int meleePowerWithEquipment(const entt::registry& registry,
                                          entt::entity entity);

[[nodiscard]] int defenseWithEquipment(const entt::registry& registry,
                                       entt::entity entity);

[[nodiscard]] Attributes attributesWithEquipment(const entt::registry& registry,
                                                 entt::entity entity);

[[nodiscard]] int attributeModifierWithEquipment(const entt::registry& registry,
                                                 entt::entity entity,
                                                 std::string_view attribute);

} // namespace rl
