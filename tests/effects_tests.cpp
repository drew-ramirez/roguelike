#include "TestSuites.hpp"

#include <string>
#include <utility>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EffectSystem.hpp"
#include "systems/EquipmentSystem.hpp"
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

    RawEffects rawEffects;
    rawEffects.inflictsDamage = 6;
    rawEffects.areaOfEffect = 1;
    auto rawQueue = effectsQueueForRawEffects(
        rawEffects, resources.player, entt::null, Point{3, 2});
    runner.expect(!rawQueue.empty() && rawQueue.size() == 1,
                  "effects queue wraps raw effect requests");
    runner.expect(rawQueue.requests().front().kind == EffectKind::Damage &&
                      rawQueue.requests().front().amount == 6 &&
                      rawQueue.requests().front().radius == 1,
                  "effects queue exposes existing request data");

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
    combatQueue.push(EffectRequest{
        EffectKind::Confusion, resources.player, Point{3, 2}, 4, 0});
    runner.expect(combatQueue.resolve(registry, resources, map),
                  "targeted effects resolve from a queue");
    runner.expect(registry.get<SufferDamage>(target).amount == 5,
                  "damage effects queue damage on targets");
    runner.expect(registry.get<Confusion>(target).turns == 4,
                  "confusion effects mark the target");

    EffectsQueue revealQueue;
    revealQueue.push(EffectRequest{EffectKind::RevealMap, resources.player});
    runner.expect(resolveEffectsQueue(registry, resources, map, revealQueue),
                  "reveal effects resolve from a queue");
    runner.expect(map.isRevealed(1, 1),
                  "resolved reveal effect mutates the map");

    const auto cursed = spawnNamedEntity(
        registry, defaultRawDatabase(), "cursed_dagger", Point{1, 1});
    registry.remove<Position>(cursed);
    registry.emplace<InBackpack>(cursed, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, cursed),
        "cursed item can be equipped");
    runner.expect(
        !equipOrUnequipItem(registry, resources.log, resources.player, cursed),
        "cursed equipped item cannot be removed");

    const auto identify = spawnNamedEntity(
        registry, defaultRawDatabase(), "identify_scroll", Point{1, 1});
    registry.remove<Position>(identify);
    registry.emplace<InBackpack>(identify, InBackpack{resources.player});
    runner.expect(
        useInventoryItem(registry, resources, map, resources.player, identify),
        "identify scroll resolves through effect system");
    runner.expect(registry.get<MagicItem>(cursed).identified &&
                      registry.all_of<IdentifiedItem>(cursed),
                  "identify effect reveals a carried magic item");
    runner.expect(!registry.valid(identify),
                  "successful identify consumes its scroll");

    const auto removeCurse = spawnNamedEntity(
        registry, defaultRawDatabase(), "remove_curse_scroll", Point{1, 1});
    registry.remove<Position>(removeCurse);
    registry.emplace<InBackpack>(removeCurse, InBackpack{resources.player});
    runner.expect(useInventoryItem(
                      registry, resources, map, resources.player, removeCurse),
                  "remove curse scroll resolves through effect system");
    runner.expect(!registry.all_of<CursedItem>(cursed),
                  "remove curse strips the curse component");
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, cursed),
        "uncursed item can be removed");
}

} // namespace rl::test
