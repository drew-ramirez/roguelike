#include "TestSuites.hpp"

#include <algorithm>
#include <entt/entt.hpp>
#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "map/Map.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/services/SettlementServiceSystem.hpp"
#include "systems/services/ShrineRiteSystem.hpp"
#include "systems/survival/CampSystem.hpp"
#include "systems/survival/CookingSystem.hpp"
#include "systems/survival/ForageSystem.hpp"
#include "systems/travel/WorldTravelSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl::test {
namespace {

[[nodiscard]] int hungerDuration(const entt::registry& registry,
                                 entt::entity entity)
{
    return registry.get<HungerClock>(entity).duration;
}

void unlockVisitedLandmark(WorldMapState& world, Point macro)
{
    auto* cell = worldCellAt(world, macro);
    if (cell == nullptr || !cell->landmark.has_value()) {
        return;
    }

    cell->discovered = true;
    cell->visited = true;
    cell->landmark->fastTravelUnlocked = true;
    if (auto* region = worldRegionById(world, cell->landmark->regionId);
        region != nullptr) {
        region->discovered = true;
        region->visited = true;
    }
}

[[nodiscard]] bool inventoryHasMeal(const entt::registry& registry,
                                    entt::entity owner,
                                    const std::string& recipeId)
{
    const auto view = registry.view<const InBackpack, const Meal>();
    for (const auto item : view) {
        const auto& backpack = view.get<const InBackpack>(item);
        const auto& meal = view.get<const Meal>(item);
        if (backpack.owner == owner && meal.recipeId == recipeId) {
            return true;
        }
    }
    return false;
}

void moveWorldToReedfen(WorldMapState& world)
{
    world.currentMacro = reedfenWorldPosition();
    world.currentScreen = Point{1, 1};
    updateCurrentWorldRegion(world);
}

} // namespace

void runWorldSurvivalTests(TestRunner& runner)
{
    {
        const auto& raws = defaultRawDatabase();
        runner.expect(raws.ingredient("pond_fish") != nullptr &&
                          raws.recipe("marsh_stew") != nullptr &&
                          raws.biomeResourceTable("reed_marsh") != nullptr,
                      "default raws expose camp survival content");
    }

    {
        entt::registry registry;
        const auto& raws = defaultRawDatabase();
        const auto camp =
            spawnRawEntity(registry, raws, "greenholt_hearth", Point{5, 4});
        const auto fish =
            spawnRawEntity(registry, raws, "pond_fish", Point{6, 4});

        runner.expect(camp.has_value() &&
                          registry.all_of<CampSite, CookingStation>(*camp),
                      "raw camp prop spawns camp and cooking components");
        runner.expect(fish.has_value() && registry.all_of<Ingredient>(*fish),
                      "raw ingredient items spawn ingredient components");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{101U});
        resources.player = spawnPlayer(registry, Point{10, 10});
        resources.rng.reseed(101U);
        auto map = openMap(40, 25);
        ShadowcastingFov fov;

        const Point target{greenholtWorldPosition().x + 1,
                           greenholtWorldPosition().y};
        auto* targetCell = worldCellAt(resources.world, target);
        runner.expect(targetCell != nullptr, "macro travel target exists");
        if (targetCell != nullptr) {
            targetCell->dangerTier = 0;
            targetCell->fields.dangerPressure = 0.0;
            targetCell->discovered = false;
        }

        const int before = hungerDuration(registry, resources.player);
        const int expectedCost =
            targetCell == nullptr ? 0 : macroTravelHungerCost(*targetCell);
        runner.expect(
            !moveWorldMapCursor(registry, resources, map, fov, Point{1, 0}) &&
                resources.world.cursor == target && targetCell != nullptr &&
                targetCell->discovered &&
                hungerDuration(registry, resources.player) ==
                    before - expectedCost,
            "macro cursor travel reveals the target and spends "
            "biome hunger");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{202U});
        resources.player = spawnPlayer(registry, Point{10, 10});
        auto map = openMap(40, 25);
        ShadowcastingFov fov;

        resources.world.cursor = reedfenWorldPosition();
        runner.expect(!enterWorldMapSelection(registry, resources, map, fov),
                      "undiscovered world-map cells cannot be entered "
                      "directly");

        auto* reedfen = worldCellAt(resources.world, reedfenWorldPosition());
        if (reedfen != nullptr) {
            reedfen->discovered = true;
            reedfen->visited = true;
        }
        auto* reedfenRegion = worldRegionById(resources.world, "reedfen_ford");
        if (reedfenRegion != nullptr) {
            reedfenRegion->visited = true;
            reedfenRegion->discovered = true;
        }

        runner.expect(availableTravelRoutes(resources).empty(),
                      "visited landmarks stay out of fast travel until "
                      "unlocked");
        unlockVisitedLandmark(resources.world, reedfenWorldPosition());
        const auto routes = availableTravelRoutes(resources);
        runner.expect(
            std::any_of(routes.begin(),
                        routes.end(),
                        [](const RouteLink& route) {
                            return route.destinationRegionId == "reedfen_ford";
                        }),
            "unlocked visited landmarks appear in fast-travel routes");

        const int before = hungerDuration(registry, resources.player);
        runner.expect(
            travelToRegion(registry, resources, map, fov, "reedfen_ford") &&
                resources.world.currentMacro == reedfenWorldPosition() &&
                resources.mapBuilderId == "surface-camp-shrine" &&
                hungerDuration(registry, resources.player) == before - 35,
            "fast travel enters the landmark screen and charges route hunger");
    }

    {
        bool dangerTriggered = false;
        for (Rng::Seed seed = 1U; seed <= 100U && !dangerTriggered; ++seed) {
            entt::registry registry;
            GameResources resources;
            resources.world = generateWorldMap(seed);
            resources.player = spawnPlayer(registry, Point{10, 10});
            resources.rng.reseed(seed);
            auto map = openMap(40, 25);
            ShadowcastingFov fov;

            const Point target{greenholtWorldPosition().x + 1,
                               greenholtWorldPosition().y};
            if (auto* dangerCell = worldCellAt(resources.world, target);
                dangerCell != nullptr) {
                dangerCell->dangerTier = 99;
                dangerCell->fields.dangerPressure = 1.0;
                dangerCell->discovered = false;
            }

            dangerTriggered =
                moveWorldMapCursor(
                    registry, resources, map, fov, Point{1, 0}) &&
                resources.world.currentMacro == target &&
                resources.world.currentScreen == Point{1, 1} &&
                resources.world.currentDangerEncounter &&
                resources.mapBuilderId == "surface-danger-encounter";
        }
        runner.expect(dangerTriggered,
                      "seeded macro travel can force a danger encounter "
                      "screen");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{303U});
        resources.player = spawnPlayer(registry, Point{38, 12});
        auto map = openMap(40, 25);
        ShadowcastingFov fov;

        const auto* currentCell =
            worldCellAt(resources.world, greenholtWorldPosition());
        const int before = hungerDuration(registry, resources.player);
        const int expectedCost =
            currentCell == nullptr ? 0 : localTravelHungerCost(*currentCell);
        runner.expect(
            tryTravelByEdge(registry, resources, map, fov, Point{1, 0}) &&
                resources.world.currentMacro == greenholtWorldPosition() &&
                resources.world.currentScreen == Point{2, 1} &&
                hungerDuration(registry, resources.player) ==
                    before - expectedCost,
            "local edge travel enters the adjacent surface screen and spends "
            "hunger");
        const auto& position = registry.get<Position>(resources.player);
        runner.expect(position.x <= 2 && map.isWalkable(position.x, position.y),
                      "local edge travel places the player near the opposite "
                      "edge");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{404U});
        resources.player = spawnPlayer(registry, Point{4, 4});
        const auto camp = spawnRawEntity(
            registry, defaultRawDatabase(), "greenholt_hearth", Point{5, 4});

        runner.expect(camp.has_value() &&
                          registry.all_of<CampSite, CookingStation>(*camp),
                      "raw camp prop spawns camp and cooking components");

        registry.get<CombatStats>(resources.player).hp = 20;
        registry.get<Pools>(resources.player).hitPoints = 20;
        const int before = hungerDuration(registry, resources.player);
        runner.expect(camp.has_value() &&
                          restAtCamp(registry, resources, *camp),
                      "rest succeeds at a safe camp");
        runner.expect(registry.get<CombatStats>(resources.player).hp == 24 &&
                          hungerDuration(registry, resources.player) ==
                              before - 20,
                      "camp rest heals and spends hunger");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{505U});
        moveWorldToReedfen(resources.world);
        resources.player = spawnPlayer(registry, Point{4, 4});
        resources.rng.reseed(7U);

        const int before = hungerDuration(registry, resources.player);
        runner.expect(forageCurrentRegion(registry, resources),
                      "forage gathers an ingredient in a known biome");

        int carriedIngredients = 0;
        const auto items = registry.view<const InBackpack, const Ingredient>();
        for (const auto item : items) {
            if (items.get<const InBackpack>(item).owner == resources.player) {
                ++carriedIngredients;
            }
        }
        runner.expect(carriedIngredients > 0,
                      "forage puts gathered ingredients in the backpack");
        runner.expect(hungerDuration(registry, resources.player) == before - 10,
                      "forage spends hunger");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{606U});
        moveWorldToReedfen(resources.world);
        resources.player = spawnPlayer(registry, Point{4, 4});
        (void)spawnIngredientInBackpack(registry, resources, "pond_fish");
        (void)spawnIngredientInBackpack(registry, resources, "marsh_root");
        (void)spawnIngredientInBackpack(registry, resources, "reed_shoot");
        const auto camp = spawnRawEntity(
            registry, defaultRawDatabase(), "reed_camp", Point{5, 4});

        runner.expect(camp.has_value() &&
                          cookAtCamp(registry, resources, *camp),
                      "camp cooking consumes matching ingredients");
        runner.expect(
            inventoryHasMeal(registry, resources.player, "marsh_stew"),
            "camp cooking creates a carried meal");
        runner.expect(carriedIngredients(registry, resources.player).empty(),
                      "camp cooking consumes selected ingredients");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{707U});
        moveWorldToReedfen(resources.world);
        resources.player = spawnPlayer(registry, Point{4, 4});
        const auto provider = spawnRawEntity(
            registry, defaultRawDatabase(), "reed_mentor", Point{5, 4});

        runner.expect(provider.has_value() &&
                          registry.all_of<SettlementService>(*provider),
                      "raw mentor spawns a settlement service component");
        runner.expect(
            provider.has_value() &&
                settlementServiceAvailable(resources, registry, *provider),
            "settlement service is available at the landmark");

        const int before = hungerDuration(registry, resources.player);
        runner.expect(provider.has_value() &&
                          useSettlementService(registry, resources, *provider),
                      "settlement service teaches its recipe");
        runner.expect(hungerDuration(registry, resources.player) == before - 5,
                      "settlement service spends hunger");

        resources.world.currentScreen = Point{0, 0};
        runner.expect(
            provider.has_value() &&
                !settlementServiceAvailable(resources, registry, *provider),
            "settlement service is gated by local screen role");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.world = generateWorldMap(Rng::Seed{808U});
        moveWorldToReedfen(resources.world);
        resources.player = spawnPlayer(registry, Point{4, 4});
        registry.get<CombatStats>(resources.player).hp = 20;
        registry.get<Pools>(resources.player).hitPoints = 20;
        const auto shrine = spawnRawEntity(
            registry, defaultRawDatabase(), "stillwater_shrine", Point{5, 4});
        (void)spawnIngredientInBackpack(registry, resources, "pond_fish");

        runner.expect(shrine.has_value() && registry.all_of<Shrine>(*shrine),
                      "raw shrine prop spawns a shrine component");
        runner.expect(shrine.has_value() &&
                          shrineAvailable(resources, registry, *shrine),
                      "shrine is available at a shrine landmark");

        const auto before = carriedIngredients(registry, resources.player);
        runner.expect(shrine.has_value() &&
                          offerAtShrine(registry, resources, *shrine),
                      "shrine offering consumes a matching ingredient");
        runner.expect(carriedIngredients(registry, resources.player).size() +
                              1U ==
                          before.size(),
                      "shrine offering removes one carried ingredient");
        runner.expect(registry.get<CombatStats>(resources.player).hp == 22,
                      "shrine offering applies its blessing");

        resources.world.currentScreen = Point{0, 0};
        runner.expect(shrine.has_value() &&
                          !shrineAvailable(resources, registry, *shrine),
                      "shrine is gated by local screen role");
    }
}

} // namespace rl::test
