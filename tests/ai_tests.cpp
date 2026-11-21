#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/ai/BystanderAI.hpp"
#include "systems/ai/MonsterAI.hpp"

namespace rl::test {
namespace {

[[nodiscard]] Point entityPoint(const entt::registry& registry,
                                entt::entity entity)
{
    const auto& position = registry.get<Position>(entity);
    return Point{position.x, position.y};
}

} // namespace

void runAiTests(TestRunner& runner)
{
    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.rng.reseed(7U);
        resources.player = spawnPlayer(registry, Point{2, 2});

        const auto bystander = registry.create();
        registry.emplace<Position>(bystander, Position{5, 4});
        registry.emplace<Name>(bystander, Name{"Townsfolk"});
        registry.emplace<BlocksTile>(bystander);
        registry.emplace<Neutral>(bystander);
        registry.emplace<Bystander>(bystander);

        bool moved = false;
        Point previous = entityPoint(registry, bystander);
        for (int turn = 0; turn < 8 && !moved; ++turn) {
            runBystanderAi(registry, resources, map);
            const Point current = entityPoint(registry, bystander);
            moved = current != previous;
            previous = current;
        }
        runner.expect(moved, "bystander AI wanders neutral NPCs");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.rng.reseed(7U);
        resources.player = spawnPlayer(registry, Point{2, 2});

        const auto vendor = registry.create();
        registry.emplace<Position>(vendor, Position{5, 4});
        registry.emplace<Name>(vendor, Name{"Blacksmith"});
        registry.emplace<BlocksTile>(vendor);
        registry.emplace<Neutral>(vendor);
        registry.emplace<Bystander>(vendor);
        registry.emplace<Vendor>(vendor, Vendor{"weapons", 250});

        const Point before = entityPoint(registry, vendor);
        for (int turn = 0; turn < 8; ++turn) {
            runBystanderAi(registry, resources, map);
        }
        runner.expect(entityPoint(registry, vendor) == before,
                      "bystander AI keeps vendors anchored");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});

        const auto townsfolk = registry.create();
        registry.emplace<Position>(townsfolk, Position{3, 2});
        registry.emplace<Name>(townsfolk, Name{"Townsfolk"});
        registry.emplace<BlocksTile>(townsfolk);
        registry.emplace<Monster>(townsfolk);
        registry.emplace<Neutral>(townsfolk);
        registry.emplace<Bystander>(townsfolk);

        runHostileMonsterAi(registry, resources, map);
        runner.expect(!registry.all_of<WantsToMelee>(townsfolk),
                      "hostile AI skips neutral mob actors");
    }
}

} // namespace rl::test
