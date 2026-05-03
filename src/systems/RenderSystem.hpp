/**
 * **************************************************************************
 * @file     RenderSystem.hpp
 * @brief    Declares ECS renderable drawing into the console buffer
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

class ConsoleBuffer;
class Map;

void renderEntities(const entt::registry& registry, ConsoleBuffer& console);
void renderEntities(const entt::registry& registry,
                    const Map& map,
                    ConsoleBuffer& console);

} // namespace rl
