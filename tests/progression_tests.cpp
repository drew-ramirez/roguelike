#include "TestSuites.hpp"

#include <algorithm>
#include <string_view>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "progression/ExperienceSystem.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/combat/DamageSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool recentLogContains(const GameResources& resources,
                                     std::string_view needle)
{
    const auto messages = resources.log.messages();
    return std::any_of(
        messages.begin(), messages.end(), [needle](std::string_view message) {
            return message.find(needle) != std::string_view::npos;
        });
}

} // namespace

void runProgressionTests(TestRunner& runner)
{
    runner.expect(xpThresholdForLevel(1) == 1000 &&
                      xpThresholdForLevel(2) == 2000,
                  "level thresholds scale by current level");

    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    const auto goblin = spawnGoblin(registry, Point{3, 2});

    queueDamage(registry, goblin, 100);
    RunState state = RunState::PlayerTurn;
    applyDamageAndCleanup(registry, resources, state);

    runner.expect(!registry.valid(goblin), "dead monster is removed");
    runner.expect(registry.get<Pools>(resources.player).xp == 100,
                  "killing rewarded monsters grants XP");
    runner.expect(recentLogContains(resources, "You gain 100 xp."),
                  "XP gain is logged");

    entt::registry levelRegistry;
    GameResources levelResources;
    levelResources.player = spawnPlayer(levelRegistry, Point{2, 2});
    auto& pools = levelRegistry.get<Pools>(levelResources.player);
    pools.xp = 950;
    const auto boar = spawnNamedEntity(
        levelRegistry, defaultRawDatabase(), "wild_boar", Point{3, 2});
    runner.expect(levelRegistry.valid(boar), "raw boar spawns for XP test");

    queueDamage(levelRegistry, boar, 100);
    RunState levelState = RunState::PlayerTurn;
    applyDamageAndCleanup(levelRegistry, levelResources, levelState);

    const auto& leveledPools = levelRegistry.get<Pools>(levelResources.player);
    const auto& leveledStats =
        levelRegistry.get<CombatStats>(levelResources.player);
    runner.expect(leveledPools.level == 2 && leveledPools.xp == 125,
                  "XP can advance levels and carry overflow");
    runner.expect(leveledPools.maxHitPoints == 35 &&
                      leveledPools.hitPoints == 35,
                  "level-up increases and refills player HP");
    runner.expect(leveledStats.maxHp == 35 && leveledStats.hp == 35,
                  "level-up keeps CombatStats compatible");
    runner.expect(recentLogContains(levelResources, "advance to level 2"),
                  "level-up feedback is logged");
}

} // namespace rl::test
