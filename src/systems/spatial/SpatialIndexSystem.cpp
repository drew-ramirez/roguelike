/**
 * **************************************************************************
 * @file     SpatialIndexSystem.cpp
 * @brief    Finds tile occupants for movement, pickup, and spawning
 * **************************************************************************
 */

#include "systems/spatial/SpatialIndexSystem.hpp"

#include "ecs/Components.hpp"
#include "map/Map.hpp"

namespace rl {

TileContent tileContentAt(const entt::registry& registry,
                          Point position,
                          entt::entity ignored)
{
    TileContent content;
    const auto view = registry.view<const Position>();
    for (const auto entity : view) {
        if (entity == ignored) {
            continue;
        }

        const auto& entityPosition = view.get<const Position>(entity);
        if (entityPosition.x != position.x || entityPosition.y != position.y) {
            continue;
        }

        content.entities.push_back(entity);
        if (!content.blocker.has_value() &&
            registry.all_of<BlocksTile>(entity)) {
            content.blocker = entity;
        }
        if (!content.item.has_value() && registry.all_of<Item>(entity)) {
            content.item = entity;
        }
        if (!content.trap.has_value() && registry.all_of<Trap>(entity)) {
            content.trap = entity;
        }
    }
    return content;
}

std::vector<entt::entity>
entitiesAt(const entt::registry& registry, Point position, entt::entity ignored)
{
    return tileContentAt(registry, position, ignored).entities;
}

std::optional<entt::entity> blockingEntityAt(const entt::registry& registry,
                                             Point position,
                                             entt::entity ignored)
{
    return tileContentAt(registry, position, ignored).blocker;
}

std::optional<entt::entity>
itemAt(const entt::registry& registry, Point position, entt::entity ignored)
{
    return tileContentAt(registry, position, ignored).item;
}

std::optional<entt::entity>
trapAt(const entt::registry& registry, Point position, entt::entity ignored)
{
    return tileContentAt(registry, position, ignored).trap;
}

bool isBlocked(const Map& map,
               const entt::registry& registry,
               Point position,
               entt::entity ignored)
{
    return map.blocksMovement(position.x, position.y) ||
           blockingEntityAt(registry, position, ignored).has_value();
}

} // namespace rl
