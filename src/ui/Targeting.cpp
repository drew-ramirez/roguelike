#include "ui/Targeting.hpp"

#include <algorithm>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/inventory/ItemSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] Point playerPoint(const entt::registry& registry,
                                const GameResources& resources)
{
    if (resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Position>(resources.player)) {
        return {};
    }

    const auto& position = registry.get<Position>(resources.player);
    return Point{position.x, position.y};
}

} // namespace

TargetingInputResult handleTargetingInput(entt::registry& registry,
                                          GameResources& resources,
                                          const Map& map,
                                          const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (event.key == Key::Escape) {
            cancelPendingTarget(resources);
            return TargetingInputResult::Close;
        }

        if (event.key == Key::Enter) {
            return confirmPendingTarget(registry, resources, map)
                       ? TargetingInputResult::TookTurn
                       : TargetingInputResult::None;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            moveTargetCursor(resources, *delta);
            if (resources.pendingTargeting.has_value()) {
                auto& cursor = resources.pendingTargeting->cursor;
                cursor.x = std::clamp(cursor.x, 0, map.width() - 1);
                cursor.y = std::clamp(cursor.y, 0, map.height() - 1);
            }
            return TargetingInputResult::None;
        }
    }

    return TargetingInputResult::None;
}

void drawTargetingOverlay(ConsoleBuffer& console,
                          const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    const Camera camera{map.width(), map.height()};
    drawTargetingOverlay(console, registry, resources, map, camera);
}

void drawTargetingOverlay(ConsoleBuffer& console,
                          const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map,
                          const Camera& camera)
{
    if (!resources.pendingTargeting.has_value()) {
        return;
    }

    const auto origin = playerPoint(registry, resources);
    const int range = resources.pendingTargeting->range;
    const int rangeSquared = range * range;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point point{x, y};
            if (!map.isVisible(x, y) ||
                distanceSquared(origin, point) > rangeSquared) {
                continue;
            }
            const auto screen = camera.worldToScreen(point);
            if (screen.has_value()) {
                console.set(screen->x,
                            screen->y,
                            U'*',
                            Color{80, 120, 220, 255},
                            Color::black());
            }
        }
    }

    const auto cursor = resources.pendingTargeting->cursor;
    const auto screenCursor = camera.worldToScreen(cursor);
    if (screenCursor.has_value()) {
        console.set(screenCursor->x,
                    screenCursor->y,
                    U'X',
                    isValidTarget(registry, resources, map, cursor)
                        ? Color::yellow()
                        : Color{255, 80, 80, 255},
                    Color::black());
    }
}

} // namespace rl
