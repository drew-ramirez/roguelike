#include "TestSuites.hpp"

#include "core/Achievements.hpp"
#include "core/GameLog.hpp"
#include "core/Morgue.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool sameColor(Color left, Color right) noexcept
{
    return left.r == right.r && left.g == right.g && left.b == right.b &&
           left.a == right.a;
}

} // namespace

void runLogTests(TestRunner& runner)
{
    GameLog log{4};
    log.add(LogCategory::Combat, "hit");
    log.add("plain");
    log.incrementCounter("kills");
    log.incrementCounter("kills", 2);
    log.addFragments(LogCategory::Items,
                     {LogFragment{"You pick up ", Color{180, 180, 180, 255}},
                      LogFragment{"a scroll", Color::yellow()}});

    const auto entries = log.entries();
    runner.expect(entries.size() == 3 &&
                      entries.front().category == LogCategory::Combat,
                  "structured log keeps categories");
    runner.expect(log.recent(1).front() == "You pick up a scroll",
                  "recent message compatibility remains");
    runner.expect(entries.back().fragments.size() == 2 &&
                      sameColor(entries.back().fragments.back().foreground,
                                Color::yellow()),
                  "structured log keeps colored message fragments");
    runner.expect(log.counter("kills") == 3,
                  "log progress counters accumulate");

    auto restoredEntries = entries;
    restoredEntries.back().sequence = 12;
    GameLog restored{4};
    restored.replaceEntries(restoredEntries);
    restored.add("after");
    const auto restoredLog = restored.entries();
    runner.expect(restoredLog.back().sequence == 13,
                  "restored log entries keep sequence continuity");

    const auto morgue = buildMorgueSummary(log, 2);
    runner.expect(morgue.kills == 3 && morgue.recentMessages.size() == 2,
                  "morgue summary derives run counters and recent messages");
    const auto achievements = achievementProgress(log);
    runner.expect(!achievements.empty() && achievements.front().unlocked,
                  "achievement summary derives progress from log counters");
}

} // namespace rl::test
