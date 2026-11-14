#include "spawn/MonsterSpawner.hpp"

#include <algorithm>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/SpawnTable.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

constexpr int forcedEarlyMonsters = 2;

[[nodiscard]] bool trySpawnMonsterInRegion(entt::registry& registry,
                                           const SpawnRegion& region,
                                           const Map& map,
                                           Rng& rng,
                                           Point playerStart,
                                           int depth)
{
    if (region.tiles.empty()) {
        return false;
    }

    constexpr int maxAttempts = 20;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        const Point position = region.tiles[static_cast<std::size_t>(
            rng.rangeInclusive(0, static_cast<int>(region.tiles.size()) - 1))];
        if (position == playerStart ||
            !map.isWalkable(position.x, position.y) ||
            tileContentAt(registry, position).blocker.has_value()) {
            continue;
        }

        const auto id =
            rollSpawn(defaultRawDatabase().monsterSpawnTable(), depth, rng);
        [[maybe_unused]] const auto monster =
            spawnNamedEntity(registry, defaultRawDatabase(), id, position);
        return true;
    }

    return false;
}

} // namespace

entt::entity spawnGoblin(entt::registry& registry, Point position)
{
    return spawnNamedEntity(registry, defaultRawDatabase(), "goblin", position);
}

entt::entity spawnOrc(entt::registry& registry, Point position)
{
    return spawnNamedEntity(registry, defaultRawDatabase(), "orc", position);
}

entt::entity spawnTroll(entt::registry& registry, Point position)
{
    return spawnNamedEntity(registry, defaultRawDatabase(), "troll", position);
}

std::size_t spawnMonsters(entt::registry& registry,
                          const std::vector<SpawnRegion>& regions,
                          const Map& map,
                          Rng& rng,
                          Point playerStart,
                          int depth)
{
    if (regions.size() <= 1) {
        return 0;
    }

    std::size_t spawned = 0;
    for (std::size_t regionIndex = 1;
         regionIndex < regions.size() &&
         spawned < static_cast<std::size_t>(maxMonstersForDepth(depth));
         ++regionIndex) {
        const bool forceSpawn = spawned < forcedEarlyMonsters;
        const bool hallway =
            regions[regionIndex].kind == SpawnRegionKind::Corridor;
        if (!forceSpawn && hallway && rng.rangeInclusive(0, 5) != 0) {
            continue;
        }
        if (!forceSpawn && !hallway && rng.rangeInclusive(0, 2) == 0) {
            continue;
        }

        if (trySpawnMonsterInRegion(
                registry, regions[regionIndex], map, rng, playerStart, depth)) {
            ++spawned;
        }
    }

    return spawned;
}

} // namespace rl
