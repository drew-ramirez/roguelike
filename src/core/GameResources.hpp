#pragma once

#include <entt/entt.hpp>

#include "core/GameLog.hpp"
#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "render/Color.hpp"

#include <optional>
#include <vector>

namespace rl {

struct PendingTargeting {
    entt::entity item = entt::null;
    int range = 0;
    std::optional<int> radius;
    Point cursor;
};

struct ParticleRequest {
    Point position;
    char32_t glyph = U'*';
    Color foreground = Color::white();
    Color background = Color::black();
    int lifetimeFrames = 8;
};

struct GameResources {
    GameResources() : rng(), log() {}

    Rng rng;
    GameLog log;
    entt::entity player = entt::null;
    int depth = 1;
    int inventorySelection = 0;
    std::optional<PendingTargeting> pendingTargeting;
    std::vector<ParticleRequest> particleRequests;
};

} // namespace rl
