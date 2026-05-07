/**
 * **************************************************************************
 * @file     MapRenderSystem.hpp
 * @brief    Declares map tile drawing into the console buffer
 * **************************************************************************
 */

#pragma once

namespace rl {

class Camera;
class ConsoleBuffer;
class Map;

void renderMap(const Map& map, ConsoleBuffer& console);
void renderMap(const Map& map, ConsoleBuffer& console, const Camera& camera);
void renderMapDebug(const Map& map,
                    ConsoleBuffer& console,
                    const Camera& camera);

} // namespace rl
