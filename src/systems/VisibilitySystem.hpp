#pragma once

#include <entt/entt.hpp>

namespace rl {

class IFovProvider;
class Map;

void updateVisibility(entt::registry& registry,
                      Map& map,
                      entt::entity viewer,
                      const IFovProvider& fovProvider);

} // namespace rl
