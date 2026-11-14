/**
 * **************************************************************************
 * @file     PortalSystem.cpp
 * @brief    Implements town portal travel using existing level transitions
 * **************************************************************************
 */

#include "systems/travel/PortalSystem.hpp"

#include <optional>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "systems/spatial/VisibilitySystem.hpp"
#include "systems/travel/MapTransitionSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] bool hasPortalAt(const entt::registry& registry, Point point)
{
    const auto view = registry.view<const Position, const TownPortal>();
    for (const auto entity : view) {
        if (!registry.all_of<Prop>(entity)) {
            continue;
        }
        const auto& position = view.get<const Position>(entity);
        if (position.x == point.x && position.y == point.y) {
            return true;
        }
    }
    return false;
}

void ensurePortalAt(entt::registry& registry, Point point)
{
    if (hasPortalAt(registry, point)) {
        return;
    }

    const auto portal =
        spawnRawEntity(registry, defaultRawDatabase(), "town_portal", point);
    if (portal.has_value()) {
        registry.emplace_or_replace<TownPortal>(*portal);
    }
}

[[nodiscard]] bool artifactClaimed(const GameResources& resources)
{
    return resources.progress.artifactClaimed;
}

} // namespace

bool playerIsOnTownPortal(const entt::registry& registry,
                          const GameResources& resources)
{
    if (!resources.townPortalActive ||
        !resources.townPortalPosition.has_value()) {
        return false;
    }
    const auto playerPosition = entityPosition(registry, resources.player);
    return playerPosition.has_value() &&
           *playerPosition == *resources.townPortalPosition;
}

bool tryUseTownPortal(entt::registry& registry,
                      GameResources& resources,
                      Map& map,
                      IFovProvider& fovProvider,
                      entt::entity user)
{
    const auto returnPosition = entityPosition(registry, user);
    if (!returnPosition.has_value()) {
        return false;
    }
    if (artifactClaimed(resources)) {
        resources.log.add("The claimed artifact prevents portal travel.");
        return false;
    }
    if (resources.depth == 1) {
        resources.log.add("The portal magic has nowhere to take you.");
        return false;
    }

    ensurePortalAt(registry, *returnPosition);
    const int returnDepth = resources.depth;
    resources.townPortalActive = true;
    resources.townPortalReturnDepth = returnDepth;
    resources.townPortalReturnPosition = *returnPosition;

    if (!tryChangeLevel(registry,
                        resources,
                        map,
                        fovProvider,
                        1,
                        LevelArrivalMode::PortalReturn)) {
        resources.townPortalActive = false;
        resources.townPortalReturnPosition.reset();
        return false;
    }

    const auto arrival = entityPosition(registry, resources.player);
    const Point townPortal =
        resources.townPortalPosition.value_or(arrival.value_or(Point{1, 1}));
    resources.townPortalPosition = townPortal;
    ensurePortalAt(registry, townPortal);
    moveEntityTo(registry, resources.player, townPortal);
    updateVisibility(registry, map, resources.player, fovProvider);
    resources.log.add("A town portal opens. Press Enter on it to return.");
    return true;
}

bool tryUseReturnPortal(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider)
{
    if (artifactClaimed(resources)) {
        resources.log.add("The claimed artifact prevents portal travel.");
        return false;
    }
    if (!resources.townPortalActive ||
        !resources.townPortalReturnPosition.has_value()) {
        resources.log.add("There is no active town portal.");
        return false;
    }
    if (resources.depth != 1 || !playerIsOnTownPortal(registry, resources)) {
        resources.log.add("You are not standing on the town portal.");
        return false;
    }

    const int returnDepth = resources.townPortalReturnDepth;
    const Point returnPosition = *resources.townPortalReturnPosition;
    if (!tryChangeLevel(registry,
                        resources,
                        map,
                        fovProvider,
                        returnDepth,
                        LevelArrivalMode::PortalReturn)) {
        return false;
    }

    moveEntityTo(registry, resources.player, returnPosition);
    resources.townPortalActive = false;
    resources.townPortalReturnPosition.reset();
    updateVisibility(registry, map, resources.player, fovProvider);
    resources.log.add("The portal collapses behind you.");
    return true;
}

} // namespace rl
