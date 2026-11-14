#pragma once

#include <optional>
#include <string>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

[[nodiscard]] std::string entityName(const entt::registry& registry,
                                     entt::entity entity);
[[nodiscard]] std::optional<Point>
entityPosition(const entt::registry& registry, entt::entity entity);
void moveEntityTo(entt::registry& registry, entt::entity entity, Point point);
[[nodiscard]] bool itemOwnedBy(const entt::registry& registry,
                               entt::entity owner,
                               entt::entity item);

} // namespace rl
