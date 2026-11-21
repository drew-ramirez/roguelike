#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/combat/RangedCombatSystem.hpp"
#include "systems/inventory/EquipmentSystem.hpp"

namespace rl::test {

void runRangedCombatTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;
    resources.rng.reseed(123U);
    resources.player = spawnPlayer(registry, Point{2, 2});

    const auto bow = spawnNamedEntity(
        registry, defaultRawDatabase(), "shortbow", Point{1, 1});
    registry.remove<Position>(bow);
    registry.emplace<InBackpack>(bow, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, bow),
        "ranged weapon equips");
    const auto goblin = spawnGoblin(registry, Point{5, 2});

    registry.emplace<WantsToShoot>(resources.player,
                                   WantsToShoot{bow, Point{5, 2}});
    runner.expect(processRangedCombat(registry, resources, map),
                  "ranged combat resolves a clear shot");
    runner.expect(registry.all_of<SufferDamage>(goblin),
                  "ranged attack queues target damage");
    runner.expect(registry.valid(bow) && registry.all_of<Equipped>(bow) &&
                      registry.get<Equipped>(bow).owner == resources.player,
                  "ranged attack keeps the bow equipped");
}

} // namespace rl::test
