#include "TestSuites.hpp"

#include <cstddef>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/ItemSystem.hpp"
#include "ui/InventoryMenu.hpp"
#include "ui/ItemSelectionMenu.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool selectInventoryItem(const entt::registry& registry,
                                       GameResources& resources,
                                       entt::entity item)
{
    const auto items = inventoryItems(registry, resources.player);
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (items[index] == item) {
            resources.inventorySelection = static_cast<int>(index);
            return true;
        }
    }
    return false;
}

} // namespace

void runInventoryTests(TestRunner& runner)
{
    auto map = openMap();

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto potion = spawnHealingPotion(registry, Point{2, 2});

        runner.expect(queuePickupItemAtPlayer(registry, resources),
                      "g queues pickup on a ground item");
        processPickupItems(registry, resources);
        runner.expect(!registry.all_of<Position>(potion),
                      "pickup removes item Position");
        runner.expect(registry.all_of<InBackpack>(potion),
                      "pickup adds InBackpack");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto potion = spawnHealingPotion(registry, Point{1, 1});
        registry.remove<Position>(potion);
        registry.emplace<InBackpack>(potion, InBackpack{resources.player});
        registry.get<CombatStats>(resources.player).hp = 20;
        registry.get<Pools>(resources.player).hitPoints = 20;

        const std::vector<InputEvent> use{{InputEventType::KeyDown, Key::U}};
        runner.expect(handleInventoryInput(registry, resources, map, use) ==
                          InventoryInputResult::TookTurn,
                      "inventory u uses selected consumable");
        runner.expect(!registry.valid(potion),
                      "valid healing use consumes the potion");
        runner.expect(registry.get<CombatStats>(resources.player).hp == 28,
                      "healing clamps through normal stat update");
        runner.expect(registry.get<Pools>(resources.player).hitPoints == 28,
                      "healing updates pools alongside combat HP");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto potion = spawnHealingPotion(registry, Point{1, 1});
        registry.remove<Position>(potion);
        registry.emplace<InBackpack>(potion, InBackpack{resources.player});

        const std::vector<InputEvent> use{{InputEventType::KeyDown, Key::U}};
        runner.expect(handleInventoryInput(registry, resources, map, use) ==
                          InventoryInputResult::None,
                      "full-health potion use does not consume a turn");
        runner.expect(registry.valid(potion),
                      "full-health potion use preserves the item");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto dagger = spawnDagger(registry, Point{1, 1});
        registry.remove<Position>(dagger);
        registry.emplace<InBackpack>(dagger, InBackpack{resources.player});

        const std::vector<InputEvent> equip{{InputEventType::KeyDown, Key::E}};
        runner.expect(handleInventoryInput(registry, resources, map, equip) ==
                          InventoryInputResult::TookTurn,
                      "inventory e equips selected gear");
        runner.expect(registry.all_of<Equipped>(dagger),
                      "equipped item receives Equipped");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto dagger = spawnDagger(registry, Point{1, 1});
        registry.remove<Position>(dagger);
        registry.emplace<InBackpack>(dagger, InBackpack{resources.player});

        const std::vector<InputEvent> drop{{InputEventType::KeyDown, Key::D}};
        runner.expect(handleInventoryInput(registry, resources, map, drop) ==
                          InventoryInputResult::TookTurn,
                      "inventory d drops selected item");
        runner.expect(registry.all_of<Position>(dagger),
                      "dropped item returns to the map");
        runner.expect(!registry.all_of<InBackpack>(dagger),
                      "dropped item leaves backpack");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto cursed = spawnNamedEntity(
            registry, defaultRawDatabase(), "cursed_dagger", Point{1, 1});
        registry.remove<Position>(cursed);
        registry.emplace<InBackpack>(cursed, InBackpack{resources.player});
        const auto identify = spawnNamedEntity(
            registry, defaultRawDatabase(), "identify_scroll", Point{1, 2});
        registry.remove<Position>(identify);
        registry.emplace<InBackpack>(identify, InBackpack{resources.player});

        runner.expect(selectInventoryItem(registry, resources, identify),
                      "test can select identify scroll");
        const std::vector<InputEvent> use{{InputEventType::KeyDown, Key::U}};
        runner.expect(handleInventoryInput(registry, resources, map, use) ==
                          InventoryInputResult::StartedItemSelection,
                      "identify scroll opens selected-item targeting");
        runner.expect(resources.pendingItemSelection.has_value() &&
                          resources.pendingItemSelection->source == identify,
                      "identify selection records the source scroll");

        const std::vector<InputEvent> confirm{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(handleItemSelectionInput(
                          registry, resources, map, confirm) ==
                          ItemSelectionInputResult::TookTurn,
                      "confirming item selection uses the scroll");
        runner.expect(registry.get<MagicItem>(cursed).identified,
                      "selected identify reveals the chosen item");
        runner.expect(!registry.valid(identify),
                      "selected identify consumes the scroll");
        runner.expect(!resources.pendingItemSelection.has_value(),
                      "successful item selection clears pending state");

        const auto removeCurse = spawnNamedEntity(registry,
                                                  defaultRawDatabase(),
                                                  "remove_curse_scroll",
                                                  Point{1, 3});
        registry.remove<Position>(removeCurse);
        registry.emplace<InBackpack>(removeCurse,
                                     InBackpack{resources.player});
        runner.expect(selectInventoryItem(registry, resources, removeCurse),
                      "test can select remove-curse scroll");
        runner.expect(handleInventoryInput(registry, resources, map, use) ==
                          InventoryInputResult::StartedItemSelection,
                      "remove-curse scroll opens selected-item targeting");
        runner.expect(handleItemSelectionInput(
                          registry, resources, map, confirm) ==
                          ItemSelectionInputResult::TookTurn,
                      "confirming curse selection uses the scroll");
        runner.expect(!registry.all_of<CursedItem>(cursed),
                      "selected remove curse strips the curse");
        runner.expect(!registry.valid(removeCurse),
                      "selected remove curse consumes the scroll");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});

        const std::vector<InputEvent> escape{
            {InputEventType::KeyDown, Key::Escape}};
        runner.expect(handleInventoryInput(registry, resources, map, escape) ==
                          InventoryInputResult::Close,
                      "Escape closes inventory context");
    }
}

} // namespace rl::test
