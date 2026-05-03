#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"

namespace rl::test {

void runEquipmentTests(TestRunner& runner)
{
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    const auto dagger = spawnDagger(registry, Point{1, 1});
    const auto sword = spawnSword(registry, Point{1, 2});
    registry.remove<Position>(dagger);
    registry.remove<Position>(sword);
    registry.emplace<InBackpack>(dagger, InBackpack{resources.player});
    registry.emplace<InBackpack>(sword, InBackpack{resources.player});

    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, dagger),
        "first weapon equips");
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, sword),
        "second same-slot weapon equips");
    runner.expect(registry.all_of<InBackpack>(dagger) &&
                      registry.all_of<Equipped>(sword),
                  "same-slot equip swaps old item back to backpack");
    runner.expect(meleePowerWithEquipment(registry, resources.player) == 9,
                  "equipment power is computed from base plus equipped item");
    runner.expect(registry.get<CombatStats>(resources.player).power == 5,
                  "base CombatStats power is not mutated by equipment");

    const auto defender = spawnGoblin(registry, Point{3, 2});
    registry.get<CombatStats>(defender).hp = 20;
    registry.emplace_or_replace<WantsToMelee>(resources.player,
                                              WantsToMelee{defender});
    processMeleeCombat(registry, resources.log);
    RunState state = RunState::PlayerTurn;
    applyDamageAndCleanup(registry, resources, state);
    runner.expect(registry.get<CombatStats>(defender).hp == 11,
                  "melee combat uses computed equipment power");
}

} // namespace rl::test
