/**
 * **************************************************************************
 * @file     MapRenderSystem.hpp
 * @brief    Declares map tile drawing into the console buffer
 * **************************************************************************
 */

#pragma once

namespace rl {

class ConsoleBuffer;
class Map;

void renderMap(const Map& map, ConsoleBuffer& console);

} // namespace rl
