/**
 * **************************************************************************
 * @file     SpellMenu.hpp
 * @brief    Declares spell casting modal input and rendering
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

enum class SpellInputResult { None, Close, TookTurn, StartedTargeting };

[[nodiscard]] SpellInputResult
handleSpellInput(entt::registry& registry,
                 GameResources& resources,
                 const Map& map,
                 const std::vector<InputEvent>& events);

void drawSpellMenu(ConsoleBuffer& console,
                   const entt::registry& registry,
                   const GameResources& resources);

} // namespace rl
