/**
 * **************************************************************************
 * @file     DlaBuilder.hpp
 * @brief    Declares diffusion-limited aggregation cave generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class DlaBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
