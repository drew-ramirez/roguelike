#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "render/WallGlyph.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/TrapSystem.hpp"
#include "ui/MainMenu.hpp"

namespace rl::test {
namespace {

[[nodiscard]] int countParticles(const entt::registry& registry)
{
    int count = 0;
    const auto view = registry.view<const Particle>();
    for ([[maybe_unused]] const auto entity : view) {
        ++count;
    }
    return count;
}

} // namespace

void runPolishPressureTests(TestRunner& runner)
{
    {
        Map map{3, 3, TileType::Floor};
        map.setTile(1, 0, TileType::Wall);
        map.setTile(1, 1, TileType::Wall);
        map.setTile(1, 2, TileType::Wall);

        runner.expect(wallNeighborMask(map, 1, 1) == 3,
                      "wall bitmask sees north and south walls");
        runner.expect(wallGlyphFromMask(3) == U'\u2502',
                      "connected wall glyph uses vertical line");
        runner.expect(wallGlyphFromMask(3, WallGlyphStyle::AsciiFallback) ==
                          U'#',
                      "wall glyph fallback stays ASCII");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto goblin = spawnGoblin(registry, Point{3, 2});

        queueDamage(registry, goblin, 2);
        RunState state = RunState::PlayerTurn;
        applyDamageAndCleanup(registry, resources, state, &map);
        runner.expect(map.hasBloodstain(3, 2),
                      "damage marks a persistent blood decal");
        runner.expect(map.isWalkable(3, 2) && !map.blocksSight(3, 2),
                      "blood does not affect movement or sight");

        const auto text = serializeGame(registry, resources, map);
        entt::registry loadedRegistry;
        GameResources loadedResources;
        Map loadedMap{1, 1};
        runner.expect(
            deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
            "blood save data loads");
        runner.expect(loadedMap.hasBloodstain(3, 2),
                      "blood survives save/load");
    }

    {
        entt::registry registry;
        GameResources resources;
        requestDamageParticle(resources, Point{1, 1});
        flushParticleRequests(registry, resources);
        runner.expect(countParticles(registry) == 1,
                      "particle requests create transient entities");
        for (int frame = 0; frame < 6; ++frame) {
            updateParticleLifetimes(registry);
        }
        runner.expect(countParticles(registry) == 0,
                      "particles expire by frame lifetime");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        auto& clock = registry.get<HungerClock>(resources.player);
        clock.duration = 2;
        processHunger(registry, resources);
        runner.expect(clock.duration == 1,
                      "hunger decreases after a real player turn");
        processHunger(registry, resources);
        runner.expect(registry.all_of<SufferDamage>(resources.player),
                      "starvation queues gentle damage");

        const auto food = spawnFoodRation(registry, Point{1, 1});
        registry.remove<Position>(food);
        registry.emplace<InBackpack>(food, InBackpack{resources.player});
        clock.duration = 10;
        runner.expect(
            useInventoryItem(registry, resources, map, resources.player, food),
            "food use consumes a turn");
        runner.expect(clock.duration > 10 && !registry.valid(food),
                      "food restores hunger and is consumed");
    }

    {
        auto map = openMap();
        map.setVisible(2, 2, true);
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto scroll = spawnMagicMappingScroll(registry, Point{1, 1});
        registry.remove<Position>(scroll);
        registry.emplace<InBackpack>(scroll, InBackpack{resources.player});

        runner.expect(useInventoryItem(
                          registry, resources, map, resources.player, scroll),
                      "magic mapping scroll consumes a turn");
        runner.expect(map.isRevealed(1, 1) && map.isRevealed(10, 6),
                      "magic mapping reveals terrain memory");
        runner.expect(!map.isVisible(10, 6),
                      "magic mapping does not expand current FOV");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto trap = spawnSpikeTrap(registry, Point{2, 2});
        runner.expect(registry.all_of<Hidden>(trap),
                      "new spike traps start hidden");
        processPlayerTraps(registry, resources);
        runner.expect(!registry.all_of<Hidden>(trap),
                      "triggered traps are revealed");
        runner.expect(registry.get<Trap>(trap).triggered,
                      "triggered trap state is durable data");
        runner.expect(registry.all_of<SufferDamage>(resources.player),
                      "player-only trap trigger queues damage");

        const auto text = serializeGame(registry, resources, openMap());
        entt::registry loadedRegistry;
        GameResources loadedResources;
        Map loadedMap{1, 1};
        runner.expect(
            deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
            "trap save data loads");
        int triggeredTraps = 0;
        const auto traps = loadedRegistry.view<const Trap>();
        for (const auto entity : traps) {
            if (traps.get<const Trap>(entity).triggered) {
                ++triggeredTraps;
            }
        }
        runner.expect(triggeredTraps == 1, "trap state survives save/load");
    }

    {
        int selection = 0;
        const std::vector<InputEvent> enter{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(
            handleMainMenuInput(enter, false, MainMenuMode::Title, selection) ==
                MainMenuAction::NewGame,
            "main menu Enter starts a new game by default");

        const std::vector<InputEvent> quit{{InputEventType::KeyDown, Key::Q}};
        runner.expect(
            handleMainMenuInput(quit, true, MainMenuMode::Title, selection) ==
                MainMenuAction::Quit,
            "main menu q quits");
    }
}

} // namespace rl::test
