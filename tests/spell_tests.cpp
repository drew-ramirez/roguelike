#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/combat/SpellSystem.hpp"

namespace rl::test {

void runSpellTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    registry.emplace<KnownSpells>(resources.player,
                                  KnownSpells{{"spark", "heal"}});
    registry.get<Pools>(resources.player).mana = 6;
    const auto goblin = spawnGoblin(registry, Point{4, 2});

    runner.expect(
        castKnownSpell(
            registry, resources, map, resources.player, "spark", Point{4, 2}),
        "known ranged spell casts at a visible target");
    runner.expect(registry.get<Pools>(resources.player).mana == 4,
                  "successful spell spends mana");
    runner.expect(registry.all_of<SufferDamage>(goblin),
                  "spell queues damage through effects");

    runner.expect(
        !castKnownSpell(
            registry, resources, map, resources.player, "spark", Point{10, 6}),
        "invalid spell target is rejected");
    runner.expect(registry.get<Pools>(resources.player).mana == 4,
                  "invalid target does not spend mana");
}

} // namespace rl::test
