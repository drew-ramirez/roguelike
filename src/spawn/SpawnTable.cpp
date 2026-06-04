#include "spawn/SpawnTable.hpp"

#include <algorithm>
#include <stdexcept>

#include "raws/RawDatabase.hpp"

namespace rl {
std::string
rollSpawn(const std::vector<SpawnEntry>& entries, int depth, Rng& rng)
{
    int totalWeight = 0;
    for (const auto& entry : entries) {
        if (depth >= entry.minDepth && depth <= entry.maxDepth) {
            totalWeight += entry.weight + (entry.addMapDepthToWeight *
                                           std::max(0, depth - entry.minDepth));
        }
    }

    if (totalWeight <= 0) {
        throw std::runtime_error("Spawn table has no entries for this depth");
    }

    int roll = rng.rangeInclusive(1, totalWeight);
    for (const auto& entry : entries) {
        if (depth < entry.minDepth || depth > entry.maxDepth) {
            continue;
        }

        roll -= entry.weight + (entry.addMapDepthToWeight *
                                std::max(0, depth - entry.minDepth));
        if (roll <= 0) {
            return entry.id;
        }
    }

    return entries.front().id;
}

const std::vector<SpawnEntry>& monsterSpawnTable()
{
    return defaultRawDatabase().monsterSpawnTable();
}

const std::vector<SpawnEntry>& itemSpawnTable()
{
    return defaultRawDatabase().itemSpawnTable();
}

int maxMonstersForDepth(int depth)
{
    return 6 + std::min(depth, 6);
}

int maxItemsForDepth(int depth)
{
    return 3 + std::min(depth, 4);
}

} // namespace rl
