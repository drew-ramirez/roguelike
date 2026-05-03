#include "spawn/MonsterSpawner.hpp"

#include <algorithm>

#include "ecs/Components.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

constexpr int maxMonsters = 8;
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
                           Point playerStart)
{
    constexpr int maxAttempts = 20;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        const Point position = randomPointInRoom(room, rng);
        if (position == playerStart ||
            blockingEntityAt(registry, position).has_value()) {
            continue;
        }

        if (rng.coinFlip()) {
            [[maybe_unused]] const auto monster =
                spawnGoblin(registry, position);
        } else {
            [[maybe_unused]] const auto monster = spawnOrc(registry, position);
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

std::size_t spawnMonsters(entt::registry& registry,
                          const std::vector<Rect>& rooms,
                          Rng& rng,
                          Point playerStart)
{
    if (rooms.size() <= 1) {
        return 0;
    }

    std::size_t spawned = 0;
    for (std::size_t roomIndex = 1;
         roomIndex < rooms.size() && spawned < maxMonsters;
         ++roomIndex) {
        const bool forceSpawn = spawned < forcedEarlyMonsters;
        if (!forceSpawn && rng.rangeInclusive(0, 2) == 0) {
            continue;
        }

        if (trySpawnMonsterInRoom(
                registry, rooms[roomIndex], rng, playerStart)) {
            ++spawned;
        }
    }

    return spawned;
}

} // namespace rl
