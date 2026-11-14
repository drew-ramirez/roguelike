/**
 * **************************************************************************
 * @file     MapTransitionSystem.hpp
 * @brief    Declares one-way dungeon descent behavior
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

enum class LevelArrivalMode { DownStairs, UpStairs, PortalReturn };

[[nodiscard]] bool playerIsOnDownStairs(const entt::registry& registry,
                                        const GameResources& resources,
                                        const Map& map);
[[nodiscard]] bool playerIsOnUpStairs(const entt::registry& registry,
                                      const GameResources& resources,
                                      const Map& map);

[[nodiscard]] bool tryChangeLevel(entt::registry& registry,
                                  GameResources& resources,
                                  Map& map,
                                  IFovProvider& fovProvider,
                                  int targetDepth,
                                  LevelArrivalMode arrivalMode);

[[nodiscard]] bool tryDescendToNextLevel(entt::registry& registry,
                                         GameResources& resources,
                                         Map& map,
                                         IFovProvider& fovProvider);
[[nodiscard]] bool tryAscendToPreviousLevel(entt::registry& registry,
                                            GameResources& resources,
                                            Map& map,
                                            IFovProvider& fovProvider);

[[nodiscard]] bool tryEnterSurfaceScreen(entt::registry& registry,
                                         GameResources& resources,
                                         Map& map,
                                         IFovProvider& fovProvider,
                                         Point macro,
                                         Point screen,
                                         bool dangerEncounter = false);

/** Rebuilds the active surface screen after a generation-version migration. */
[[nodiscard]] bool regenerateCurrentSurfaceScreen(entt::registry& registry,
                                                  GameResources& resources,
                                                  Map& map);

} // namespace rl
