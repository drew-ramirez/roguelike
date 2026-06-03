#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
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
    runner.expect(blockedResources.depth == 2 &&
                      !blockedResources.townPortalActive,
                  "blocked town portal leaves dungeon state unchanged");

    auto returnMap = openMap(40, 25);
    entt::registry returnRegistry;
    GameResources returnResources;
    ShadowcastingFov returnFov;
    returnResources.depth = 1;
    returnResources.mapBuilderId = "normal";
    returnResources.townPortalActive = true;
    returnResources.townPortalReturnDepth = 2;
    returnResources.townPortalReturnPosition = Point{6, 6};
    returnResources.townPortalPosition = Point{6, 6};
    returnResources.player = spawnPlayer(returnRegistry, Point{6, 6});
    returnResources.progress.artifactClaimed = true;
    runner.expect(!tryUseReturnPortal(
                      returnRegistry, returnResources, returnMap, returnFov),
                  "artifact claim blocks return portal use");
    runner.expect(returnResources.depth == 1 &&
                      returnResources.townPortalActive,
                  "blocked return portal leaves portal state unchanged");

    entt::registry victoryRegistry;
    GameResources victoryResources;
    victoryResources.player = spawnPlayer(victoryRegistry, Point{2, 2});
    const auto dragon = spawnNamedEntity(
        victoryRegistry, defaultRawDatabase(), "dragon", Point{5, 2});
    queueDamage(victoryRegistry, dragon, 200);
    RunState state = RunState::PlayerTurn;
    applyDamageAndCleanup(victoryRegistry, victoryResources, state);
    runner.expect(victoryResources.progress.artifactClaimed,
                  "dragon defeat claims artifact progress");
    runner.expect(state == RunState::GameOver,
                  "artifact victory still ends the run");
}

} // namespace rl::test
