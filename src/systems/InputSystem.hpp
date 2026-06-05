/**
 * **************************************************************************
 * @file     InputSystem.hpp
 * @brief    Declares player input handling for movement and bump combat
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "input/InputEvent.hpp"

namespace rl {

struct GameResources;
class Map;

enum class PlayerInputResult {
    None,
    TookTurn,
    OpenInventory,
    OpenCamp,
    OpenSettlementService,
    OpenVendor,
    OpenSpellMenu,
    StartedTargeting,
    OpenWorldMap,
    TravelLocalEdge,
    UsePortal,
    Descend,
    Ascend,
    Save
};

/**
 * Converts supported movement keys into tile deltas
 *
 * @return Empty when the key is not movement input
 */
[[nodiscard]] std::optional<Point> movementDeltaForKey(Key key);

/**
 * Moves an entity when the destination is walkable and not entity-blocked
 *
 * Marks its viewshed dirty after movement so visibility refreshes next
 */
[[nodiscard]] bool tryMoveEntity(entt::registry& registry,
                                 entt::entity entity,
                                 const Map& map,
                                 Point delta);

/**
 * Applies map-context player input without letting menus consume movement
 */
[[nodiscard]] PlayerInputResult
handleMapInput(entt::registry& registry,
               GameResources& resources,
               Map& map,
               const std::vector<InputEvent>& events);

/**
 * Applies one player key event as movement or bump combat
 *
 * @return True when input consumed a turn
 */
[[nodiscard]] bool handlePlayerInput(entt::registry& registry,
                                     const GameResources& resources,
                                     const Map& map,
                                     const std::vector<InputEvent>& events);

} // namespace rl
