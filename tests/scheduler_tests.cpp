#include "TestSuites.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "systems/SystemScheduler.hpp"

namespace rl::test {

void runSchedulerTests(TestRunner& runner)
{
    const auto phases = defaultSystemPhaseOrder();
    const auto findPhase = [&](SystemPhase phase) {
        return std::find(phases.begin(), phases.end(), phase);
    };

    runner.expect(phases.size() == 9U &&
                      findPhase(SystemPhase::PreTurn) <
                          findPhase(SystemPhase::PlayerIntent) &&
                      findPhase(SystemPhase::AITurn) <
                          findPhase(SystemPhase::RenderPrep),
                  "scheduler exposes deterministic turn phases");

    std::vector<std::string> calls;
    SystemScheduler scheduler;
    scheduler.registerSystem(SystemPhase::Combat, "combat-one", [&] {
        calls.push_back("combat-one");
    });
    scheduler.registerSystem(
        SystemPhase::PreTurn, "pre-turn", [&] { calls.push_back("pre-turn"); });
    scheduler.registerSystem(SystemPhase::Combat, "combat-two", [&] {
        calls.push_back("combat-two");
    });
    scheduler.registerSystem(
        SystemPhase::AITurn, "ai-turn", [&] { calls.push_back("ai-turn"); });

    scheduler.run();
    runner.expect(calls ==
                      std::vector<std::string>{
                          "pre-turn", "ai-turn", "combat-one", "combat-two"},
                  "scheduler runs by phase order then registration order");

    calls.clear();
    scheduler.runPhase(SystemPhase::Combat);
    runner.expect(calls == std::vector<std::string>{"combat-one", "combat-two"},
                  "scheduler can run a focused phase for turn sequencing");

    const auto plan = scheduler.executionPlan();
    runner.expect(plan.size() == 4U && plan.front().name == "pre-turn" &&
                      plan.back().name == "combat-two",
                  "scheduler reports deterministic execution plan");
}

} // namespace rl::test
