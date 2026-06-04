/**
 * **************************************************************************
 * @file     VendorMenu.hpp
 * @brief    Declares vendor buy/sell modal input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
struct GameResources;

enum class VendorInputResult { None, Close, TookTurn };

[[nodiscard]] VendorInputResult
handleVendorInput(entt::registry& registry,
                  GameResources& resources,
                  const std::vector<InputEvent>& events);

void drawVendorMenu(ConsoleBuffer& console,
                    const entt::registry& registry,
                    const GameResources& resources);

} // namespace rl
