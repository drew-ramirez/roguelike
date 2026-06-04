/**
 * **************************************************************************
 * @file     SpawnTable.hpp
 * @brief    Declares centralized depth-aware spawn table helpers
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include "core/Rng.hpp"

namespace rl {

struct SpawnEntry {
    std::string id;
    int weight = 1;
    int minDepth = 1;
    int maxDepth = 1000;
    int addMapDepthToWeight = 0;
};

[[nodiscard]] std::string
rollSpawn(const std::vector<SpawnEntry>& entries, int depth, Rng& rng);

[[nodiscard]] const std::vector<SpawnEntry>& monsterSpawnTable();
[[nodiscard]] const std::vector<SpawnEntry>& itemSpawnTable();

[[nodiscard]] int maxMonstersForDepth(int depth);
[[nodiscard]] int maxItemsForDepth(int depth);

} // namespace rl
