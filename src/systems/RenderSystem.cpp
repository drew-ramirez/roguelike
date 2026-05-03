#include "systems/RenderSystem.hpp"

#include <algorithm>
#include <vector>

#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"

namespace rl {

namespace {

struct EntityDraw {
    Position position;
    Renderable renderable;
};

} // namespace

void renderEntities(const entt::registry& registry, ConsoleBuffer& console)
{
    std::vector<EntityDraw> draws;

    const auto view = registry.view<const Position, const Renderable>();
    for (const auto entity : view) {
        const auto& position = view.get<const Position>(entity);
        const auto& renderable = view.get<const Renderable>(entity);
        draws.push_back(EntityDraw{position, renderable});
    }

    std::sort(draws.begin(),
              draws.end(),
              [](const EntityDraw& left, const EntityDraw& right) {
                  return left.renderable.order < right.renderable.order;
              });

    for (const auto& draw : draws) {
        console.set(draw.position.x,
                    draw.position.y,
                    draw.renderable.glyph,
                    draw.renderable.foreground,
                    draw.renderable.background);
    }
}

} // namespace rl
