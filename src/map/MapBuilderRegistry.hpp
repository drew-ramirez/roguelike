/**
 * **************************************************************************
 * @file     MapBuilderRegistry.hpp
 * @brief    Declares registered map builder lookup and dispatch
 * **************************************************************************
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

[[nodiscard]] std::vector<std::string> availableMapBuilders();
[[nodiscard]] bool isKnownMapBuilder(std::string_view builderId);
[[nodiscard]] MapBuildResult buildMapById(std::string_view builderId,
                                          int width,
                                          int height,
                                          Rng& rng,
                                          MapBuildOptions options);

} // namespace rl
