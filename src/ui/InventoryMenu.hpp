/**
 * **************************************************************************
 * @file     InventoryMenu.hpp
 * @brief    Declares inventory modal input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
class Map;
struct GameResources;

enum class InventoryInputResult { None, Close, TookTurn, StartedTargeting };

[[nodiscard]] InventoryInputResult
handleInventoryInput(entt::registry& registry,
                     GameResources& resources,
                     const Map& map,
                     const std::vector<InputEvent>& events);

void drawInventoryMenu(ConsoleBuffer& console,
                       const entt::registry& registry,
                       const GameResources& resources);

} // namespace rl
