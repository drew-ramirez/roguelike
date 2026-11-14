/**
 * **************************************************************************
 * @file     CellularAutomataBuilder.hpp
 * @brief    Declares cave generation by cellular automata smoothing
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Builds broad natural cave masses, then joins and refines their regions. */
class CellularAutomataBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
