#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/MonsterAI.hpp"
#include "systems/RangedCombatSystem.hpp"

namespace rl::test {

void runDragonTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    const auto dragon =
        spawnNamedEntity(registry, defaultRawDatabase(), "dragon", Point{5, 2});

    runHostileMonsterAi(registry, resources, map);
    runner.expect(registry.all_of<WantsToShoot>(dragon),
                  "dragon AI queues breath at range");
    runner.expect(processRangedCombat(registry, resources, map),
                  "dragon breath resolves as ranged damage");
    runner.expect(registry.all_of<SufferDamage>(resources.player),
                  "dragon breath damages the player");

    entt::registry artifactRegistry;
    GameResources artifactResources;
    artifactResources.player = spawnPlayer(artifactRegistry, Point{2, 2});
    const auto artifact = artifactRegistry.create();
    artifactRegistry.emplace<Position>(artifact, Position{3, 2});
    artifactRegistry.emplace<Name>(artifact, Name{"Artifact Guardian"});
    artifactRegistry.emplace<CombatStats>(artifact, CombatStats{1, 1, 0, 1});
    artifactRegistry.emplace<GrantsWin>(artifact);
    queueDamage(artifactRegistry, artifact, 1);
    RunState state = RunState::AwaitingInput;
    applyDamageAndCleanup(artifactRegistry, artifactResources, state, &map);
    runner.expect(artifactResources.progress.artifactClaimed,
                  "defeating the artifact holder records progress");
    runner.expect(state == RunState::GameOver,
                  "claiming the artifact ends the run");
    runner.expect(artifactResources.log.counter("artifact_claimed") == 1,
                  "artifact claim increments the progress counter");
}

} // namespace rl::test
