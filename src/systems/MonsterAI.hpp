#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class Map;

void runMonsterAi(entt::registry& registry, const GameResources& resources, const Map& map);

} // namespace rl
