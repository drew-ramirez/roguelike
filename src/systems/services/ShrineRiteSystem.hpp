/**
 * **************************************************************************
 * @file     ShrineRiteSystem.hpp
 * @brief    Declares shrine offering interactions
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

[[nodiscard]] bool shrineAvailable(const GameResources& resources,
                                   const entt::registry& registry,
                                   entt::entity shrineEntity);

[[nodiscard]] bool offerAtShrine(entt::registry& registry,
                                 GameResources& resources,
                                 entt::entity shrineEntity);

} // namespace rl
