/**
 * **************************************************************************
 * @file     BspDungeonBuilder.hpp
 * @brief    Declares binary-space-partition dungeon generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class BspDungeonBuilder {
  public:
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
