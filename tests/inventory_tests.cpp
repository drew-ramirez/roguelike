#include "TestSuites.hpp"

#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/ItemSystem.hpp"
#include "ui/InventoryMenu.hpp"

namespace rl::test {

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

        const std::vector<InputEvent> use{{InputEventType::KeyDown, Key::U}};
        runner.expect(handleInventoryInput(registry, resources, map, use) ==
                          InventoryInputResult::TookTurn,
                      "inventory u uses selected consumable");
        runner.expect(!registry.valid(potion),
                      "valid healing use consumes the potion");
        runner.expect(registry.get<CombatStats>(resources.player).hp == 28,
                      "healing clamps through normal stat update");
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

        const std::vector<InputEvent> escape{
            {InputEventType::KeyDown, Key::Escape}};
        runner.expect(handleInventoryInput(registry, resources, map, escape) ==
                          InventoryInputResult::Close,
                      "Escape closes inventory context");
    }
}

} // namespace rl::test
