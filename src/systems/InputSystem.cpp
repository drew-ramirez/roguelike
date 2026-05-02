#include "systems/InputSystem.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"

namespace rl {

std::optional<Point> movementDeltaForKey(Key key)
{
    switch (key) {
    case Key::Up:
    case Key::W:
        return Point{0, -1};
    case Key::Down:
    case Key::S:
        return Point{0, 1};
    case Key::Left:
    case Key::A:
        return Point{-1, 0};
    case Key::Right:
    case Key::D:
        return Point{1, 0};
    case Key::Unknown:
        return std::nullopt;
    }

    return std::nullopt;
}

bool tryMoveEntity(entt::registry& registry, entt::entity entity, const Map& map, Point delta)
{
    if (entity == entt::null || !registry.valid(entity) || !registry.all_of<Position>(entity)) {
        return false;
    }

    auto& position = registry.get<Position>(entity);
    const Point target{position.x + delta.x, position.y + delta.y};
    if (map.blocksMovement(target.x, target.y)) {
        return false;
    }

    position.x = target.x;
    position.y = target.y;
    if (registry.all_of<Viewshed>(entity)) {
        registry.patch<Viewshed>(entity, [](Viewshed& viewshed) {
            viewshed.dirty = true;
        });
    }
    return true;
}

void handlePlayerInput(entt::registry& registry, const GameResources& resources, const Map& map, const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            [[maybe_unused]] const bool moved = tryMoveEntity(registry, resources.player, map, *delta);
        }
    }
}

} // namespace rl