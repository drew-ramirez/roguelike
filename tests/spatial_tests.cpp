#include "TestSuites.hpp"

#include <algorithm>
#include <vector>

#include <entt/entt.hpp>

#include "ecs/Components.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/spatial/SpatialIndexSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] entt::entity entityAt(entt::registry& registry, Point position)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{position.x, position.y});
    return entity;
}

[[nodiscard]] bool contains(const std::vector<entt::entity>& entities,
                            entt::entity entity)
{
    return std::find(entities.begin(), entities.end(), entity) !=
           entities.end();
}

} // namespace

void runSpatialTests(TestRunner& runner)
{
    {
        entt::registry registry;
        const auto blocker = entityAt(registry, Point{4, 3});
        registry.emplace<BlocksTile>(blocker);
        const auto item = entityAt(registry, Point{4, 3});
        registry.emplace<Item>(item);
        const auto trap = entityAt(registry, Point{4, 3});
        registry.emplace<Trap>(trap);
        const auto elsewhere = entityAt(registry, Point{5, 3});
        registry.emplace<Item>(elsewhere);

        const auto content = tileContentAt(registry, Point{4, 3});
        runner.expect(content.blocker == blocker,
                      "tile content reports blocking entity");
        runner.expect(content.item == item, "tile content reports item entity");
        runner.expect(content.trap == trap, "tile content reports trap entity");
        runner.expect(content.entities.size() == 3 &&
                          contains(content.entities, blocker) &&
                          contains(content.entities, item) &&
                          contains(content.entities, trap) &&
                          !contains(content.entities, elsewhere),
                      "tile content reports all occupants on the tile");

        const auto ignored = tileContentAt(registry, Point{4, 3}, blocker);
        runner.expect(!ignored.blocker.has_value(),
                      "ignored blocker does not block its own tile");
        runner.expect(ignored.item == item,
                      "ignored blocker leaves same-tile items visible");
    }

    {
        auto map = openMap();
        entt::registry registry;
        const auto blocker = entityAt(registry, Point{3, 2});
        registry.emplace<BlocksTile>(blocker);

        runner.expect(isBlocked(map, registry, Point{3, 2}),
                      "spatial query sees live blockers");
        registry.destroy(blocker);
        runner.expect(!isBlocked(map, registry, Point{3, 2}),
                      "destroyed blockers stop blocking immediately");
    }

    {
        auto map = openMap();
        entt::registry registry;
        const auto player = spawnPlayer(registry, Point{2, 2});
        const auto blocker = entityAt(registry, Point{3, 2});
        registry.emplace<BlocksTile>(blocker);

        runner.expect(!tryMoveEntity(registry, player, map, Point{1, 0}),
                      "movement refuses occupied tiles");
        registry.destroy(blocker);
        runner.expect(tryMoveEntity(registry, player, map, Point{1, 0}),
                      "movement accepts a tile after blocker cleanup");
        const auto& position = registry.get<Position>(player);
        runner.expect(position.x == 3 && position.y == 2,
                      "movement lands on the cleared tile");
    }
}

} // namespace rl::test
