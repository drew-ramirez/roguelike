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
    runner.expect(registry.all_of<Attributes, Pools>(resources.player),
                  "player spawns with attributes and pools");
    runner.expect(registry.get<Pools>(resources.player).hitPoints == 30 &&
                      registry.get<Pools>(resources.player).mana == 6,
                  "player pools mirror starting health and mana");
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
    registry.get<CombatStats>(defender) = CombatStats{20, 20, 0, 3};
    registry.get<Pools>(defender) = Pools{20, 20, 0, 0, 1, 0, 0};
    registry.emplace_or_replace<WantsToMelee>(resources.player,
                                              WantsToMelee{defender});
    processMeleeCombat(registry, resources.log);
    RunState state = RunState::PlayerTurn;
    applyDamageAndCleanup(registry, resources, state);
    runner.expect(registry.get<CombatStats>(defender).hp == 11,
                  "melee combat uses computed equipment power");
    runner.expect(registry.get<Pools>(defender).hitPoints == 11,
                  "damage updates pools alongside combat HP");

    entt::registry statRegistry;
    GameResources statResources;
    statResources.rng.reseed(123U);
    statResources.player = spawnPlayer(statRegistry, Point{2, 2});
    statRegistry.emplace_or_replace<Attributes>(
        statResources.player, Attributes{14, 10, 10, 10});

    const auto trainingSword = statRegistry.create();
    statRegistry.emplace<Name>(trainingSword, Name{"Training Sword"});
    statRegistry.emplace<Item>(trainingSword);
    statRegistry.emplace<Equippable>(trainingSword,
                                     Equippable{EquipmentSlot::Weapon});
    statRegistry.emplace<WeaponStats>(
        trainingSword, WeaponStats{"2d1+2", 1, "might"});
    statRegistry.emplace<InBackpack>(
        trainingSword, InBackpack{statResources.player});
    runner.expect(equipOrUnequipItem(statRegistry,
                                     statResources.log,
                                     statResources.player,
                                     trainingSword),
                  "stat weapon equips before combat");

    const auto target = spawnGoblin(statRegistry, Point{3, 2});
    statRegistry.get<CombatStats>(target) = CombatStats{20, 20, 1, 3};
    statRegistry.get<Pools>(target) = Pools{20, 20, 0, 0, 1, 0, 0};
    statRegistry.get<CombatStats>(target).defense = 1;
    statRegistry.emplace_or_replace<WantsToMelee>(statResources.player,
                                                  WantsToMelee{target});

    processMeleeCombat(statRegistry, statResources.log, &statResources.rng);
    runner.expect(statRegistry.get<SufferDamage>(target).amount == 11,
                  "stat melee uses weapon dice hit bonus and attributes");
}

} // namespace rl::test
