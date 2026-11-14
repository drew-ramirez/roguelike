#include "TestSuites.hpp"

#include <algorithm>
#include <iterator>
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
#include "systems/VendorSystem.hpp"

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

    {
        Rng buildRng{606U};
        auto townBuild = SimpleMapBuilder::build(40, 25, buildRng);
        const auto& tiles = townBuild.spawnRegions[1].tiles;
        const auto vendorPoint =
            *std::find_if(tiles.begin(), tiles.end(), [&](Point point) {
                return point != townBuild.playerStart &&
                       point != townBuild.stairsPosition &&
                       townBuild.map.isWalkable(point.x, point.y);
            });
        townBuild.spawnRequests.push_back({"blacksmith", vendorPoint});
        entt::registry townRegistry;
        GameResources townResources;
        townResources.rng.reseed(42U);
        Rng untouchedRng{42U};
        townResources.player = spawnPlayer(townRegistry, townBuild.playerStart);
        const auto monsters = populateGeneratedLevel(
            townRegistry, townResources, townBuild.map, townBuild);
        const auto vendors = townRegistry.view<const Vendor>();
        bool stocked = false;
        for (const auto vendor : vendors) {
            stocked = stocked || !vendorStock(townRegistry, vendor).empty();
        }
        runner.expect(stocked && countNamed(townRegistry, "Blacksmith") == 1,
                      "town population creates and stocks authored vendors");
        runner.expect(
            monsters == 0 && townRegistry.view<const Monster>().empty() &&
                townRegistry.view<const Trap>().empty() &&
                townRegistry.view<const Item, const Position>().begin() ==
                    townRegistry.view<const Item, const Position>().end(),
            "town population skips random level entities");
        runner.expect(townResources.rng.rangeInclusive(0, 100000) ==
                          untouchedRng.rangeInclusive(0, 100000),
                      "town population does not consume random spawn rolls");
    }

    {
        Rng buildRng{12345U};
        auto dungeonBuild = SimpleMapBuilder::build(40, 25, buildRng);
        entt::registry dungeonRegistry;
        GameResources dungeonResources;
        dungeonResources.mapBuilderId = "simple";
        dungeonResources.rng.reseed(9876U);
        dungeonResources.player =
            spawnPlayer(dungeonRegistry, dungeonBuild.playerStart);
        const auto monsters = populateGeneratedLevel(
            dungeonRegistry, dungeonResources, dungeonBuild.map, dungeonBuild);
        const auto view = dungeonRegistry.view<const Monster>();
        const auto actual =
            static_cast<std::size_t>(std::distance(view.begin(), view.end()));
        runner.expect(monsters >= 2 && monsters == actual,
                      "dungeon population reports its spawned monster count");
    }

    Rng itemRng{2222U};
    const auto itemBuild = SimpleMapBuilder::build(40, 25, itemRng);
    entt::registry itemRegistry;
    const auto itemPlayer = spawnPlayer(itemRegistry, itemBuild.playerStart);
    const auto itemCount = spawnItems(itemRegistry,
                                      itemBuild.spawnRegions,
                                      itemBuild.map,
                                      itemRng,
                                      itemBuild.playerStart,
                                      1);
    runner.expect(itemCount >= 2 &&
                      countNamed(itemRegistry, "Healing Potion") >= 1 &&
                      countNamed(itemRegistry, "Food Ration") >= 1,
                  "early item spawning guarantees healing and food");
    runner.expect(itemRegistry.valid(itemPlayer),
                  "supply spawn keeps the player entity valid");
}

} // namespace rl::test
