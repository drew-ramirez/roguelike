#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "spawn/Spawner.hpp"
#include "systems/PortalSystem.hpp"

namespace rl::test {

void runPortalTests(TestRunner& runner)
{
    auto map = openMap(40, 25);
    entt::registry registry;
    GameResources resources;
    ShadowcastingFov fov;
    resources.depth = 2;
    resources.mapBuilderId = "normal";
    resources.rng.reseed(42U);
    resources.player = spawnPlayer(registry, Point{6, 6});

    runner.expect(
        tryUseTownPortal(registry, resources, map, fov, resources.player),
        "town portal moves player back to town");
    runner.expect(resources.depth == 1 && resources.townPortalActive,
                  "portal records active town return state");
    runner.expect(resources.townPortalReturnDepth == 2 &&
                      resources.townPortalReturnPosition == Point{6, 6},
                  "portal records return depth and position");
    runner.expect(resources.townPortalPosition.has_value(),
                  "portal records town portal position");

    runner.expect(tryUseReturnPortal(registry, resources, map, fov),
                  "standing on town portal returns to dungeon");
    runner.expect(resources.depth == 2 && !resources.townPortalActive,
                  "return portal restores depth and clears active portal");
    const auto& position = registry.get<Position>(resources.player);
    runner.expect(position.x == 6 && position.y == 6,
                  "return portal restores original position");

    auto blockedMap = openMap(40, 25);
    entt::registry blockedRegistry;
    GameResources blockedResources;
    ShadowcastingFov blockedFov;
    blockedResources.depth = 2;
    blockedResources.mapBuilderId = "normal";
    blockedResources.rng.reseed(43U);
    blockedResources.player = spawnPlayer(blockedRegistry, Point{6, 6});
    blockedResources.progress.artifactClaimed = true;
    runner.expect(!tryUseTownPortal(blockedRegistry,
                                    blockedResources,
                                    blockedMap,
                                    blockedFov,
                                    blockedResources.player),
                  "artifact claim blocks town portal use");
}

} // namespace rl::test
