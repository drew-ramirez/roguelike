#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/combat/SpellSystem.hpp"
#include "systems/effects/EffectSystem.hpp"
#include "systems/inventory/EquipmentSystem.hpp"
#include "systems/inventory/ItemSystem.hpp"

namespace rl::test {

void runEffectsTests(TestRunner& runner)
{
    auto map = openMap();
    entt::registry registry;
    GameResources resources;
    resources.player = spawnPlayer(registry, Point{2, 2});

    revealAll(map);
    const auto first = spawnGoblin(registry, Point{3, 2});
    const auto second = spawnGoblin(registry, Point{4, 2});
    const auto outside = spawnGoblin(registry, Point{7, 2});
    RawEffects rawEffects;
    rawEffects.inflictsDamage = 6;
    rawEffects.areaOfEffect = 1;
    auto queue = EffectsQueue::forRawEffects(
        rawEffects, resources.player, entt::null, Point{3, 2});
    runner.expect(queue.resolve(registry, resources, map),
                  "raw damage resolves at a visible target");
    runner.expect(registry.all_of<SufferDamage>(first) &&
                      registry.all_of<SufferDamage>(second) &&
                      registry.get<SufferDamage>(first).amount == 6 &&
                      registry.get<SufferDamage>(second).amount == 6 &&
                      !registry.all_of<SufferDamage>(outside),
                  "raw damage affects only targets inside its radius");
    runner.expect(!queue.resolve(registry, resources, map) &&
                      registry.all_of<SufferDamage>(first) &&
                      registry.get<SufferDamage>(first).amount == 6,
                  "resolving drains the queue");

    auto hiddenMap = openMap();
    EffectsQueue revealQueue;
    revealQueue.enqueue(
        EffectRequest{EffectKind::RevealMap, resources.player, entt::null});
    runner.expect(revealQueue.resolve(registry, resources, hiddenMap),
                  "effects queue resolves reveal");
    runner.expect(hiddenMap.isRevealed(1, 1),
                  "resolved reveal effect mutates the map");

    const auto firstMagic = spawnNamedEntity(
        registry, defaultRawDatabase(), "cursed_dagger", Point{1, 1});
    const auto selectedMagic = spawnNamedEntity(
        registry, defaultRawDatabase(), "cursed_dagger", Point{1, 1});
    for (const auto item : {firstMagic, selectedMagic}) {
        registry.remove<Position>(item);
        registry.emplace<InBackpack>(item, InBackpack{resources.player});
    }
    const auto selectedScroll = spawnNamedEntity(
        registry, defaultRawDatabase(), "identify_scroll", Point{1, 1});
    registry.remove<Position>(selectedScroll);
    registry.emplace<InBackpack>(selectedScroll, InBackpack{resources.player});
    runner.expect(useInventoryItemOnTargetItem(registry,
                                               resources,
                                               map,
                                               resources.player,
                                               selectedScroll,
                                               selectedMagic),
                  "identify scroll resolves on selected item");
    runner.expect(registry.get<MagicItem>(selectedMagic).identified &&
                      !registry.get<MagicItem>(firstMagic).identified &&
                      !registry.valid(selectedScroll),
                  "selected item is identified and scroll is consumed");
    registry.destroy(firstMagic);
    registry.destroy(selectedMagic);

    registry.emplace<KnownSpells>(resources.player, KnownSpells{{"heal"}});
    auto& pools = registry.get<Pools>(resources.player);
    pools.mana = 6;
    runner.expect(
        !castKnownSpell(
            registry, resources, map, resources.player, "heal", std::nullopt) &&
            pools.mana == 6,
        "failed healing spell does not spend mana");

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
