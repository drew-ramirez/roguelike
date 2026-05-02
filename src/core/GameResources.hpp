#pragma once

#include <entt/entt.hpp>

#include "core/GameLog.hpp"
#include "core/Rng.hpp"

namespace rl {

struct GameResources {
    Rng rng;
    GameLog log;
    entt::entity player = entt::null;
};

} // namespace rl
