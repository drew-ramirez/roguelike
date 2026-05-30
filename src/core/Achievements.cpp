#include "core/Achievements.hpp"

#include <algorithm>

namespace rl {

std::vector<AchievementProgress> achievementProgress(const GameLog& log)
{
    const int kills = log.counter("kills");
    const int artifacts = log.counter("artifact_claimed");
    return {
        AchievementProgress{"first_blood", "First Blood", kills, 1, kills >= 1},
        AchievementProgress{
            "dragon_slayer", "Dragon Slayer", artifacts, 1, artifacts >= 1},
        AchievementProgress{
            "veteran", "Veteran", std::min(kills, 25), 25, kills >= 25},
    };
}

} // namespace rl
