/**
 * **************************************************************************
 * @file     WorldTravelSystem.hpp
 * @brief    Declares surface macro and local-screen travel helpers
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "world/WorldTypes.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

[[nodiscard]] std::vector<RouteLink>
availableTravelRoutes(const GameResources& resources);

[[nodiscard]] int macroTravelHungerCost(const WorldCellDescriptor& cell);
[[nodiscard]] int localTravelHungerCost(const WorldCellDescriptor& cell);
[[nodiscard]] int travelHungerCost(const RouteLink& route) noexcept;

[[nodiscard]] bool moveWorldMapCursor(entt::registry& registry,
                                      GameResources& resources,
                                      Map& map,
                                      IFovProvider& fovProvider,
                                      Point direction);

[[nodiscard]] bool enterWorldMapSelection(entt::registry& registry,
                                          GameResources& resources,
                                          Map& map,
                                          IFovProvider& fovProvider,
                                          bool dangerEncounter = false);

[[nodiscard]] bool tryTravelByEdge(entt::registry& registry,
                                   GameResources& resources,
                                   Map& map,
                                   IFovProvider& fovProvider,
                                   Point direction);

[[nodiscard]] bool travelToRegion(entt::registry& registry,
                                  GameResources& resources,
                                  Map& map,
                                  IFovProvider& fovProvider,
                                  const std::string& destinationRegionId);

} // namespace rl
