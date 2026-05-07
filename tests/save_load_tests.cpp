#include "TestSuites.hpp"

#include <algorithm>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/ItemSystem.hpp"

namespace rl::test {

void runSaveLoadTests(TestRunner& runner)
{
    auto map = openMap();
    map.setTile(6, 4, TileType::DownStairs);
    map.setTile(6, 3, TileType::UpStairs);
    map.setTile(3, 3, TileType::ClosedDoor);
    map.setTile(4, 3, TileType::OpenDoor);
    map.setRevealed(2, 2, true);

    entt::registry registry;
    GameResources resources;
    resources.depth = 3;
    resources.rng.reseed(77);
    (void)resources.rng.rangeInclusive(1, 100);
    resources.player = spawnPlayer(registry, Point{2, 2});
    registry.get<CombatStats>(resources.player).hp = 17;

    const auto potion = spawnHealingPotion(registry, Point{1, 1});
    registry.remove<Position>(potion);
    registry.emplace<InBackpack>(potion, InBackpack{resources.player});
    const auto armor = spawnChainMail(registry, Point{1, 2});
    registry.remove<Position>(armor);
    registry.emplace<InBackpack>(armor, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, armor),
        "armor equips before save");
    [[maybe_unused]] const auto goblin = spawnGoblin(registry, Point{4, 2});

    const auto text = serializeGame(registry, resources, map);
    const int nextOriginal = resources.rng.rangeInclusive(1, 100000);

    entt::registry loadedRegistry;
    GameResources loadedResources;
    Map loadedMap{1, 1};
    runner.expect(
        deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
        "save JSON loads successfully");
    runner.expect(loadedResources.depth == 3, "load restores depth");
    runner.expect(loadedMap.tileAt(6, 4) == TileType::DownStairs,
                  "load restores stair tile");
    runner.expect(loadedMap.tileAt(6, 3) == TileType::UpStairs,
                  "load restores up stair tile");
    runner.expect(loadedMap.tileAt(3, 3) == TileType::ClosedDoor &&
                      loadedMap.tileAt(4, 3) == TileType::OpenDoor,
                  "load restores door tile state");
    runner.expect(loadedMap.isRevealed(2, 2), "load restores revealed memory");
    runner.expect(loadedRegistry.valid(loadedResources.player),
                  "load restores player entity");
    runner.expect(loadedRegistry.get<CombatStats>(loadedResources.player).hp ==
                      17,
                  "load restores player HP");
    runner.expect(nextOriginal == loadedResources.rng.rangeInclusive(1, 100000),
                  "load restores RNG engine continuation");

    const auto items = inventoryItems(loadedRegistry, loadedResources.player);
    runner.expect(items.size() == 2, "load restores backpack and equipment");
    runner.expect(
        std::any_of(items.begin(),
                    items.end(),
                    [&](auto item) {
                        return loadedRegistry.all_of<Equipped>(item) &&
                               loadedRegistry.get<Equipped>(item).owner ==
                                   loadedResources.player;
                    }),
        "load remaps equipped owner references");

    int monsterCount = 0;
    const auto monsters = loadedRegistry.view<const Monster>();
    for ([[maybe_unused]] const auto entity : monsters) {
        ++monsterCount;
    }
    runner.expect(monsterCount == 1, "load restores monsters");
}

} // namespace rl::test
