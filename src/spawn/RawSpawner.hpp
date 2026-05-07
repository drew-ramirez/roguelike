/**
 * **************************************************************************
 * @file     RawSpawner.hpp
 * @brief    Declares raw-template entity construction helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string_view>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "raws/RawDatabase.hpp"

namespace rl {

struct RawSpawnOptions {
    std::optional<bool> hidden;
    std::optional<bool> trapTriggered;
};

[[nodiscard]] std::optional<entt::entity>
spawnRawEntity(entt::registry& registry,
               const RawDatabase& raws,
               std::string_view name,
               Point position,
               RawSpawnOptions options = {});

[[nodiscard]] entt::entity spawnNamedEntity(entt::registry& registry,
                                            const RawDatabase& raws,
                                            std::string_view name,
                                            Point position,
                                            RawSpawnOptions options = {});

} // namespace rl
