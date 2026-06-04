/**
 * **************************************************************************
 * @file     Achievements.hpp
 * @brief    Declares progress summaries derived from structured game logs
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include "core/GameLog.hpp"

namespace rl {

struct AchievementProgress {
    std::string id;
    std::string label;
    int progress = 0;
    int goal = 1;
    bool unlocked = false;
};

[[nodiscard]] std::vector<AchievementProgress>
achievementProgress(const GameLog& log);

} // namespace rl
