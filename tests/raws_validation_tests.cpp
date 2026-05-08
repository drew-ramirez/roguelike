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
    runner.expect(defaultRawDatabase().entity("wolf") != nullptr &&
                      defaultRawDatabase().entity("wild_boar") != nullptr,
                  "default raw content includes woods wildlife");

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
