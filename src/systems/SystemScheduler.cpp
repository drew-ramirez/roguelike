/**
 * **************************************************************************
 * @file     SystemScheduler.cpp
 * @brief    Runs gameplay systems in an explicit deterministic order
 * **************************************************************************
 */

#include "systems/SystemScheduler.hpp"

#include "core/GameResources.hpp"
#include "fov/IFovProvider.hpp"
#include "map/Map.hpp"
#include "systems/BystanderAI.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"
#include "systems/MonsterAI.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/RangedCombatSystem.hpp"
#include "systems/SpellSystem.hpp"
#include "systems/TrapSystem.hpp"
#include "systems/VisibilitySystem.hpp"

namespace rl {
void resolveTurnSystems(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        RunState& state,
                        bool runMonsters)
{
    state = RunState::PlayerTurn;
    processPickupItems(registry, resources);
    [[maybe_unused]] const bool usedItem =
        processItemUses(registry, resources, map, &fovProvider);
    [[maybe_unused]] const bool castSpell =
        processSpellCasts(registry, resources, map, &fovProvider);
    [[maybe_unused]] const bool shot =
        processRangedCombat(registry, resources, map);
    processMeleeCombat(registry, resources.log, &resources.rng);
    processPlayerTraps(registry, resources);
    processHunger(registry, resources);
    applyDamageAndCleanup(registry, resources, state, &map);
    updateVisibility(registry, map, resources.player, fovProvider);

    if (state != RunState::GameOver && runMonsters) {
        state = RunState::MonsterTurn;
        runBystanderAi(registry, resources, map);
        runHostileMonsterAi(registry, resources, map);
        [[maybe_unused]] const bool monsterShot =
            processRangedCombat(registry, resources, map);
        processMeleeCombat(registry, resources.log, &resources.rng);
        applyDamageAndCleanup(registry, resources, state, &map);
    }

    flushParticleRequests(registry, resources);
    if (state != RunState::GameOver) {
        state = RunState::AwaitingInput;
    }
}

} // namespace rl
