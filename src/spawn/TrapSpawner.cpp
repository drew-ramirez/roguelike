/**
 * **************************************************************************
 * @file     TrapSpawner.cpp
 * @brief    Places low-damage hidden spike traps on valid floor tiles
 * **************************************************************************
 */

#include "spawn/TrapSpawner.hpp"

#include <algorithm>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool canSpawnTrapAt(const entt::registry& registry,
                                  Point point,
                                  Point playerStart,
                                  Point stairsPosition)
{
    const auto tileContent = tileContentAt(registry, point);
    return point != playerStart && point != stairsPosition &&
           !tileContent.blocker.has_value() && !tileContent.item.has_value() &&
           !tileContent.trap.has_value();
}

[[nodiscard]] bool canSpawnTrapAt(const entt::registry& registry,
                                  const Map& map,
                                  Point point,
                                  Point playerStart,
                                  Point stairsPosition)
{
    return map.isWalkable(point.x, point.y) &&
           canSpawnTrapAt(registry, point, playerStart, stairsPosition);
}

} // namespace

entt::entity spawnSpikeTrap(entt::registry& registry,
                            Point position,
                            bool hidden,
                            bool triggered)
{
    const auto entity = spawnNamedEntity(registry,
                                         defaultRawDatabase(),
                                         "spike_trap",
                                         position,
                                         RawSpawnOptions{hidden, triggered});
    registry.emplace_or_replace<Trap>(entity, Trap{3, triggered});
    if ((!hidden || triggered) && registry.all_of<Hidden>(entity)) {
        registry.remove<Hidden>(entity);
    }
    return entity;
}

std::size_t spawnTraps(entt::registry& registry,
                       const std::vector<SpawnRegion>& regions,
                       const Map& map,
                       Rng& rng,
                       Point playerStart,
                       Point stairsPosition,
                       int depth)
{
    if (regions.size() <= 2) {
        return 0;
    }

    const auto maximum = static_cast<std::size_t>(std::clamp(depth + 1, 1, 6));
    std::size_t spawned = 0;
    for (std::size_t regionIndex = 1;
         regionIndex < regions.size() && spawned < maximum;
         ++regionIndex) {
        if (rng.rangeInclusive(0, 5) != 0) {
            continue;
        }

        constexpr int maxAttempts = 10;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            const auto& tiles = regions[regionIndex].tiles;
            if (tiles.empty()) {
                break;
            }
            const Point position = tiles[static_cast<std::size_t>(
                rng.rangeInclusive(0, static_cast<int>(tiles.size()) - 1))];
            if (!canSpawnTrapAt(
                    registry, map, position, playerStart, stairsPosition)) {
                continue;
            }

            [[maybe_unused]] const auto trap =
                spawnSpikeTrap(registry, position);
            ++spawned;
            break;
        }
    }
    return spawned;
}

} // namespace rl
