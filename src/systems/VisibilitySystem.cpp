#include "systems/VisibilitySystem.hpp"

#include "ecs/Components.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"

namespace rl {

void updateVisibility(entt::registry& registry, Map& map, entt::entity viewer, const IFovProvider& fovProvider)
{
    if (viewer == entt::null || !registry.valid(viewer) || !registry.all_of<Position, Viewshed>(viewer)) {
        return;
    }

    auto& viewshed = registry.get<Viewshed>(viewer);
    if (!viewshed.dirty) {
        return;
    }

    const auto& position = registry.get<Position>(viewer);
    viewshed.visible = fovProvider.compute(map, Point{position.x, position.y}, viewshed.range);
    viewshed.dirty = false;

    map.clearVisible();
    for (const Point point : viewshed.visible) {
        map.setVisible(point.x, point.y);
    }
}

} // namespace rl
