#include "TestSuites.hpp"

#include <algorithm>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
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
    resources.townPortalActive = true;
    resources.townPortalReturnDepth = 4;
    resources.townPortalReturnPosition = Point{5, 5};
    resources.townPortalPosition = Point{2, 2};
    resources.log.incrementCounter("kills", 2);
    resources.log.addFragments(
        LogCategory::Items,
        {LogFragment{"You identify ", Color{180, 180, 180, 255}},
         LogFragment{"a bright wand", Color::yellow()}});
    resources.rng.reseed(77);
    (void)resources.rng.rangeInclusive(1, 100);
    resources.player = spawnPlayer(registry, Point{2, 2});
    registry.get<CombatStats>(resources.player).hp = 17;
    registry.get<Attributes>(resources.player).might = 13;
    registry.get<Pools>(resources.player).hitPoints = 17;
    registry.get<Pools>(resources.player).xp = 250;
    registry.get<Pools>(resources.player).gold = 35;

    const auto potion = spawnHealingPotion(registry, Point{1, 1});
    registry.remove<Position>(potion);
    registry.emplace<InBackpack>(potion, InBackpack{resources.player});
    const auto sword = spawnSword(registry, Point{1, 3});
    registry.remove<Position>(sword);
    registry.emplace<InBackpack>(sword, InBackpack{resources.player});
    const auto armor = spawnChainMail(registry, Point{1, 2});
    registry.remove<Position>(armor);
    registry.emplace<InBackpack>(armor, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, armor),
        "armor equips before save");
    const auto goblin = spawnGoblin(registry, Point{4, 2});
    registry.emplace_or_replace<Faction>(goblin, Faction{"goblins"});
    const auto magic = spawnNamedEntity(
        registry, defaultRawDatabase(), "cursed_dagger", Point{5, 2});
    registry.emplace<Confusion>(magic, Confusion{2});

    const auto text = serializeGame(registry, resources, map);
    const int nextOriginal = resources.rng.rangeInclusive(1, 100000);

    entt::registry loadedRegistry;
    GameResources loadedResources;
    Map loadedMap{1, 1};
    runner.expect(
        deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
        "save JSON loads successfully");
    runner.expect(loadedResources.depth == 3, "load restores depth");
    runner.expect(loadedResources.townPortalActive &&
                      loadedResources.townPortalReturnDepth == 4 &&
                      loadedResources.townPortalReturnPosition == Point{5, 5} &&
                      loadedResources.townPortalPosition == Point{2, 2},
                  "load restores active portal state");
    runner.expect(loadedResources.log.counter("kills") == 2,
                  "load restores log progress counters");
    const auto logEntries = loadedResources.log.entries();
    runner.expect(std::any_of(logEntries.begin(),
                              logEntries.end(),
                              [](const LogEntry& entry) {
                                  return entry.category == LogCategory::Items &&
                                         entry.message ==
                                             "You identify a bright wand" &&
                                         entry.fragments.size() == 2;
                              }),
                  "load restores structured log fragments");
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
    runner.expect(
        loadedRegistry.get<Attributes>(loadedResources.player).might == 13,
        "load restores player attributes");
    runner.expect(loadedRegistry.get<Pools>(loadedResources.player).xp == 250 &&
                      loadedRegistry.get<Pools>(loadedResources.player).gold ==
                          35,
                  "load restores player pools");
    runner.expect(nextOriginal == loadedResources.rng.rangeInclusive(1, 100000),
                  "load restores RNG engine continuation");

    const auto items = inventoryItems(loadedRegistry, loadedResources.player);
    runner.expect(items.size() == 3, "load restores backpack and equipment");
    runner.expect(
        std::any_of(items.begin(),
                    items.end(),
                    [&](auto item) {
                        return loadedRegistry.all_of<Equipped>(item) &&
                               loadedRegistry.get<Equipped>(item).owner ==
                                   loadedResources.player;
                    }),
        "load remaps equipped owner references");
    runner.expect(
        std::any_of(
            items.begin(),
            items.end(),
            [&](auto item) {
                return loadedRegistry.all_of<RawIdentity, ItemStats>(item) &&
                       loadedRegistry.get<RawIdentity>(item).templateName ==
                           "sword" &&
                       loadedRegistry.get<ItemStats>(item).baseValue == 45 &&
                       loadedRegistry.all_of<WeaponStats>(item) &&
                       loadedRegistry.get<WeaponStats>(item).damage == "1d8+1";
            }),
        "load restores raw equipment stat components");

    int monsterCount = 0;
    bool loadedRewardedMonster = false;
    bool loadedFutureComponents = false;
    const auto monsters = loadedRegistry.view<const Monster>();
    for (const auto entity : monsters) {
        ++monsterCount;
        if (loadedRegistry.all_of<XpReward>(entity) &&
            loadedRegistry.get<XpReward>(entity).amount == 100) {
            loadedRewardedMonster = true;
        }
        if (loadedRegistry.all_of<Faction>(entity) &&
            loadedRegistry.get<Faction>(entity).id == "goblins") {
            loadedFutureComponents = true;
        }
    }
    runner.expect(monsterCount == 1, "load restores monsters");
    runner.expect(loadedRewardedMonster,
                  "load restores active monster XP rewards");
    runner.expect(loadedFutureComponents,
                  "load restores active future gameplay components");
    bool loadedMagic = false;
    const auto loadedItems = loadedRegistry.view<const Item>();
    for (const auto entity : loadedItems) {
        if (loadedRegistry.all_of<MagicItem, CursedItem, Confusion>(entity)) {
            loadedMagic = true;
        }
    }
    runner.expect(loadedMagic,
                  "load restores magic curse and status components");
}

} // namespace rl::test
