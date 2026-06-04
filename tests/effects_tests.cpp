#include "TestSuites.hpp"

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

void runEffectsTests(TestRunner& runner)
{
    auto map = openMap();
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});

    RawEffects rawEffects;
    rawEffects.inflictsDamage = 6;
    rawEffects.areaOfEffect = 1;
    auto queue = effectsQueueForRawEffects(
        rawEffects, resources.player, entt::null, Point{3, 2});
    runner.expect(!queue.empty() && queue.size() == 1,
                  "effects queue wraps raw effect requests");
    runner.expect(queue.requests().front().kind == EffectKind::Damage &&
                      queue.requests().front().amount == 6 &&
                      queue.requests().front().radius == 1,
                  "effects queue exposes existing effect request data");

    EffectsQueue revealQueue;
    revealQueue.push(
        EffectRequest{EffectKind::RevealMap, resources.player, entt::null});
    runner.expect(resolveEffectsQueue(registry, resources, map, revealQueue),
                  "effects queue resolves through the effect system");
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
