/**
 * **************************************************************************
 * @file     VisibilitySystem.cpp
 * @brief    Refreshes visible tiles and revealed map memory
 * **************************************************************************
 */

#include "systems/spatial/VisibilitySystem.hpp"

#include "ecs/Components.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"

namespace rl {

void updateVisibility(entt::registry& registry,
                      Map& map,
                      entt::entity viewer,
                      const IFovProvider& fovProvider)
{
    if (viewer == entt::null || !registry.valid(viewer) ||
        !registry.all_of<Position, Viewshed>(viewer)) {
        return;
    }

    auto& viewshed = registry.get<Viewshed>(viewer);
    // Viewsheds refresh only after another system marks them dirty
    if (!viewshed.dirty) {
        return;
    }

    const auto& position = registry.get<Position>(viewer);
    viewshed.visible =
        fovProvider.compute(map, Point{position.x, position.y}, viewshed.range);
    viewshed.dirty = false;

    // Visible resets each update, while revealed stays as map memory
    map.clearVisible();
    for (const Point point : viewshed.visible) {
        map.setVisible(point.x, point.y);
    }
}

} // namespace rl
