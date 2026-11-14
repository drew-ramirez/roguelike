#include "TestSuites.hpp"

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/SystemScheduler.hpp"

namespace rl::test {

void runSchedulerTests(TestRunner& runner)
{
    {
        auto map = openMap();
        revealAll(map);
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        registry.emplace<KnownSpells>(resources.player, KnownSpells{{"spark"}});
        registry.get<Pools>(resources.player).mana = 6;
        const auto monster = spawnGoblin(registry, Point{3, 2});
        registry.get<CombatStats>(monster).hp = 1;
        const int playerHp = registry.get<CombatStats>(resources.player).hp;
        registry.emplace<WantsToCastSpell>(
            resources.player, WantsToCastSpell{"spark", Point{3, 2}});
        ShadowcastingFov fov;
        RunState state = RunState::AwaitingInput;
        resolveTurnSystems(registry, resources, map, fov, state, true);
        runner.expect(!registry.valid(monster) &&
                          registry.get<CombatStats>(resources.player).hp ==
                              playerHp,
                      "player spell defeats adjacent monster before its turn");
        runner.expect(state == RunState::AwaitingInput,
                      "completed turn returns to input");
    }
    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto monster = spawnGoblin(registry, Point{3, 2});
        registry.emplace<Confusion>(monster, Confusion{3});
        ShadowcastingFov fov;
        RunState state = RunState::AwaitingInput;
        resolveTurnSystems(registry, resources, map, fov, state, false);
        runner.expect(registry.get<Confusion>(monster).turns == 3,
                      "turn can skip monster AI");
        resolveTurnSystems(registry, resources, map, fov, state, true);
        runner.expect(registry.get<Confusion>(monster).turns == 2,
                      "enabled monster AI takes its turn");
        queueDamage(registry,
                    resources.player,
                    registry.get<CombatStats>(resources.player).hp);
        resolveTurnSystems(registry, resources, map, fov, state, true);
        runner.expect(state == RunState::GameOver &&
                          registry.get<Confusion>(monster).turns == 2,
                      "player death prevents the monster turn");
    }
}

} // namespace rl::test
