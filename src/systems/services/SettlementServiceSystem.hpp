/**
 * **************************************************************************
 * @file     SettlementServiceSystem.hpp
 * @brief    Declares landmark settlement service actions
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

[[nodiscard]] bool settlementServiceAvailable(const GameResources& resources,
                                              const entt::registry& registry,
                                              entt::entity provider);

[[nodiscard]] bool useSettlementService(entt::registry& registry,
                                        GameResources& resources,
                                        entt::entity provider);

} // namespace rl
