#include "TestSuites.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "input/InputEvent.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MapTransitionSystem.hpp"
#include "ui/WorldMapMenu.hpp"
#include "world/WorldGeneration.hpp"

namespace rl::test {

void runInputTests(TestRunner& runner)
{
    auto map = openMap();
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});

    const std::vector<InputEvent> moveRight{{InputEventType::KeyDown, Key::D}};
    runner.expect(handleMapInput(registry, resources, map, moveRight) ==
                      PlayerInputResult::TookTurn,
                  "normal-map d should still move right");
    const auto& moved = registry.get<Position>(resources.player);
    runner.expect(moved.x == 3 && moved.y == 2, "d moved the player right");

    const std::vector<InputEvent> openInventory{
        {InputEventType::KeyDown, Key::I}};
    runner.expect(handleMapInput(registry, resources, map, openInventory) ==
                      PlayerInputResult::OpenInventory,
                  "i opens inventory instead of taking a turn");

    const std::vector<InputEvent> descend{
        {InputEventType::KeyDown, Key::Descend}};
    runner.expect(handleMapInput(registry, resources, map, descend) ==
                      PlayerInputResult::Descend,
                  "descend key requests down transition");

    const std::vector<InputEvent> ascend{
        {InputEventType::KeyDown, Key::Ascend}};
    runner.expect(handleMapInput(registry, resources, map, ascend) ==
                      PlayerInputResult::Ascend,
                  "ascend key requests up transition");

    GameResources surfaceResources;
    surfaceResources.world = generateWorldMap(Rng::Seed{42U});
    surfaceResources.world.currentMacro = reedfenWorldPosition();
    surfaceResources.world.cursor = greenholtWorldPosition();
    runner.expect(handleMapInput(registry, surfaceResources, map, ascend) ==
                          PlayerInputResult::OpenWorldMap &&
                      surfaceResources.world.cursor == reedfenWorldPosition(),
                  "surface ascend key opens the world map");

    GameResources undergroundResources;
    undergroundResources.world = generateWorldMap(Rng::Seed{42U});
    undergroundResources.world.onSurface = false;
    runner.expect(handleMapInput(registry, undergroundResources, map, ascend) ==
                      PlayerInputResult::Ascend,
                  "underground ascend key keeps stair behavior");

    GameResources worldMapResources;
    worldMapResources.world = generateWorldMap(Rng::Seed{11U});
    const std::vector<InputEvent> moveCursor{
        {InputEventType::KeyDown, Key::Right}};
    runner.expect(handleWorldMapInput(worldMapResources, moveCursor) ==
                          WorldMapInputResult::MoveCursor &&
                      worldMapResources.world.pendingLocalTravel ==
                          std::optional<Point>{Point{1, 0}} &&
                      worldMapResources.world.cursor ==
                          greenholtWorldPosition(),
                  "world map movement records pending macro travel");
    runner.expect(handleWorldMapInput(worldMapResources, descend) ==
                      WorldMapInputResult::EnterSelected,
                  "world map descend key enters the selected cell");

    GameResources edgeResources;
    edgeResources.world = generateWorldMap(Rng::Seed{12U});
    edgeResources.player = resources.player;
    auto edgeMap = openMap(40, 25);
    registry.get<Position>(resources.player) = Position{38, 12};
    runner.expect(handleMapInput(registry, edgeResources, edgeMap, moveRight) ==
                          PlayerInputResult::TravelLocalEdge &&
                      edgeResources.world.pendingLocalTravel ==
                          std::optional<Point>{Point{1, 0}},
                  "surface edge movement requests local screen travel");

    auto surfaceMap = openMap(40, 25);
    entt::registry surfaceRegistry;
    GameResources entryResources;
    entryResources.world = generateWorldMap(Rng::Seed{12U});
    entryResources.player = spawnPlayer(surfaceRegistry, Point{2, 2});
    ShadowcastingFov surfaceFov;
    runner.expect(tryEnterSurfaceScreen(surfaceRegistry,
                                        entryResources,
                                        surfaceMap,
                                        surfaceFov,
                                        reedfenWorldPosition(),
                                        Point{1, 1}) &&
                      entryResources.world.currentMacro ==
                          reedfenWorldPosition() &&
                      entryResources.world.currentScreen == Point{1, 1} &&
                      worldCellAt(entryResources.world, reedfenWorldPosition())
                          ->discovered,
                  "world map entry generates and discovers a local screen");

    const std::vector<InputEvent> wallMove{{InputEventType::KeyDown, Key::W}};
    auto& position = registry.get<Position>(resources.player);
    position.x = 1;
    position.y = 1;
    runner.expect(handleMapInput(registry, resources, map, wallMove) ==
                      PlayerInputResult::None,
                  "walls block WASD movement");

    map.setTile(2, 1, TileType::ClosedDoor);
    registry.get<Position>(resources.player) = Position{1, 1};
    registry.get<Viewshed>(resources.player).dirty = false;
    const std::vector<InputEvent> openDoor{{InputEventType::KeyDown, Key::D}};
    runner.expect(handleMapInput(registry, resources, map, openDoor) ==
                      PlayerInputResult::TookTurn,
                  "bumping a closed door consumes a turn");
    runner.expect(map.tileAt(2, 1) == TileType::OpenDoor,
                  "bumping a closed door opens it");
    runner.expect(registry.get<Position>(resources.player).x == 1,
                  "opening a door does not move the player");
    runner.expect(registry.get<Viewshed>(resources.player).dirty,
                  "opening a door marks FOV dirty");

    {
        auto neutralMap = openMap();
        entt::registry neutralRegistry;
        GameResources neutralResources;
        neutralResources.player = spawnPlayer(neutralRegistry, Point{2, 2});

        const auto vendor = neutralRegistry.create();
        neutralRegistry.emplace<Position>(vendor, Position{3, 2});
        neutralRegistry.emplace<Name>(vendor, Name{"Blacksmith"});
        neutralRegistry.emplace<BlocksTile>(vendor);
        neutralRegistry.emplace<Neutral>(vendor);
        neutralRegistry.emplace<Vendor>(vendor, Vendor{"weapons", 250});

        runner.expect(
            handleMapInput(
                neutralRegistry, neutralResources, neutralMap, moveRight) ==
                PlayerInputResult::OpenVendor,
            "bumping a vendor opens the vendor menu");
        runner.expect(
            !neutralRegistry.all_of<WantsToMelee>(neutralResources.player),
            "bumping a vendor does not queue melee");
        runner.expect(
            neutralRegistry.get<Position>(neutralResources.player).x == 2,
            "bumping a vendor does not move the player");
        runner.expect(neutralResources.pendingVendor.has_value() &&
                          neutralResources.pendingVendor->vendor == vendor,
                      "bumping a vendor records the active vendor");
    }

    {
        auto propMap = openMap();
        entt::registry propRegistry;
        GameResources propResources;
        propResources.player = spawnPlayer(propRegistry, Point{2, 2});

        const auto prop = propRegistry.create();
        propRegistry.emplace<Position>(prop, Position{3, 2});
        propRegistry.emplace<Name>(prop, Name{"Chair"});
        propRegistry.emplace<BlocksTile>(prop);
        propRegistry.emplace<Prop>(prop);

        runner.expect(
            handleMapInput(propRegistry, propResources, propMap, moveRight) ==
                PlayerInputResult::None,
            "bumping a prop does not consume a turn");
        runner.expect(!propRegistry.all_of<WantsToMelee>(propResources.player),
                      "bumping a prop does not queue melee");
    }

    Map sightMap{6, 3, TileType::Floor};
    sightMap.setTile(2, 1, TileType::ClosedDoor);
    ShadowcastingFov fov;
    const auto closedVisible = fov.compute(sightMap, Point{1, 1}, 5);
    runner.expect(std::find(closedVisible.begin(),
                            closedVisible.end(),
                            Point{3, 1}) == closedVisible.end(),
                  "closed doors block FOV");
    sightMap.setTile(2, 1, TileType::OpenDoor);
    const auto openVisible = fov.compute(sightMap, Point{1, 1}, 5);
    runner.expect(std::find(openVisible.begin(),
                            openVisible.end(),
                            Point{3, 1}) != openVisible.end(),
                  "open doors do not block FOV");
}

} // namespace rl::test
