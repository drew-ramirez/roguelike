#include "systems/InputSystem.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "systems/SpatialIndexSystem.hpp"

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

bool tryMoveEntity(entt::registry& registry,
                   entt::entity entity,
                   const Map& map,
                   Point delta)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return false;
    }

    auto& position = registry.get<Position>(entity);
    const Point target{position.x + delta.x, position.y + delta.y};
    if (isBlocked(map, registry, target, entity)) {
        return false;
    }

    position.x = target.x;
    position.y = target.y;
    if (registry.all_of<Viewshed>(entity)) {
        // Moving changes what the entity can see, so queue a refresh
        registry.patch<Viewshed>(
            entity, [](Viewshed& viewshed) { viewshed.dirty = true; });
    }
    return true;
}

bool handlePlayerInput(entt::registry& registry,
                       const GameResources& resources,
                       const Map& map,
                       const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            if (resources.player == entt::null ||
                !registry.valid(resources.player) ||
                !registry.all_of<Position>(resources.player)) {
                return false;
            }

            const auto& position = registry.get<Position>(resources.player);
            const Point target{position.x + delta->x, position.y + delta->y};
            // Walking into a blocker becomes bump combat instead of movement
            if (const auto blocker =
                    blockingEntityAt(registry, target, resources.player);
                blocker.has_value()) {
                registry.emplace_or_replace<WantsToMelee>(
                    resources.player, WantsToMelee{*blocker});
                return true;
            }

            return tryMoveEntity(registry, resources.player, map, *delta);
        }
    }

    return false;
}

} // namespace rl
