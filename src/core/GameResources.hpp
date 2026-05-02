#pragma once

#include <entt/entt.hpp>

#include "core/Rng.hpp"

namespace rl {

struct GameResources {
    Rng rng;
    entt::entity player = entt::null;
};

} // namespace rl
