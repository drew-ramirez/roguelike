/**
 * **************************************************************************
 * @file     VoronoiBuilder.hpp
 * @brief    Declares Voronoi hive map generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class VoronoiBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
