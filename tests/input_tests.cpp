#include "TestSuites.hpp"

#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
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
}

} // namespace rl::test
