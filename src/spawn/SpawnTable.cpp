#include "spawn/SpawnTable.hpp"

#include <algorithm>
#include <stdexcept>

namespace rl {
namespace {

const std::vector<SpawnEntry> monsters{
    SpawnEntry{"goblin", 80, 1},
    SpawnEntry{"orc", 30, 2},
    SpawnEntry{"troll", 12, 3},
};

const std::vector<SpawnEntry> items{
    SpawnEntry{"healing_potion", 70, 1},
    SpawnEntry{"food_ration", 45, 1},
    SpawnEntry{"fireball_scroll", 20, 1},
    SpawnEntry{"magic_mapping_scroll", 12, 1},
    SpawnEntry{"dagger", 20, 1},
    SpawnEntry{"sword", 18, 2},
    SpawnEntry{"chain_mail", 12, 2},
};

} // namespace

std::string
rollSpawn(const std::vector<SpawnEntry>& entries, int depth, Rng& rng)
{
    int totalWeight = 0;
    for (const auto& entry : entries) {
        if (depth >= entry.minDepth) {
            totalWeight += entry.weight;
        }
    }

    if (totalWeight <= 0) {
        throw std::runtime_error("Spawn table has no entries for this depth");
    }

    int roll = rng.rangeInclusive(1, totalWeight);
    for (const auto& entry : entries) {
        if (depth < entry.minDepth) {
            continue;
        }

        roll -= entry.weight;
        if (roll <= 0) {
            return entry.id;
        }
    }

    return entries.front().id;
}

const std::vector<SpawnEntry>& monsterSpawnTable()
{
    return monsters;
}

const std::vector<SpawnEntry>& itemSpawnTable()
{
    return items;
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
