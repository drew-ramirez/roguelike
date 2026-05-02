#pragma once

#include <entt/entt.hpp>

namespace rl {

class ConsoleBuffer;

void renderEntities(const entt::registry& registry, ConsoleBuffer& console);

} // namespace rl
