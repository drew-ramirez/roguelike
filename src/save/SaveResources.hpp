#pragma once

#include <unordered_map>

#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json resourcesToJson(const entt::registry& registry,
                                   const GameResources& resources);
void loadResources(const Json& json,
                   const std::unordered_map<EntityId, entt::entity>& entities,
                   GameResources& resources);

} // namespace rl::save