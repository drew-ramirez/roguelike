/**
 * **************************************************************************
 * @file     PortalSystem.hpp
 * @brief    Declares town portal activation and return behavior
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

[[nodiscard]] bool playerIsOnTownPortal(const entt::registry& registry,
                                        const GameResources& resources);

[[nodiscard]] bool tryUseTownPortal(entt::registry& registry,
                                    GameResources& resources,
                                    Map& map,
                                    IFovProvider& fovProvider,
                                    entt::entity user);

[[nodiscard]] bool tryUseReturnPortal(entt::registry& registry,
                                      GameResources& resources,
                                      Map& map,
                                      IFovProvider& fovProvider);

} // namespace rl
