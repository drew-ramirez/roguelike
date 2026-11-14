#pragma once

#include <cstddef>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

struct GameResources;

/// Populate a generated level and return its random monster count.
[[nodiscard]] std::size_t populateGeneratedLevel(entt::registry& registry,
                                                 GameResources& resources,
                                                 const Map& map,
                                                 const MapBuildResult& build);

[[nodiscard]] entt::entity spawnPlayer(entt::registry& registry,
                                       Point position);
[[nodiscard]] std::size_t
spawnRequestedEntities(entt::registry& registry,
                       const std::vector<SpawnRequest>& requests,
                       const Map& map,
                       Point playerStart,
                       Point stairsPosition);

} // namespace rl
