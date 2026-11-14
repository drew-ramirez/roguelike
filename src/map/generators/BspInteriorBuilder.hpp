/**
 * **************************************************************************
 * @file     BspInteriorBuilder.hpp
 * @brief    Declares BSP generation with simple interior variation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Builds dense constructed interiors from broad, inset BSP leaf rooms. */
class BspInteriorBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
