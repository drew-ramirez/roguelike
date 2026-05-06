/**
 * **************************************************************************
 * @file     RenderSystem.cpp
 * @brief    Sorts and draws visible ECS renderables
 * **************************************************************************
 */

#include "systems/RenderSystem.hpp"

#include <algorithm>
#include <vector>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"

namespace rl {

namespace {

struct EntityDraw {
    Position position;
    Renderable renderable;
};

} // namespace

void renderEntityView(const entt::registry& registry,
                      const Map* map,
                      ConsoleBuffer& console,
                      const Camera* camera)
{
    std::vector<EntityDraw> draws;

    const auto view = registry.view<const Position, const Renderable>();
    for (const auto entity : view) {
        if (registry.all_of<Hidden>(entity)) {
            continue;
        }

        const auto& position = view.get<const Position>(entity);
        if (map != nullptr && !map->isVisible(position.x, position.y)) {
            continue;
        }
        if (camera != nullptr &&
            !camera->containsWorld(Point{position.x, position.y})) {
            continue;
        }

        const auto& renderable = view.get<const Renderable>(entity);
        draws.push_back(EntityDraw{position, renderable});
    }

    std::sort(draws.begin(),
              draws.end(),
              [](const EntityDraw& left, const EntityDraw& right) {
                  return left.renderable.order < right.renderable.order;
              });

    for (const auto& draw : draws) {
        Point screen{draw.position.x, draw.position.y};
        if (camera != nullptr) {
            const auto converted =
                camera->worldToScreen(Point{draw.position.x, draw.position.y});
            if (!converted.has_value()) {
                continue;
            }
            screen = *converted;
        }
        console.set(screen.x,
                    screen.y,
                    draw.renderable.glyph,
                    draw.renderable.foreground,
                    draw.renderable.background);
    }
}

void renderEntities(const entt::registry& registry, ConsoleBuffer& console)
{
    renderEntityView(registry, nullptr, console, nullptr);
}

void renderEntities(const entt::registry& registry,
                    const Map& map,
                    ConsoleBuffer& console)
{
    renderEntityView(registry, &map, console, nullptr);
}

void renderEntities(const entt::registry& registry,
                    const Map& map,
                    ConsoleBuffer& console,
                    const Camera& camera)
{
    renderEntityView(registry, &map, console, &camera);
}

} // namespace rl
