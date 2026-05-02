#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "input/InputEvent.hpp"

namespace rl {

struct GameResources;
class Map;

[[nodiscard]] std::optional<Point> movementDeltaForKey(Key key);
[[nodiscard]] bool tryMoveEntity(entt::registry& registry, entt::entity entity, const Map& map, Point delta);
void handlePlayerInput(entt::registry& registry, const GameResources& resources, const Map& map, const std::vector<InputEvent>& events);

} // namespace rl
