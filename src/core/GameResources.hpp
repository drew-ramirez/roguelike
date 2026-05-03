#pragma once

#include <entt/entt.hpp>

#include "core/GameLog.hpp"
#include "core/Point.hpp"
#include "core/Rng.hpp"

#include <optional>

namespace rl {

struct PendingTargeting {
    entt::entity item = entt::null;
    int range = 0;
    std::optional<int> radius;
    Point cursor;
};

struct GameResources {
    Rng rng;
    GameLog log;
    entt::entity player = entt::null;
    int depth = 1;
    int inventorySelection = 0;
    std::optional<PendingTargeting> pendingTargeting;
};

} // namespace rl
