/**
 * **************************************************************************
 * @file     ShrineMenu.hpp
 * @brief    Declares shrine menu input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
struct GameResources;

enum class ShrineInputResult { None, Close, TookTurn };

[[nodiscard]] ShrineInputResult
handleShrineInput(entt::registry& registry,
                  GameResources& resources,
                  const std::vector<InputEvent>& events);

void drawShrineMenu(ConsoleBuffer& console,
                    const entt::registry& registry,
                    const GameResources& resources);

} // namespace rl
