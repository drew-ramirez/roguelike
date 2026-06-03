#pragma once

#include <unordered_map>

#include <entt/entt.hpp>

#include "save/SaveTypes.hpp"

namespace rl::save {

void ensureSaveIds(entt::registry& registry);
[[nodiscard]] EntityId entityId(const entt::registry& registry,
                                entt::entity entity);
[[nodiscard]] Json entityToJson(const entt::registry& registry,
                                entt::entity entity);
void loadEntityImmediateComponents(entt::registry& registry,
                                   entt::entity entity,
                                   EntityId id,
                                   const Json& components);
void loadEntityReferences(
    entt::registry& registry,
    entt::entity entity,
    const Json& components,
    const std::unordered_map<EntityId, entt::entity>& entities);

} // namespace rl::save
