#include "TestSuites.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "raws/RawDatabase.hpp"
#include "render/ConsoleBuffer.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/inventory/ItemSystem.hpp"
#include "ui/InventoryMenu.hpp"
#include "ui/ItemDetails.hpp"
#include "ui/ItemSelectionMenu.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool lineContains(const std::vector<std::string>& lines,
                                std::string_view text)
{
    return std::any_of(lines.begin(), lines.end(), [text](const auto& line) {
        return line.find(text) != std::string::npos;
    });
}

[[nodiscard]] bool bufferContains(const ConsoleBuffer& console,
                                  std::string_view text)
{
    std::string row;
    row.reserve(static_cast<std::size_t>(console.width()));
    for (int y = 0; y < console.height(); ++y) {
        row.clear();
        for (int x = 0; x < console.width(); ++x) {
            const auto glyph = console.cellAt(x, y).glyph;
            row.push_back(glyph < 128 ? static_cast<char>(glyph) : '?');
        }
        if (row.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void carryItem(entt::registry& registry, entt::entity owner, entt::entity item)
{
    if (registry.all_of<Position>(item)) {
        registry.remove<Position>(item);
    }
    registry.emplace_or_replace<InBackpack>(item, InBackpack{owner});
}

} // namespace

void runItemDetailsTests(TestRunner& runner)
{
    {
        entt::registry registry;
        const auto sword = spawnNamedEntity(
            registry, defaultRawDatabase(), "sword", Point{1, 1});
        const auto lines = itemDetailLines(registry, sword);

        runner.expect(displayItemName(registry, sword) == "Sword",
                      "item detail uses visible item names");
        runner.expect(lineContains(lines, "Slot: weapon") &&
                          lineContains(lines, "Melee: 1d8+1, +2 hit, might") &&
                          lineContains(lines, "Power bonus: +4"),
                      "item detail describes melee equipment stats");
    }

    {
        entt::registry registry;
        const auto potion = spawnHealingPotion(registry, Point{1, 1});
        const auto fireball = spawnNamedEntity(
            registry, defaultRawDatabase(), "fireball_scroll", Point{2, 1});
        const auto spellbook = spawnNamedEntity(
            registry, defaultRawDatabase(), "spark_spellbook", Point{3, 1});

        runner.expect(
            lineContains(itemDetailLines(registry, potion), "Heals 8 HP"),
            "item detail describes consumable healing");
        runner.expect(displayItemName(registry, fireball) == "Orange Scroll",
                      "item detail keeps unidentified magic labels");
        const auto fireballLines = itemDetailLines(registry, fireball);
        runner.expect(
            lineContains(fireballLines,
                         "Ranged: 3d4, range 6, +0 hit, "
                         "intelligence") &&
                lineContains(fireballLines, "Effect: 12 damage in radius 2") &&
                lineContains(fireballLines, "Magic: rare (unidentified)"),
            "item detail describes ranged magic consumables");
        runner.expect(lineContains(itemDetailLines(registry, spellbook),
                                   "Spellbook: teaches spark"),
                      "item detail describes spellbook metadata");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto potion = spawnHealingPotion(registry, Point{1, 1});
        carryItem(registry, resources.player, potion);

        ConsoleBuffer console{80, 50};
        console.clear();
        drawInventoryMenu(console, registry, resources);

        runner.expect(
            bufferContains(console, "Heals 8 HP") &&
                bufferContains(console, "Actions: D drop, U/Enter use"),
            "inventory menu renders selected item details");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto source = spawnNamedEntity(
            registry, defaultRawDatabase(), "identify_scroll", Point{1, 1});
        const auto target = spawnNamedEntity(
            registry, defaultRawDatabase(), "cursed_dagger", Point{1, 2});
        carryItem(registry, resources.player, source);
        carryItem(registry, resources.player, target);
        resources.pendingItemSelection =
            PendingItemSelection{ItemSelectionMode::Identify, source};

        ConsoleBuffer console{80, 50};
        console.clear();
        drawItemSelectionMenu(console, registry, resources);
        runner.expect(
            bufferContains(console, "Old Dagger") &&
                bufferContains(console, "Magic: common (unidentified)"),
            "item selection menu renders selected target details");

        const std::vector<InputEvent> escape{
            {InputEventType::KeyDown, Key::Escape}};
        runner.expect(
            handleItemSelectionInput(registry, resources, map, escape) ==
                ItemSelectionInputResult::Close,
            "item selection Escape cancels");
        runner.expect(!resources.pendingItemSelection.has_value() &&
                          registry.valid(source) &&
                          !registry.get<MagicItem>(target).identified,
                      "item selection cancel preserves source and target");

        resources.pendingItemSelection =
            PendingItemSelection{ItemSelectionMode::Identify, source};
        const std::vector<InputEvent> enter{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(
            handleItemSelectionInput(registry, resources, map, enter) ==
                ItemSelectionInputResult::TookTurn,
            "item selection Enter confirms the target");
        runner.expect(!registry.valid(source) && registry.valid(target) &&
                          registry.get<MagicItem>(target).identified,
                      "item selection confirm consumes identify source");
    }
}

} // namespace rl::test
