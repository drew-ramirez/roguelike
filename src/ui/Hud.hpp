#pragma once

#include <entt/entt.hpp>

#include "core/RunState.hpp"

namespace rl {

struct GameResources;
class ConsoleBuffer;

void drawHud(ConsoleBuffer& console, const GameResources& resources, const entt::registry& registry, RunState state);

} // namespace rl
