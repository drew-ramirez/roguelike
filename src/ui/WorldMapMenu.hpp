/**
 * **************************************************************************
 * @file     WorldMapMenu.hpp
 * @brief    Declares world-map modal input and rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;
struct GameResources;
struct WorldMapState;

enum class WorldMapInputResult { None, Close, MoveCursor, EnterSelected };

[[nodiscard]] WorldMapInputResult
handleWorldMapInput(GameResources& resources,
                    const std::vector<InputEvent>& events);

void drawWorldMapMenu(ConsoleBuffer& console, const WorldMapState& world);

} // namespace rl
