#include "TestSuites.hpp"

#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/RawSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/RangedCombatSystem.hpp"

namespace rl::test {

void runRangedCombatTests(TestRunner& runner)
{
    auto map = openMap();
    revealAll(map);
    entt::registry registry;
    GameResources resources;
    resources.rng.reseed(123U);
    resources.player = spawnPlayer(registry, Point{2, 2});

    const auto bow = spawnNamedEntity(
        registry, defaultRawDatabase(), "shortbow", Point{1, 1});
    registry.remove<Position>(bow);
    registry.emplace<InBackpack>(bow, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, bow),
        "ranged weapon equips");

    const std::vector<InputEvent> fire{{InputEventType::KeyDown, Key::F}};
    runner.expect(handleMapInput(registry, resources, map, fire) ==
                      PlayerInputResult::StartedTargeting,
                  "F starts ranged targeting with an equipped weapon");
    runner.expect(resources.pendingTargeting.has_value() &&
                      resources.pendingTargeting->mode == TargetingMode::Shoot &&
                      resources.pendingTargeting->item == bow,
                  "ranged targeting records the equipped weapon");

    resources.pendingTargeting->cursor = Point{5, 2};
    runner.expect(confirmPendingTarget(registry, resources, map),
                  "confirming a ranged target queues a shot");
    runner.expect(registry.all_of<WantsToShoot>(resources.player),
                  "confirmed ranged target creates a shot intent");

    const auto goblin = spawnGoblin(registry, Point{5, 2});
    runner.expect(processRangedCombat(registry, resources, map),
                  "ranged combat resolves a clear shot");
    runner.expect(registry.all_of<SufferDamage>(goblin),
                  "ranged attack queues target damage");
    runner.expect(registry.valid(bow), "ranged attack does not consume ammo");
}

} // namespace rl::test
