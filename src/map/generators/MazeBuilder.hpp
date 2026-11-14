/**
 * **************************************************************************
 * @file     MazeBuilder.hpp
 * @brief    Declares perfect maze generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Builds an extreme one-tile perfect maze for dedicated labyrinth spaces. */
class MazeBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
