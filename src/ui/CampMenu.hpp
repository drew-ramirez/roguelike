/**
 * **************************************************************************
 * @file     CampMenu.hpp
 * @brief    Declares camp action menu input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
struct GameResources;

enum class CampInputResult { None, Close, TookTurn };

[[nodiscard]] CampInputResult
handleCampInput(entt::registry& registry,
                GameResources& resources,
                const std::vector<InputEvent>& events);

void drawCampMenu(ConsoleBuffer& console,
                  const entt::registry& registry,
                  const GameResources& resources);

} // namespace rl
