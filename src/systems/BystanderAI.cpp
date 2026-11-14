/**
 * **************************************************************************
 * @file     BystanderAI.cpp
 * @brief    Moves non-vendor bystanders with simple random wandering
 * **************************************************************************
 */

#include "systems/BystanderAI.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "systems/InputSystem.hpp"

namespace rl {

void runBystanderAi(entt::registry& registry,
                    GameResources& resources,
                    const Map& map)
{
    const auto bystanders = registry.view<const Bystander, Position>();
    for (const auto bystander : bystanders) {
        if (registry.all_of<Vendor>(bystander) ||
            registry.all_of<SettlementService>(bystander)) {
            continue;
        }

        const Point step{resources.rng.rangeInclusive(-1, 1),
                         resources.rng.rangeInclusive(-1, 1)};
        if (step.x != 0 || step.y != 0) {
            [[maybe_unused]] const bool moved =
                tryMoveEntity(registry, bystander, map, step);
        }
    }
}

} // namespace rl
