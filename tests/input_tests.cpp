#include "TestSuites.hpp"

#include <algorithm>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "input/InputEvent.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"

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
