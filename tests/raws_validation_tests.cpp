#include "TestSuites.hpp"

#include <exception>

#include <entt/entt.hpp>

#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/SpawnTable.hpp"

namespace rl::test {

void runRawsValidationTests(TestRunner& runner)
{
    const auto raws = RawDatabase::fromJson(R"({
      "mobs": [{
        "id": "goblin",
        "name": "Goblin",
        "renderable": {"glyph": "g", "fg": "#6EDC6E", "bg": "#000000", "order": 5},
        "blocks_tile": true,
        "combat": {"max_hp": 8, "hp": 8, "defense": 0, "power": 3},
        "xp_reward": 25
      }],
      "items": [{
        "id": "healing_potion",
        "name": "Healing Potion",
        "renderable": {"glyph": "!", "fg": "#FF00FF", "bg": "#000000", "order": 2},
        "consumable": {"effects": {"provides_healing": "8"}}
      }],
      "props": [{
        "id": "spike_trap",
        "name": "Spike Trap",
        "renderable": {"glyph": "^", "fg": "#AAAAAA", "bg": "#000000", "order": 1},
        "effects": {"inflicts_damage": 3},
        "hidden": true
      }],
      "monster_spawn_table": [{"name": "goblin", "weight": 1, "min_depth": 1}],
      "item_spawn_table": [{"name": "healing_potion", "weight": 1, "min_depth": 1}]
    })");

    entt::registry registry;
    const auto goblin = spawnRawEntity(registry, raws, "goblin", Point{2, 2});
    runner.expect(goblin.has_value(), "raw goblin spawns by id");
    runner.expect(registry.all_of<Position,
                                  Renderable,
                                  Name,
                                  Monster,
                                  BlocksTile,
                                  CombatStats,
                                  Pools,
                                  XpReward>(*goblin),
                  "raw goblin has expected gameplay components");

    const auto potion =
        spawnRawEntity(registry, raws, "Healing Potion", Point{3, 2});
    runner.expect(potion.has_value(), "raw item spawns by display alias");
    runner.expect(registry.all_of<Item, Consumable, ProvidesHealing>(*potion),
                  "raw potion has healing components");

    const auto trap = spawnRawEntity(registry,
                                     raws,
                                     "spike_trap",
                                     Point{4, 2},
                                     RawSpawnOptions{false, true});
    runner.expect(trap.has_value(), "raw trap spawns");
    runner.expect(!registry.all_of<Hidden>(*trap),
                  "raw spawn options can reveal traps");

    const auto missing = spawnRawEntity(registry, raws, "missing", Point{1, 1});
    runner.expect(!missing.has_value(), "unknown raw id returns empty");

    Rng first{1U};
    Rng second{1U};
    runner.expect(rollSpawn(raws.monsterSpawnTable(), 1, first) ==
                      rollSpawn(raws.monsterSpawnTable(), 1, second),
                  "raw spawn table rolls are deterministic");
    const auto& defaults = defaultRawDatabase();
    runner.expect(defaults.entity("wolf") != nullptr &&
                      defaults.entity("wild_boar") != nullptr &&
                      defaults.entity("kobold") != nullptr &&
                      defaults.entity("cave_bat") != nullptr &&
                      defaults.entity("limestone_column") != nullptr,
                  "default raw content includes early region wildlife");
    runner.expect(defaults.entity("deep_kobold") != nullptr &&
                      defaults.entity("stone_golem") != nullptr &&
                      defaults.entity("dwarf_sentry") != nullptr &&
                      defaults.entity("sporeling") != nullptr &&
                      defaults.entity("fungal_giant") != nullptr &&
                      defaults.entity("dark_elf_marksman_a") != nullptr &&
                      defaults.entity("dark_elf_blade_b") != nullptr,
                  "default raw content includes late region mobs");
    runner.expect(defaults.entity("deep_limestone_column") != nullptr &&
                      defaults.entity("dwarf_rubble") != nullptr &&
                      defaults.entity("mushroom_patch") != nullptr &&
                      defaults.entity("confusion_spore") != nullptr &&
                      defaults.entity("gauntlets_of_might") != nullptr &&
                      defaults.entity("hand_crossbow") != nullptr &&
                      defaults.entity("scimitar") != nullptr &&
                      defaults.entity("buckler") != nullptr,
                  "default raw content includes late region props and loot");

    const auto crossbow =
        spawnRawEntity(registry, defaults, "hand_crossbow", Point{5, 2});
    runner.expect(crossbow.has_value() &&
                      registry.all_of<Item, RangedWeapon>(*crossbow),
                  "raw hand crossbow spawns as a ranged item");

    const auto marksman =
        spawnRawEntity(registry, defaults, "dark_elf_marksman_a", Point{6, 2});
    runner.expect(
        marksman.has_value() &&
            registry.all_of<Monster, Faction, RangedWeapon>(*marksman),
        "raw dark elf marksman spawns with faction and ranged data");

    bool rejectedBadReference = false;
    try {
        (void)RawDatabase::fromJson(R"({
          "items": [{
            "id": "potion",
            "name": "Potion",
            "renderable": {"glyph": "!", "fg": "#FF00FF", "bg": "#000000", "order": 2}
          }],
          "item_spawn_table": [{"name": "missing", "weight": 1, "min_depth": 1}]
        })");
    } catch (const std::exception&) {
        rejectedBadReference = true;
    }
    runner.expect(rejectedBadReference,
                  "raw validation rejects missing spawn references");
}

} // namespace rl::test
