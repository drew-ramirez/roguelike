/**
 * **************************************************************************
 * @file     TrapSpawner.cpp
 * @brief    Places low-damage hidden spike traps on valid floor tiles
 * **************************************************************************
 */

#include "spawn/TrapSpawner.hpp"

#include <algorithm>

#include "ecs/Components.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

Point randomPointInRoom(const Rect& room, Rng& rng)
{
    return Point{
        rng.rangeInclusive(room.x1, room.x2 - 1),
        rng.rangeInclusive(room.y1, room.y2 - 1),
    };
}

[[nodiscard]] bool trapAtPosition(const entt::registry& registry, Point point)
{
    const auto view = registry.view<const Trap, const Position>();
    for (const auto entity : view) {
        const auto& position = view.get<const Position>(entity);
        if (position.x == point.x && position.y == point.y) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool canSpawnTrapAt(const entt::registry& registry,
                                  Point point,
                                  Point playerStart,
                                  Point stairsPosition)
{
    return point != playerStart && point != stairsPosition &&
           !blockingEntityAt(registry, point).has_value() &&
           !itemAtPosition(registry, point).has_value() &&
           !trapAtPosition(registry, point);
}

} // namespace

entt::entity spawnSpikeTrap(entt::registry& registry,
                            Point position,
                            bool hidden,
                            bool triggered)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    registry.emplace<Renderable>(
        entity, Renderable{U'^', Color{220, 90, 80, 255}, Color::black(), 1});
    registry.emplace<Name>(entity, Name{"Spike Trap"});
    registry.emplace<Trap>(entity, Trap{3, triggered});
    if (hidden && !triggered) {
        registry.emplace<Hidden>(entity);
    }
    return entity;
}

std::size_t spawnTraps(entt::registry& registry,
                       const std::vector<Rect>& rooms,
                       Rng& rng,
                       Point playerStart,
                       Point stairsPosition,
                       int depth)
{
    if (rooms.size() <= 2) {
        return 0;
    }

    const auto maximum = static_cast<std::size_t>(std::clamp(depth + 1, 1, 6));
    std::size_t spawned = 0;
    for (std::size_t roomIndex = 1;
         roomIndex < rooms.size() && spawned < maximum;
         ++roomIndex) {
        if (rng.rangeInclusive(0, 4) != 0) {
            continue;
        }

        constexpr int maxAttempts = 10;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            const Point position = randomPointInRoom(rooms[roomIndex], rng);
            if (!canSpawnTrapAt(
                    registry, position, playerStart, stairsPosition)) {
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
