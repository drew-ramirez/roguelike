/**
 * **************************************************************************
 * @file     ItemSelectionMenu.hpp
 * @brief    Declares selected-item effect modal input and rendering
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

enum class ItemSelectionInputResult { None, Close, TookTurn };

[[nodiscard]] ItemSelectionInputResult
handleItemSelectionInput(entt::registry& registry,
                         GameResources& resources,
                         Map& map,
                         const std::vector<InputEvent>& events);

void drawItemSelectionMenu(ConsoleBuffer& console,
                           const entt::registry& registry,
                           const GameResources& resources);

} // namespace rl
