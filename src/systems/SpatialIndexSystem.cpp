#include "systems/SpatialIndexSystem.hpp"

#include "ecs/Components.hpp"
#include "map/Map.hpp"

namespace rl {

std::optional<entt::entity> blockingEntityAt(const entt::registry& registry,
                                             Point position,
                                             entt::entity ignored)
{
    const auto view = registry.view<const Position, const BlocksTile>();
    for (const auto entity : view) {
        if (entity == ignored) {
            continue;
        }

        const auto& blockerPosition = view.get<const Position>(entity);
        if (blockerPosition.x == position.x &&
            blockerPosition.y == position.y) {
            return entity;
        }
    }

    return std::nullopt;
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
