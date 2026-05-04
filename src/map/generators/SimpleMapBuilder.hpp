/**
 * **************************************************************************
 * @file     SimpleMapBuilder.hpp
 * @brief    Declares deterministic rooms-and-corridors map generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class SimpleMapBuilder {
  public:
    /**
     * Builds a connected rooms-and-corridors map from the provided RNG stream
     *
     * @param width The requested map width in tiles
     * @param height The requested map height in tiles
     * @param rng Central project RNG used to keep generation deterministic
     * @return Generated map data, player start, and accepted room list
     */
    [[nodiscard]] static MapBuildResult
    build(int width,
          int height,
          Rng& rng,
          MapBuildOptions options = MapBuildOptions{});
};

} // namespace rl
