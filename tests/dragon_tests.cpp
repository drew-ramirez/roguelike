#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/ai/MonsterAI.hpp"
#include "systems/combat/RangedCombatSystem.hpp"

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
}

} // namespace rl::test
