#pragma once

#include <unordered_map>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json resourcesToJson(const entt::registry& registry,
                                   const GameResources& resources);
void loadResources(const Json& json,
                   const std::unordered_map<EntityId, entt::entity>& entities,
                   GameResources& resources);

} // namespace rl::save
