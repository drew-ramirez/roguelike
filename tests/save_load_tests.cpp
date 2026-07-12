#include "TestSuites.hpp"

#include <algorithm>
#include <utility>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "save/SaveGame.hpp"
#include "save/SaveTypes.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl::test {

void runSaveLoadTests(TestRunner& runner)
{
    auto map = openMap();
    map.setTile(6, 4, TileType::DownStairs);
    map.setTile(6, 3, TileType::UpStairs);
    map.setTile(3, 3, TileType::ClosedDoor);
    map.setTile(4, 3, TileType::OpenDoor);
    map.setTile(1, 4, TileType::Tree);
    map.setTile(2, 4, TileType::DenseReeds);
    map.setTile(3, 4, TileType::Rock);
    map.setRevealed(2, 2, true);

    entt::registry registry;
    GameResources resources;
    resources.depth = 3;
    resources.townPortalActive = true;
    resources.townPortalReturnDepth = 4;
    resources.townPortalReturnPosition = Point{5, 5};
    resources.townPortalPosition = Point{2, 2};
    resources.world = generateWorldMap(resources.rng.seed());
    resources.world.currentMacro = reedfenWorldPosition();
    resources.world.currentScreen = Point{1, 1};
    resources.world.cursor = limestoneWorldPosition();
    resources.world.undergroundEntryDepth = 5;
    updateCurrentWorldRegion(resources.world);
    runner.expect(unlockCurrentLandmarkFastTravel(resources.world),
                  "test setup unlocks Reedfen fast travel");
    if (auto* greenholt =
            worldRegionById(resources.world, "greenholt_hearth")) {
        for (auto& route : greenholt->routes) {
            if (route.destinationRegionId == "reedfen_ford") {
                route.discovered = false;
            }
        }
    }
    SurfaceScreenState cachedSurface;
    cachedSurface.key =
        SurfaceScreenKey{reedfenWorldPosition(), Point{0, 1}, false};
    cachedSurface.builderId = "surface-cardinal-transition";
    cachedSurface.map = Map{6, 6, TileType::Floor};
    cachedSurface.map.setTile(1, 1, TileType::Wall);
    cachedSurface.map.setRevealed(2, 2, true);
    StoredEntity cachedEntity;
    cachedEntity.name = Name{"Cached reed cache"};
    cachedEntity.position = Position{2, 2};
    cachedEntity.ingredient =
        Ingredient{{"fish", "marsh"}, "reed_marsh", "reedfen_ford", 2, 1};
    cachedEntity.meal = Meal{"marsh_stew"};
    cachedEntity.campSite = CampSite{"reed_blind", 1};
    cachedEntity.settlementService =
        SettlementService{"teach_recipe", "marsh_stew"};
    cachedEntity.shrine = Shrine{"stillwater", {"fish", "marsh"}, 2};
    cachedEntity.cookingStation =
        CookingStation{"marsh_fire", {"boiled"}, {"reed_marsh"}};
    cachedSurface.entities.push_back(cachedEntity);
    resources.world.surfaceScreens.emplace(cachedSurface.key,
                                           std::move(cachedSurface));
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
    const auto pondFish = spawnRawEntity(
        registry, defaultRawDatabase(), "pond_fish", Point{2, 3});
    if (pondFish.has_value()) {
        registry.remove<Position>(*pondFish);
        registry.emplace<InBackpack>(*pondFish, InBackpack{resources.player});
        registry.get<Ingredient>(*pondFish).originBiome = "reed_marsh";
        registry.get<Ingredient>(*pondFish).originRegionId = "reedfen_ford";
    }
    const auto cookedMeal = spawnRawEntity(
        registry, defaultRawDatabase(), "cooked_meal", Point{2, 4});
    if (cookedMeal.has_value()) {
        registry.remove<Position>(*cookedMeal);
        registry.emplace<InBackpack>(*cookedMeal, InBackpack{resources.player});
        registry.emplace<Meal>(*cookedMeal, Meal{"marsh_stew"});
    }
    (void)spawnRawEntity(
        registry, defaultRawDatabase(), "greenholt_hearth", Point{6, 2});
    const auto mentor = registry.create();
    registry.emplace<Name>(mentor, Name{"Saved Reed Mentor"});
    registry.emplace<Position>(mentor, Position{7, 2});
    registry.emplace<BlocksTile>(mentor);
    registry.emplace<Neutral>(mentor);
    registry.emplace<SettlementService>(
        mentor, SettlementService{"teach_recipe", "marsh_stew"});
    const auto shrine = registry.create();
    registry.emplace<Name>(shrine, Name{"Saved Stillwater Shrine"});
    registry.emplace<Position>(shrine, Position{8, 2});
    registry.emplace<BlocksTile>(shrine);
    registry.emplace<Prop>(shrine);
    registry.emplace<Shrine>(shrine, Shrine{"stillwater", {"fish"}, 2});

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
    runner.expect(
        loadedResources.world.currentMacro == reedfenWorldPosition() &&
            loadedResources.world.currentScreen == Point{1, 1} &&
            loadedResources.world.cursor == limestoneWorldPosition() &&
            loadedResources.world.currentRegionId == "reedfen_ford" &&
            loadedResources.world.undergroundEntryDepth == 5,
        "load restores surface world position and cursor state");
    const auto* loadedReedfen =
        worldCellAt(loadedResources.world, reedfenWorldPosition());
    runner.expect(loadedReedfen != nullptr && loadedReedfen->discovered &&
                      loadedReedfen->visited &&
                      loadedReedfen->landmark.has_value() &&
                      loadedReedfen->landmark->fastTravelUnlocked,
                  "load restores landmark discovery and fast travel unlock");
    const auto* loadedGreenholt =
        worldRegionById(loadedResources.world, "greenholt_hearth");
    runner.expect(loadedGreenholt != nullptr &&
                      findDiscoveredRoute(*loadedGreenholt, "reedfen_ford") ==
                          nullptr,
                  "load restores route discovery state");
    const SurfaceScreenKey cachedKey{
        reedfenWorldPosition(), Point{0, 1}, false};
    const auto cached = loadedResources.world.surfaceScreens.find(cachedKey);
    runner.expect(
        cached != loadedResources.world.surfaceScreens.end() &&
            cached->second.builderId == "surface-cardinal-transition" &&
            cached->second.map.tileAt(1, 1) == TileType::Wall &&
            cached->second.map.isRevealed(2, 2) &&
            cached->second.entities.size() == 1 &&
            cached->second.entities.front().name.has_value() &&
            cached->second.entities.front().name->value ==
                "Cached reed cache" &&
            cached->second.entities.front().ingredient.has_value() &&
            cached->second.entities.front().meal.has_value() &&
            cached->second.entities.front().campSite.has_value() &&
            cached->second.entities.front().settlementService.has_value() &&
            cached->second.entities.front().shrine.has_value() &&
            cached->second.entities.front().cookingStation.has_value(),
        "load restores cached surface screen snapshots");
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
    runner.expect(loadedMap.tileAt(1, 4) == TileType::Tree &&
                      loadedMap.tileAt(2, 4) == TileType::DenseReeds &&
                      loadedMap.tileAt(3, 4) == TileType::Rock,
                  "load round-trips every natural terrain tile");
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
    runner.expect(items.size() == 5, "load restores backpack and equipment");
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
    bool loadedIngredient = false;
    bool loadedMeal = false;
    for (const auto item : items) {
        if (loadedRegistry.all_of<Ingredient>(item) &&
            loadedRegistry.get<Ingredient>(item).originRegionId ==
                "reedfen_ford") {
            loadedIngredient = true;
        }
        if (loadedRegistry.all_of<Meal>(item) &&
            loadedRegistry.get<Meal>(item).recipeId == "marsh_stew") {
            loadedMeal = true;
        }
    }
    runner.expect(loadedIngredient && loadedMeal,
                  "load restores active camp survival item components");

    bool loadedCamp = false;
    const auto loadedCamps =
        loadedRegistry.view<const CampSite, const CookingStation>();
    for (const auto entity : loadedCamps) {
        if (loadedRegistry.get<CampSite>(entity).campType == "ember_camp" &&
            loadedRegistry.get<CookingStation>(entity).stationId ==
                "campfire") {
            loadedCamp = true;
        }
    }
    runner.expect(loadedCamp,
                  "load restores active camp and cooking station components");
    bool loadedService = false;
    const auto loadedServices = loadedRegistry.view<const SettlementService>();
    for (const auto entity : loadedServices) {
        if (loadedRegistry.get<SettlementService>(entity).recipeId ==
            "marsh_stew") {
            loadedService = true;
        }
    }
    runner.expect(loadedService,
                  "load restores active settlement service components");
    bool loadedShrine = false;
    const auto loadedShrines = loadedRegistry.view<const Shrine>();
    for (const auto entity : loadedShrines) {
        if (loadedRegistry.get<Shrine>(entity).powerId == "stillwater") {
            loadedShrine = true;
        }
    }
    runner.expect(loadedShrine, "load restores active shrine components");

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

    {
        entt::registry oldRegistry;
        GameResources oldResources;
        oldResources.player = spawnPlayer(oldRegistry, Point{2, 2});
        oldResources.world = generateWorldMap(oldResources.rng.seed());
        oldResources.world.active = true;
        oldResources.world.onSurface = true;
        oldResources.world.currentMacro = reedfenWorldPosition();
        oldResources.world.currentScreen = Point{0, 1};
        oldResources.world.currentDangerEncounter = false;
        updateCurrentWorldRegion(oldResources.world);

        const auto obsolete = oldRegistry.create();
        oldRegistry.emplace<Name>(obsolete, Name{"Obsolete surface marker"});
        oldRegistry.emplace<Position>(obsolete, Position{4, 4});
        oldRegistry.emplace<Prop>(obsolete);

        SurfaceScreenState stale;
        stale.key =
            SurfaceScreenKey{reedfenWorldPosition(), Point{1, 1}, false};
        stale.builderId = "surface-camp-shrine";
        stale.map = Map{80, 43, TileType::Wall};
        oldResources.world.surfaceScreens.emplace(stale.key, std::move(stale));

        Map oldSurface{80, 43, TileType::Floor};
        auto archive = save::Json::parse(
            serializeGame(oldRegistry, oldResources, oldSurface));
        archive["version"] = 8;
        archive["resources"]["world"].erase("surface_generation_version");

        entt::registry migratedRegistry;
        GameResources migratedResources;
        Map migratedMap{1, 1};
        runner.expect(deserializeGame(archive.dump(),
                                      migratedRegistry,
                                      migratedResources,
                                      migratedMap),
                      "older surface save migrates successfully");

        bool foundObsoleteMarker = false;
        for (const auto entity : migratedRegistry.view<const Name>()) {
            foundObsoleteMarker = foundObsoleteMarker ||
                                  migratedRegistry.get<Name>(entity).value ==
                                      "Obsolete surface marker";
        }
        int reeds = 0;
        for (int y = 0; y < migratedMap.height(); ++y) {
            for (int x = 0; x < migratedMap.width(); ++x) {
                reeds +=
                    migratedMap.tileAt(x, y) == TileType::DenseReeds ? 1 : 0;
            }
        }
        const auto& migratedPosition =
            migratedRegistry.get<Position>(migratedResources.player);
        runner.expect(
            migratedResources.world.surfaceScreens.empty() &&
                !migratedResources.world.surfaceRegenerationRequired &&
                !foundObsoleteMarker && reeds > 0 &&
                migratedPosition.x == migratedMap.width() / 2 &&
                migratedPosition.y == migratedMap.height() / 2,
            "surface migration discards stale screens and local "
            "entities then regenerates deterministic terrain");
    }
}

} // namespace rl::test
