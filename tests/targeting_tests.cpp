#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/inventory/ItemSystem.hpp"
#include "systems/rendering/RenderSystem.hpp"
#include "ui/Targeting.hpp"

namespace rl::test {

void runTargetingTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);

    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    const auto scroll = spawnFireballScroll(registry, Point{1, 1});
    registry.remove<Position>(scroll);
    registry.emplace<InBackpack>(scroll, InBackpack{resources.player});
    const auto first = spawnGoblin(registry, Point{4, 2});
    const auto second = spawnGoblin(registry, Point{5, 2});
    const auto outside = spawnGoblin(registry, Point{9, 6});

    runner.expect(
        !useInventoryItem(registry, resources, map, resources.player, scroll),
        "ranged item opens targeting before consuming a turn");
    runner.expect(resources.pendingTargeting.has_value(),
                  "ranged item creates pending targeting state");
    cancelPendingTarget(resources);
    runner.expect(registry.valid(scroll),
                  "targeting cancel preserves the scroll");

    runner.expect(
        !useInventoryItem(registry, resources, map, resources.player, scroll),
        "ranged item can be targeted again");
    resources.pendingTargeting->cursor = Point{10, 6};
    runner.expect(!confirmPendingTarget(registry, resources, map),
                  "invalid ranged target is rejected");
    runner.expect(registry.valid(scroll),
                  "invalid target does not consume the scroll");

    resources.pendingTargeting->cursor = Point{4, 2};
    runner.expect(confirmPendingTarget(registry, resources, map),
                  "valid target confirms item use");
    runner.expect(processItemUses(registry, resources, map),
                  "confirmed ranged item resolves effect");
    runner.expect(!registry.valid(scroll),
                  "confirmed ranged consumable is consumed");
    runner.expect(registry.all_of<SufferDamage>(first) &&
                      registry.all_of<SufferDamage>(second),
                  "AOE damages monsters inside radius");
    runner.expect(!registry.all_of<SufferDamage>(outside),
                  "AOE does not damage monsters outside radius");

    Camera camera{5, 4};
    camera.centerOn(Point{8, 5}, map);
    runner.expect(camera.worldToScreen(Point{8, 5}) == Point{2, 2} &&
                      camera.screenToWorld(Point{2, 2}) == Point{8, 5},
                  "scrolled camera maps the center tile in both directions");
    runner.expect(!camera.worldToScreen(Point{1, 1}).has_value(),
                  "camera culls world points outside the viewport");

    Map largeMap = openMap(20, 12);
    largeMap.setVisible(8, 5);
    largeMap.setVisible(1, 1);
    entt::registry renderRegistry;
    [[maybe_unused]] const auto visible =
        spawnGoblin(renderRegistry, Point{8, 5});
    const auto outsideView = spawnGoblin(renderRegistry, Point{1, 1});
    renderRegistry.get<Renderable>(outsideView).glyph = U'X';
    ConsoleBuffer console{10, 6};
    console.clear();
    renderEntities(renderRegistry, largeMap, console, camera);
    runner.expect(console.cellAt(2, 2).glyph == U'g',
                  "scrolled rendering draws the visible goblin at center");
    runner.expect(console.cellAt(1, 1).glyph == U' ',
                  "scrolled rendering omits visible entities off screen");
}

} // namespace rl::test
