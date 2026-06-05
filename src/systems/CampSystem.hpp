/**
 * **************************************************************************
 * @file     CampSystem.hpp
 * @brief    Declares camp rest actions for surface survival
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

[[nodiscard]] bool canRestAtCamp(const GameResources& resources,
                                 const entt::registry& registry,
                                 entt::entity camp);

[[nodiscard]] bool restAtCamp(entt::registry& registry,
                              GameResources& resources,
                              entt::entity camp);

} // namespace rl
