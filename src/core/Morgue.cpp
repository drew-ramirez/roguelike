#include "core/Morgue.hpp"

namespace rl {

MorgueSummary buildMorgueSummary(const GameLog& log,
                                 std::size_t recentMessageCount)
{
    MorgueSummary summary;
    summary.kills = log.counter("kills");
    summary.artifactsClaimed = log.counter("artifact_claimed");

    const auto recent = log.recent(recentMessageCount);
    summary.recentMessages.reserve(recent.size());
    for (const auto message : recent) {
        summary.recentMessages.emplace_back(message);
    }
    return summary;
}

} // namespace rl
