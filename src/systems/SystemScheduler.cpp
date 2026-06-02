/**
 * **************************************************************************
 * @file     SystemScheduler.cpp
 * @brief    Runs gameplay systems in an explicit deterministic order
 * **************************************************************************
 */

#include "systems/SystemScheduler.hpp"

#include <stdexcept>
#include <utility>

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
namespace {

[[nodiscard]] std::vector<SystemPhase> turnResolutionPhaseOrder()
{
    return {SystemPhase::PreTurn,
            SystemPhase::PlayerIntent,
            SystemPhase::Movement,
            SystemPhase::Combat,
            SystemPhase::Effects,
            SystemPhase::Cleanup,
            SystemPhase::Visibility,
            SystemPhase::AITurn,
            SystemPhase::Movement,
            SystemPhase::Combat,
            SystemPhase::Cleanup,
            SystemPhase::RenderPrep};
}

} // namespace

std::vector<SystemPhase> defaultSystemPhaseOrder()
{
    return {SystemPhase::PreTurn,
            SystemPhase::PlayerIntent,
            SystemPhase::AITurn,
            SystemPhase::Movement,
            SystemPhase::Combat,
            SystemPhase::Effects,
            SystemPhase::Cleanup,
            SystemPhase::Visibility,
            SystemPhase::RenderPrep};
}

SystemScheduler::SystemScheduler() : phaseOrder_(defaultSystemPhaseOrder()) {}

SystemScheduler::SystemScheduler(std::vector<SystemPhase> phaseOrder)
    : phaseOrder_(std::move(phaseOrder))
{
    if (phaseOrder_.empty()) {
        throw std::invalid_argument("SystemScheduler needs at least one phase");
    }
}

void SystemScheduler::registerSystem(SystemPhase phase,
                                     std::string name,
                                     SystemTask task)
{
    if (name.empty()) {
        throw std::invalid_argument("Scheduled system name must not be empty");
    }
    if (!task) {
        throw std::invalid_argument("Scheduled system task must be callable");
    }

    systems_.push_back(
        ScheduledSystem{phase, std::move(name), std::move(task)});
}

void SystemScheduler::run() const
{
    for (const auto phase : phaseOrder_) {
        runPhase(phase);
    }
}

void SystemScheduler::runPhase(SystemPhase phase) const
{
    for (const auto& system : systems_) {
        if (system.phase == phase) {
            system.task();
        }
    }
}

std::vector<ScheduledSystemInfo> SystemScheduler::executionPlan() const
{
    std::vector<ScheduledSystemInfo> result;
    result.reserve(systems_.size());
    for (const auto phase : phaseOrder_) {
        for (const auto& system : systems_) {
            if (system.phase == phase) {
                result.push_back(
                    ScheduledSystemInfo{system.phase, system.name});
            }
        }
    }
    return result;
}

void resolveTurnSystems(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        RunState& state,
                        bool runMonsters)
{
    enum class TurnSegment { Player, Monsters, Finished };

    TurnSegment segment = TurnSegment::Finished;
    SystemScheduler scheduler{turnResolutionPhaseOrder()};

    scheduler.registerSystem(SystemPhase::PreTurn, "begin_player_turn", [&] {
        segment = TurnSegment::Player;
        state = RunState::PlayerTurn;
    });
    scheduler.registerSystem(SystemPhase::PlayerIntent, "pickup_items", [&] {
        processPickupItems(registry, resources);
    });
    scheduler.registerSystem(SystemPhase::PlayerIntent, "item_uses", [&] {
        [[maybe_unused]] const bool usedItem =
            processItemUses(registry, resources, map, &fovProvider);
    });
    scheduler.registerSystem(SystemPhase::PlayerIntent, "spell_casts", [&] {
        [[maybe_unused]] const bool castSpell =
            processSpellCasts(registry, resources, map, &fovProvider);
    });
    scheduler.registerSystem(SystemPhase::Combat, "player_ranged_combat", [&] {
        if (segment == TurnSegment::Player) {
            [[maybe_unused]] const bool shot =
                processRangedCombat(registry, resources, map);
        }
    });
    scheduler.registerSystem(SystemPhase::Combat, "player_melee_combat", [&] {
        if (segment == TurnSegment::Player) {
            processMeleeCombat(registry, resources.log, &resources.rng);
        }
    });
    scheduler.registerSystem(SystemPhase::Effects, "player_traps", [&] {
        if (segment == TurnSegment::Player) {
            processPlayerTraps(registry, resources);
        }
    });
    scheduler.registerSystem(SystemPhase::Effects, "hunger", [&] {
        if (segment == TurnSegment::Player) {
            processHunger(registry, resources);
        }
    });
    scheduler.registerSystem(SystemPhase::Cleanup, "damage_cleanup", [&] {
        if (segment != TurnSegment::Finished) {
            applyDamageAndCleanup(registry, resources, state, &map);
        }
    });
    scheduler.registerSystem(SystemPhase::Visibility, "player_visibility", [&] {
        if (segment == TurnSegment::Player) {
            updateVisibility(registry, map, resources.player, fovProvider);
        }
    });
    scheduler.registerSystem(SystemPhase::AITurn, "monster_ai", [&] {
        if (state == RunState::GameOver || !runMonsters) {
            segment = TurnSegment::Finished;
            return;
        }

        segment = TurnSegment::Monsters;
        state = RunState::MonsterTurn;
        runBystanderAi(registry, resources, map);
        runHostileMonsterAi(registry, resources, map);
    });
    scheduler.registerSystem(SystemPhase::Combat, "monster_ranged_combat", [&] {
        if (segment == TurnSegment::Monsters) {
            [[maybe_unused]] const bool monsterShot =
                processRangedCombat(registry, resources, map);
        }
    });
    scheduler.registerSystem(SystemPhase::Combat, "monster_melee_combat", [&] {
        if (segment == TurnSegment::Monsters) {
            processMeleeCombat(registry, resources.log, &resources.rng);
        }
    });
    scheduler.registerSystem(SystemPhase::RenderPrep, "flush_particles", [&] {
        flushParticleRequests(registry, resources);
    });
    scheduler.registerSystem(SystemPhase::RenderPrep, "await_input", [&] {
        if (state != RunState::GameOver) {
            state = RunState::AwaitingInput;
        }
    });

    scheduler.run();
}

} // namespace rl
