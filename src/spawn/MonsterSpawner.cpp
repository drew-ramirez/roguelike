#include "spawn/MonsterSpawner.hpp"

#include <algorithm>

#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "spawn/SpawnTable.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

constexpr int forcedEarlyMonsters = 2;

Point randomPointInRoom(const Rect& room, Rng& rng)
{
    return Point{
        rng.rangeInclusive(room.x1, room.x2 - 1),
        rng.rangeInclusive(room.y1, room.y2 - 1),
    };
}

bool trySpawnMonsterInRoom(entt::registry& registry,
                           const Rect& room,
                           Rng& rng,
                           Point playerStart,
                           int depth)
{
    constexpr int maxAttempts = 20;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        const Point position = randomPointInRoom(room, rng);
        if (position == playerStart ||
            blockingEntityAt(registry, position).has_value()) {
            continue;
        }

        const auto id = rollSpawn(monsterSpawnTable(), depth, rng);
        if (id == "orc") {
            [[maybe_unused]] const auto monster = spawnOrc(registry, position);
        } else if (id == "troll") {
            [[maybe_unused]] const auto monster =
                spawnTroll(registry, position);
        } else {
            [[maybe_unused]] const auto monster =
                spawnGoblin(registry, position);
        }
        return true;
    }

    return false;
}

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
            blockingEntityAt(registry, position).has_value()) {
            continue;
        }

        const auto id = rollSpawn(monsterSpawnTable(), depth, rng);
        if (id == "orc") {
            [[maybe_unused]] const auto monster = spawnOrc(registry, position);
        } else if (id == "troll") {
            [[maybe_unused]] const auto monster =
                spawnTroll(registry, position);
        } else {
            [[maybe_unused]] const auto monster =
                spawnGoblin(registry, position);
        }
        return true;
    }

    return false;
}

} // namespace

entt::entity spawnGoblin(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'g', Color{110, 220, 110, 255}, Color::black(), 5});
    registry.emplace<Name>(entity, Name{"Goblin"});
    registry.emplace<Monster>(entity);
    registry.emplace<BlocksTile>(entity);
    registry.emplace<CombatStats>(entity, CombatStats{8, 8, 0, 3});
    return entity;
}

entt::entity spawnOrc(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'o', Color{80, 180, 80, 255}, Color::black(), 5});
    registry.emplace<Name>(entity, Name{"Orc"});
    registry.emplace<Monster>(entity);
    registry.emplace<BlocksTile>(entity);
    registry.emplace<CombatStats>(entity, CombatStats{12, 12, 1, 4});
    return entity;
}

entt::entity spawnTroll(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'T', Color{90, 160, 90, 255}, Color::black(), 5});
    registry.emplace<Name>(entity, Name{"Troll"});
    registry.emplace<Monster>(entity);
    registry.emplace<BlocksTile>(entity);
    registry.emplace<CombatStats>(entity, CombatStats{20, 20, 2, 6});
    return entity;
}

std::size_t spawnMonsters(entt::registry& registry,
                          const std::vector<Rect>& rooms,
                          Rng& rng,
                          Point playerStart,
                          int depth)
{
    if (rooms.size() <= 1) {
        return 0;
    }

    std::size_t spawned = 0;
    for (std::size_t roomIndex = 1;
         roomIndex < rooms.size() &&
         spawned < static_cast<std::size_t>(maxMonstersForDepth(depth));
         ++roomIndex) {
        const bool forceSpawn = spawned < forcedEarlyMonsters;
        if (!forceSpawn && rng.rangeInclusive(0, 2) == 0) {
            continue;
        }

        if (trySpawnMonsterInRoom(
                registry, rooms[roomIndex], rng, playerStart, depth)) {
            ++spawned;
        }
    }

    return spawned;
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
