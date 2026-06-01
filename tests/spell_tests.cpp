#include "TestSuites.hpp"

#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/SpellSystem.hpp"
#include "ui/SpellMenu.hpp"

namespace rl::test {

void runSpellTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});
    registry.emplace<KnownSpells>(resources.player,
                                  KnownSpells{{"spark", "heal"}});
    registry.get<Pools>(resources.player).mana = 6;
    const auto goblin = spawnGoblin(registry, Point{4, 2});

    runner.expect(
        castKnownSpell(
            registry, resources, map, resources.player, "spark", Point{4, 2}),
        "known ranged spell casts at a visible target");
    runner.expect(registry.get<Pools>(resources.player).mana == 4,
                  "successful spell spends mana");
    runner.expect(registry.all_of<SufferDamage>(goblin),
                  "spell queues damage through effects");

    runner.expect(
        !castKnownSpell(
            registry, resources, map, resources.player, "spark", Point{10, 6}),
        "invalid spell target is rejected");
    runner.expect(registry.get<Pools>(resources.player).mana == 4,
                  "invalid target does not spend mana");

    {
        auto menuMap = openMap();
        revealAll(menuMap);
        entt::registry menuRegistry;
        GameResources menuResources;
        menuResources.player = spawnPlayer(menuRegistry, Point{2, 2});
        menuRegistry.emplace<KnownSpells>(menuResources.player,
                                          KnownSpells{{"spark", "heal"}});

        const std::vector<InputEvent> open{{InputEventType::KeyDown, Key::C}};
        runner.expect(handleMapInput(
                          menuRegistry, menuResources, menuMap, open) ==
                          PlayerInputResult::OpenSpellMenu,
                      "C opens the spell menu from map input");
        runner.expect(menuResources.spellSelection == 0,
                      "opening the spell menu resets selection");

        const std::vector<InputEvent> confirm{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(handleSpellInput(
                          menuRegistry, menuResources, menuMap, confirm) ==
                          SpellInputResult::StartedTargeting,
                      "ranged spells start targeting from the spell menu");
        runner.expect(menuResources.pendingTargeting.has_value() &&
                          menuResources.pendingTargeting->mode ==
                              TargetingMode::Spell &&
                          menuResources.pendingTargeting->spellId == "spark" &&
                          menuResources.pendingTargeting->range == 6,
                      "spell targeting records the selected spell");

        menuResources.pendingTargeting->cursor = Point{4, 2};
        runner.expect(confirmPendingTarget(
                          menuRegistry, menuResources, menuMap),
                      "confirming a spell target queues the cast");
        runner.expect(menuRegistry.all_of<WantsToCastSpell>(
                          menuResources.player),
                      "spell target confirmation queues a cast intent");
        const auto& intent =
            menuRegistry.get<WantsToCastSpell>(menuResources.player);
        runner.expect(intent.spellId == "spark" && intent.target.has_value() &&
                          intent.target->x == 4 && intent.target->y == 2,
                      "queued spell cast stores the chosen target");
    }

    {
        auto selfMap = openMap();
        entt::registry selfRegistry;
        GameResources selfResources;
        selfResources.player = spawnPlayer(selfRegistry, Point{2, 2});
        selfRegistry.emplace<KnownSpells>(selfResources.player,
                                          KnownSpells{{"heal"}});
        const std::vector<InputEvent> confirm{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(handleSpellInput(
                          selfRegistry, selfResources, selfMap, confirm) ==
                          SpellInputResult::TookTurn,
                      "self spells queue directly from the spell menu");
        const auto& intent =
            selfRegistry.get<WantsToCastSpell>(selfResources.player);
        runner.expect(intent.spellId == "heal" && !intent.target.has_value(),
                      "self spell intents do not require a target");
    }
}

} // namespace rl::test
