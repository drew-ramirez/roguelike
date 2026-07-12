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

/** Selects rounded, diamond-like, or square-like Voronoi cell geometry. */
enum class VoronoiDistance { Pythagoras, Manhattan, Chebyshev };

/** Controls cell density and boundary geometry for zoning or hive layouts. */
struct VoronoiSettings {
    int seedCount = 64;
    VoronoiDistance distance = VoronoiDistance::Pythagoras;
};

/** Builds explicit cells for zoning, grouping, and faction separation. */
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
