#include "TestSuites.hpp"

#include <algorithm>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "map/Map.hpp"
#include "spawn/Spawner.hpp"
#include "systems/WorldTravelSystem.hpp"
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

} // namespace

void runWorldSurvivalTests(TestRunner& runner)
{
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
}

} // namespace rl::test
