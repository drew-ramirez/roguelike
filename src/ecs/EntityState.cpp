#include "ecs/EntityState.hpp"

#include "ecs/Components.hpp"

namespace rl {

std::string entityName(const entt::registry& registry, entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<Name>(entity)) {
        return registry.get<Name>(entity).value;
    }
    return "Something";
}

std::optional<Point> entityPosition(const entt::registry& registry,
                                    entt::entity entity)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return std::nullopt;
    }
    const auto& position = registry.get<Position>(entity);
    return Point{position.x, position.y};
}

void moveEntityTo(entt::registry& registry, entt::entity entity, Point point)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<Position>(entity)) {
        return;
    }
    auto& position = registry.get<Position>(entity);
    position.x = point.x;
    position.y = point.y;
    if (registry.all_of<Viewshed>(entity)) {
        registry.patch<Viewshed>(
            entity, [](Viewshed& viewshed) { viewshed.dirty = true; });
    }
}

bool itemOwnedBy(const entt::registry& registry,
                 entt::entity owner,
                 entt::entity item)
{
    return registry.valid(item) &&
           ((registry.all_of<InBackpack>(item) &&
             registry.get<InBackpack>(item).owner == owner) ||
            (registry.all_of<Equipped>(item) &&
             registry.get<Equipped>(item).owner == owner));
}

} // namespace rl
