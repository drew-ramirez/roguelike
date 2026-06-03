/**
 * **************************************************************************
 * @file     SystemScheduler.hpp
 * @brief    Declares deterministic single-threaded turn phase dispatch
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <entt/entt.hpp>

#include "core/RunState.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;
class Rng;

enum class SystemPhase {
    PreTurn,
    PlayerIntent,
    AITurn,
    Movement,
    Combat,
    Effects,
    Cleanup,
    Visibility,
    RenderPrep
};

struct ScheduledSystemInfo {
    SystemPhase phase = SystemPhase::PreTurn;
    std::string name;
    std::size_t registrationOrder = 0;
};

class SystemScheduler {
  public:
    using SystemTask = std::function<void()>;

    SystemScheduler();
    explicit SystemScheduler(std::vector<SystemPhase> phaseOrder);

    void registerSystem(SystemPhase phase, std::string name, SystemTask task);

    void run() const;
    void runPhase(SystemPhase phase) const;

    [[nodiscard]] std::vector<SystemPhase> phaseOrder() const;
    [[nodiscard]] std::vector<ScheduledSystemInfo> registeredSystems() const;
    [[nodiscard]] std::vector<ScheduledSystemInfo> executionPlan() const;

  private:
    struct ScheduledSystem {
        SystemPhase phase = SystemPhase::PreTurn;
        std::string name;
        SystemTask task;
        std::size_t registrationOrder = 0;
    };

    std::vector<SystemPhase> phaseOrder_;
    std::vector<ScheduledSystem> systems_;
};

[[nodiscard]] std::vector<SystemPhase> defaultSystemPhaseOrder();
[[nodiscard]] std::string_view systemPhaseName(SystemPhase phase) noexcept;

void resolveTurnSystems(entt::registry& registry,
                        GameResources& resources,
                        Map& map,
                        IFovProvider& fovProvider,
                        RunState& state,
                        bool runMonsters);

} // namespace rl
