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

enum class VoronoiDistance { Pythagoras, Manhattan, Chebyshev };

struct VoronoiSettings {
    int seedCount = 64;
    VoronoiDistance distance = VoronoiDistance::Pythagoras;
};

class VoronoiBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});

    [[nodiscard]] static MapBuildResult
    buildWithSettings(int width,
                      int height,
                      Rng& rng,
                      const VoronoiSettings& settings,
                      MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
