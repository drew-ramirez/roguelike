#include "TestSuites.hpp"

#include <algorithm>
#include <string>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/SpawnTable.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/MapTransitionSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] int countNamed(const entt::registry& registry,
                             const std::string& name)
{
    int count = 0;
    const auto view = registry.view<const Name>();
    for (const auto entity : view) {
        if (view.get<const Name>(entity).value == name) {
            ++count;
        }
    }
    return count;
}

} // namespace

void runDescentAndSpawnTests(TestRunner& runner)
{
    Rng rng{12345U};
    auto build = SimpleMapBuilder::build(40, 25, rng);
    entt::registry registry;
    GameResources resources;
    resources.rng.reseed(9876U);
    resources.player = spawnPlayer(registry, build.playerStart);
    auto& playerPosition = registry.get<Position>(resources.player);
    const auto stairs = findTile(build.map, TileType::DownStairs);
    runner.expect(stairs.has_value(), "generated maps include stairs");
    playerPosition.x = stairs->x;
    playerPosition.y = stairs->y;

    const auto carried = spawnHealingPotion(registry, Point{1, 1});
    registry.remove<Position>(carried);
    registry.emplace<InBackpack>(carried, InBackpack{resources.player});
    const auto equipped = spawnDagger(registry, Point{1, 2});
    registry.remove<Position>(equipped);
    registry.emplace<InBackpack>(equipped, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, equipped),
        "weapon equips before descent");
    const auto localMonster = spawnGoblin(registry, Point{2, 2});
    const auto localItem = spawnHealingPotion(registry, Point{3, 2});

    ShadowcastingFov fov;
    runner.expect(tryDescendToNextLevel(registry, resources, build.map, fov),
                  "player descends while standing on stairs");
    runner.expect(resources.depth == 2, "descent increments depth");
    runner.expect(registry.valid(resources.player), "descent preserves player");
    runner.expect(registry.valid(carried) && registry.valid(equipped),
                  "descent preserves carried and equipped items");
    runner.expect(!registry.valid(localMonster) && !registry.valid(localItem),
                  "descent removes level-local entities");
    runner.expect(findTile(build.map, TileType::DownStairs).has_value(),
                  "new level also has stairs");
    runner.expect(countNamed(registry, "Wolf") == 1 &&
                      countNamed(registry, "Wild Boar") == 1,
                  "descent into woods spawns authored wildlife");
    runner.expect(countNamed(registry, "Food Ration") >= 1,
                  "descent into woods spawns a supply pickup");

    Rng first{100U};
    Rng second{100U};
    const auto firstRoll = rollSpawn(monsterSpawnTable(), 1, first);
    const auto secondRoll = rollSpawn(monsterSpawnTable(), 1, second);
    runner.expect(firstRoll == secondRoll,
                  "spawn tables are deterministic for a fixed seed");
    runner.expect(
        std::any_of(monsterSpawnTable().begin(),
                    monsterSpawnTable().end(),
                    [](const SpawnEntry& entry) { return entry.minDepth > 1; }),
        "spawn table has depth-gated entries");
}

} // namespace rl::test
