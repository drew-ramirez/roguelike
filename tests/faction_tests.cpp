#include "TestSuites.hpp"

#include <string>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "spawn/Spawner.hpp"
#include "systems/ai/MonsterAI.hpp"

namespace rl::test {
namespace {

[[nodiscard]] entt::entity
makeActor(entt::registry& registry, Point position, std::string faction)
{
    const auto actor = registry.create();
    registry.emplace<Position>(actor, Position{position.x, position.y});
    registry.emplace<Name>(actor, Name{"Faction Actor"});
    registry.emplace<Monster>(actor);
    registry.emplace<BlocksTile>(actor);
    registry.emplace<CombatStats>(actor, CombatStats{10, 10, 0, 3});
    registry.emplace<Faction>(actor, Faction{std::move(faction)});
    return actor;
}

} // namespace

void runFactionTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{9, 6});
        const auto first = makeActor(registry, Point{3, 2}, "dark_elf_clan_a");
        const auto rival = makeActor(registry, Point{4, 2}, "dark_elf_clan_b");

        runHostileMonsterAi(registry, resources, map);
        runner.expect(registry.all_of<WantsToMelee>(first) &&
                          registry.get<WantsToMelee>(first).target == rival,
                      "rival factions target each other");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{9, 6});
        const auto first = makeActor(registry, Point{3, 2}, "dark_elf_clan_a");
        [[maybe_unused]] const auto ally =
            makeActor(registry, Point{4, 2}, "dark_elf_clan_a");

        runHostileMonsterAi(registry, resources, map);
        runner.expect(!registry.all_of<WantsToMelee>(first),
                      "allied factions ignore each other");
    }
}

} // namespace rl::test
