/**
 * **************************************************************************
 * @file     Morgue.hpp
 * @brief    Declares death and run summary data built from game progress logs
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/GameLog.hpp"

namespace rl {

struct MorgueSummary {
    int kills = 0;
    int artifactsClaimed = 0;
    std::vector<std::string> recentMessages;
};

[[nodiscard]] MorgueSummary
buildMorgueSummary(const GameLog& log, std::size_t recentMessageCount = 8);

} // namespace rl
