#include "spawn/ItemSpawner.hpp"

#include <algorithm>
#include <string>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/SpawnTable.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool
canSpawnAt(const entt::registry& registry, Point point, Point playerStart)
{
    const auto tileContent = tileContentAt(registry, point);
    return point != playerStart && !tileContent.blocker.has_value() &&
           !tileContent.item.has_value();
}

[[nodiscard]] bool canSpawnAt(const entt::registry& registry,
                              const Map& map,
                              Point point,
                              Point playerStart)
{
    return map.isWalkable(point.x, point.y) &&
           canSpawnAt(registry, point, playerStart);
}

} // namespace

entt::entity
spawnHealingPotion(entt::registry& registry, Point position, int amount)
{
    const auto entity = spawnNamedEntity(
        registry, defaultRawDatabase(), "healing_potion", position);
    registry.replace<ProvidesHealing>(entity, ProvidesHealing{amount});
    return entity;
}

entt::entity spawnFoodRation(entt::registry& registry, Point position)
{
    return spawnNamedEntity(
        registry, defaultRawDatabase(), "food_ration", position);
}

entt::entity spawnFireballScroll(entt::registry& registry, Point position)
{
    return spawnNamedEntity(
        registry, defaultRawDatabase(), "fireball_scroll", position);
}

entt::entity spawnMagicMappingScroll(entt::registry& registry, Point position)
{
    return spawnNamedEntity(
        registry, defaultRawDatabase(), "magic_mapping_scroll", position);
}

entt::entity spawnDagger(entt::registry& registry, Point position)
{
    return spawnNamedEntity(registry, defaultRawDatabase(), "dagger", position);
}

entt::entity spawnSword(entt::registry& registry, Point position)
{
    return spawnNamedEntity(registry, defaultRawDatabase(), "sword", position);
}

entt::entity spawnChainMail(entt::registry& registry, Point position)
{
    return spawnNamedEntity(
        registry, defaultRawDatabase(), "chain_mail", position);
}

std::size_t spawnItems(entt::registry& registry,
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
    const auto maximum = static_cast<std::size_t>(maxItemsForDepth(depth));
    for (std::size_t regionIndex = 1;
         regionIndex < regions.size() && spawned < maximum;
         ++regionIndex) {
        const bool hallway =
            regions[regionIndex].kind == SpawnRegionKind::Corridor;
        const bool forceEarlyItems = spawned < 2 && !hallway;
        if (!forceEarlyItems && hallway && rng.rangeInclusive(0, 8) != 0) {
            continue;
        }
        if (!forceEarlyItems && !hallway && rng.rangeInclusive(0, 3) != 0) {
            continue;
        }

        constexpr int maxAttempts = 12;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            const auto& tiles = regions[regionIndex].tiles;
            if (tiles.empty()) {
                break;
            }
            const Point position = tiles[static_cast<std::size_t>(
                rng.rangeInclusive(0, static_cast<int>(tiles.size()) - 1))];
            if (!canSpawnAt(registry, map, position, playerStart)) {
                continue;
            }

            const auto id =
                forceEarlyItems
                    ? (spawned == 0 ? std::string{"healing_potion"}
                                    : std::string{"food_ration"})
                    : rollSpawn(
                          defaultRawDatabase().itemSpawnTable(), depth, rng);
            [[maybe_unused]] const auto item =
                spawnNamedEntity(registry, defaultRawDatabase(), id, position);
            ++spawned;
            break;
        }
    }

    return spawned;
}

} // namespace rl
