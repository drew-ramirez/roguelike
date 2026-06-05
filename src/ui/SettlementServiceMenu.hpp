/**
 * **************************************************************************
 * @file     SettlementServiceMenu.hpp
 * @brief    Declares settlement service menu input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
struct GameResources;

enum class SettlementServiceInputResult { None, Close, TookTurn };

[[nodiscard]] SettlementServiceInputResult
handleSettlementServiceInput(entt::registry& registry,
                             GameResources& resources,
                             const std::vector<InputEvent>& events);

void drawSettlementServiceMenu(ConsoleBuffer& console,
                               const entt::registry& registry,
                               const GameResources& resources);

} // namespace rl
