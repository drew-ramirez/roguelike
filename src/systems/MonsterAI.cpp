/**
 * **************************************************************************
 * @file     MonsterAI.cpp
 * @brief    Moves visible monsters and queues adjacent melee attacks
 * **************************************************************************
 */

#include "systems/MonsterAI.hpp"

#include <algorithm>
#include <cstdlib>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "systems/InputSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"

namespace rl {
namespace {

int sign(int value) noexcept
{
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

Point chooseMonsterStep(Point monsterPosition,
                        Point playerPosition,
                        const Map& map,
                        const entt::registry& registry,
                        entt::entity monster)
{
    const int deltaX = playerPosition.x - monsterPosition.x;
    const int deltaY = playerPosition.y - monsterPosition.y;
    const Point primary = std::abs(deltaX) >= std::abs(deltaY)
                              ? Point{sign(deltaX), 0}
                              : Point{0, sign(deltaY)};
    const Point secondary =
        primary.x != 0 ? Point{0, sign(deltaY)} : Point{sign(deltaX), 0};

    // Try the longer axis first, then slide along the other axis if blocked
    const Point primaryTarget = monsterPosition + primary;
    if ((primary.x != 0 || primary.y != 0) &&
        !isBlocked(map, registry, primaryTarget, monster)) {
        return primary;
    }

    const Point secondaryTarget = monsterPosition + secondary;
    if ((secondary.x != 0 || secondary.y != 0) &&
        !isBlocked(map, registry, secondaryTarget, monster)) {
        return secondary;
    }

    return Point{};
}

} // namespace

void runMonsterAi(entt::registry& registry,
                  const GameResources& resources,
                  const Map& map)
{
    if (resources.player == entt::null || !registry.valid(resources.player) ||
        !registry.all_of<Position>(resources.player)) {
        return;
    }

    const auto& playerPositionComponent =
        registry.get<Position>(resources.player);
    const Point playerPosition{playerPositionComponent.x,
                               playerPositionComponent.y};

    const auto view = registry.view<const Monster, Position>();
    for (const auto monster : view) {
        auto& monsterPositionComponent = view.get<Position>(monster);
        const Point monsterPosition{monsterPositionComponent.x,
                                    monsterPositionComponent.y};
        const int distance = std::abs(playerPosition.x - monsterPosition.x) +
                             std::abs(playerPosition.y - monsterPosition.y);

        // Adjacent monsters attack, visible ones chase with a greedy step
        if (distance == 1) {
            registry.emplace_or_replace<WantsToMelee>(
                monster, WantsToMelee{resources.player});
            continue;
        }

        if (!map.isVisible(monsterPosition.x, monsterPosition.y)) {
            continue;
        }

        const Point step = chooseMonsterStep(
            monsterPosition, playerPosition, map, registry, monster);
        if (step.x != 0 || step.y != 0) {
            [[maybe_unused]] const bool moved =
                tryMoveEntity(registry, monster, map, step);
        }
    }
}

} // namespace rl
