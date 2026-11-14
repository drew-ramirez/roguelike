#include "TestSuites.hpp"

#include <string>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MasterDungeonMap.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/travel/MapTransitionSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] int countNamedItems(const entt::registry& registry,
                                  const std::string& name)
{
    int count = 0;
    const auto items = registry.view<const Item, const Name>();
    for (const auto entity : items) {
        if (items.get<const Name>(entity).value == name) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] int countNamedMonsters(const entt::registry& registry,
                                     const std::string& name)
{
    int count = 0;
    const auto monsters = registry.view<const Monster, const Name>();
    for (const auto entity : monsters) {
        if (monsters.get<const Name>(entity).value == name) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool hasNamedCamp(const entt::registry& registry,
                                const std::string& name)
{
    const auto camps =
        registry.view<const CampSite, const CookingStation, const Name>();
    for (const auto entity : camps) {
        if (camps.get<const Name>(entity).value == name) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool hasNamedMealIngredient(const entt::registry& registry,
                                          const std::string& name,
                                          const std::string& recipeId)
{
    const auto ingredients =
        registry.view<const Ingredient, const Meal, const Name>();
    for (const auto entity : ingredients) {
        if (ingredients.get<const Name>(entity).value == name &&
            ingredients.get<const Meal>(entity).recipeId == recipeId) {
            return true;
        }
    }
    return false;
}

} // namespace

void runLevelSnapshotTests(TestRunner& runner)
{
    {
        auto map = openMap();
        map.setRevealed(2, 2, true);

        entt::registry registry;
        const auto player = spawnPlayer(registry, Point{2, 2});
        [[maybe_unused]] const auto droppedPotion =
            spawnHealingPotion(registry, Point{3, 3});
        [[maybe_unused]] const auto camp = spawnRawEntity(
            registry, defaultRawDatabase(), "greenholt_hearth", Point{5, 5});
        const auto droppedFish = spawnRawEntity(
            registry, defaultRawDatabase(), "pond_fish", Point{6, 5});
        if (droppedFish.has_value()) {
            registry.emplace<Meal>(*droppedFish, Meal{"marsh_stew"});
            registry.get<Ingredient>(*droppedFish).originBiome = "reed_marsh";
        }
        const auto backpackPotion = spawnHealingPotion(registry, Point{4, 3});
        registry.remove<Position>(backpackPotion);
        registry.emplace<InBackpack>(backpackPotion, InBackpack{player});

        const auto defeatedGoblin = spawnGoblin(registry, Point{5, 3});
        registry.destroy(defeatedGoblin);

        MasterDungeonMap dungeon;
        dungeon.storeLevel(1, "town", map, registry, player);
        runner.expect(dungeon.hasLevel(1), "snapshot store records a level");

        destroyLevelLocalEntities(registry, player);
        runner.expect(registry.valid(player),
                      "snapshot cleanup preserves the player");
        runner.expect(registry.valid(backpackPotion),
                      "snapshot cleanup preserves backpack contents");

        Map restored{1, 1, TileType::Wall};
        dungeon.restoreLevel(1, restored, registry);
        runner.expect(restored.isRevealed(2, 2),
                      "snapshot restore preserves map memory");
        runner.expect(countNamedItems(registry, "Healing Potion") == 2,
                      "snapshot restore recreates level-local items");
        runner.expect(hasNamedCamp(registry, "Greenholt Hearth"),
                      "snapshot restore preserves camp cooking props");
        runner.expect(
            hasNamedMealIngredient(registry, "Pond Fish", "marsh_stew"),
            "snapshot restore preserves survival item components");
        runner.expect(countNamedMonsters(registry, "Goblin") == 0,
                      "snapshot restore omits defeated monsters");
    }

    {
        Rng rng{99U};
        auto level = buildLevelForDepth(1, 40, 25, rng, MapBuildOptions{});
        level.map.setRevealed(2, 2, true);

        entt::registry registry;
        GameResources resources;
        resources.mapBuilderId = "normal";
        resources.rng.reseed(123U);
        resources.player = spawnPlayer(registry, level.playerStart);
        [[maybe_unused]] const auto dropped =
            spawnHealingPotion(registry, Point{3, 3});
        const auto defeated = spawnGoblin(registry, Point{4, 3});
        registry.destroy(defeated);

        ShadowcastingFov fov;
        runner.expect(tryChangeLevel(registry,
                                     resources,
                                     level.map,
                                     fov,
                                     2,
                                     LevelArrivalMode::UpStairs),
                      "stair transition reaches a deeper level");
        runner.expect(resources.depth == 2, "stair transition updates depth");

        runner.expect(tryChangeLevel(registry,
                                     resources,
                                     level.map,
                                     fov,
                                     1,
                                     LevelArrivalMode::DownStairs),
                      "backtracking restores an archived level");
        runner.expect(resources.depth == 1, "backtracking restores depth");
        runner.expect(level.map.isRevealed(2, 2),
                      "backtracking restores map memory");
        runner.expect(countNamedItems(registry, "Healing Potion") == 1,
                      "backtracking restores dropped item snapshot");
        runner.expect(countNamedMonsters(registry, "Goblin") == 0,
                      "backtracking does not revive defeated monsters");
    }

    {
        auto storedMap = openMap();
        storedMap.setRevealed(2, 2, true);

        entt::registry registry;
        GameResources resources;
        resources.depth = 2;
        resources.rng.reseed(77U);
        resources.player = spawnPlayer(registry, Point{2, 2});
        [[maybe_unused]] const auto potion =
            spawnHealingPotion(registry, Point{3, 3});
        resources.dungeon.storeLevel(
            1, "town", storedMap, registry, resources.player);

        auto currentMap = openMap();
        currentMap.setTile(5, 5, TileType::DownStairs);
        const auto text = serializeGame(registry, resources, currentMap);

        entt::registry loadedRegistry;
        GameResources loadedResources;
        Map loadedMap{1, 1};
        runner.expect(
            deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
            "save JSON with archived levels loads");
        runner.expect(loadedResources.dungeon.hasLevel(1),
                      "load restores archived dungeon levels");

        Map restored{1, 1, TileType::Wall};
        loadedResources.dungeon.restoreLevel(1, restored, loadedRegistry);
        runner.expect(restored.isRevealed(2, 2),
                      "load restores archived map memory");
        runner.expect(countNamedItems(loadedRegistry, "Healing Potion") >= 1,
                      "load restores archived level entities");
    }
}

} // namespace rl::test
