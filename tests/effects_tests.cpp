#include "TestSuites.hpp"

#include <string>
#include <utility>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "systems/EffectSystem.hpp"
#include "systems/ItemSystem.hpp"

namespace rl::test {
namespace {

entt::entity carriedConsumable(entt::registry& registry,
                               entt::entity owner,
                               std::string name)
{
    const auto item = registry.create();
    registry.emplace<Item>(item);
    registry.emplace<Consumable>(item);
    registry.emplace<Name>(item, Name{std::move(name)});
    registry.emplace<InBackpack>(item, InBackpack{owner});
    return item;
}

} // namespace

void runEffectsTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;

    resources.player = registry.create();
    registry.emplace<Position>(resources.player, Position{2, 2});
    registry.emplace<CombatStats>(resources.player, CombatStats{20, 8, 0, 1});
    registry.emplace<Pools>(resources.player, Pools{8, 20, 0, 0, 1, 0, 0});
    registry.emplace<HungerClock>(resources.player,
                                  HungerClock{HungerState::Hungry, 500});

    const auto potion = carriedConsumable(registry, resources.player, "Potion");
    registry.emplace<ProvidesHealing>(potion, ProvidesHealing{8});
    runner.expect(useInventoryItem(
                      registry, resources, map, resources.player, potion),
                  "healing items resolve through reusable effects");
    runner.expect(registry.get<CombatStats>(resources.player).hp == 16 &&
                      registry.get<Pools>(resources.player).hitPoints == 16,
                  "healing effects update combat stats and pools");
    runner.expect(!registry.valid(potion),
                  "successful consumable effects destroy the item");

    const auto ration = carriedConsumable(registry, resources.player, "Ration");
    registry.emplace<ProvidesNutrition>(ration, ProvidesNutrition{300});
    runner.expect(useInventoryItem(
                      registry, resources, map, resources.player, ration),
                  "nutrition items resolve through reusable effects");
    runner.expect(registry.get<HungerClock>(resources.player).duration == 800,
                  "nutrition effects feed the hunger clock");

    const auto target = registry.create();
    registry.emplace<Position>(target, Position{3, 2});
    registry.emplace<CombatStats>(target, CombatStats{12, 12, 0, 1});
    EffectsQueue combatQueue;
    combatQueue.push(EffectRequest{
        EffectKind::Damage, resources.player, Point{3, 2}, 5, 0});
    runner.expect(combatQueue.resolve(registry, resources, map),
                  "targeted effects resolve from a queue");
    runner.expect(registry.get<SufferDamage>(target).amount == 5,
                  "damage effects queue damage on targets");

    EffectsQueue revealQueue;
    revealQueue.push(EffectRequest{EffectKind::RevealMap, resources.player});
    runner.expect(revealQueue.resolve(registry, resources, map),
                  "reveal effects resolve from a queue");
    runner.expect(map.isRevealed(1, 1),
                  "resolved reveal effect mutates the map");
}

} // namespace rl::test
