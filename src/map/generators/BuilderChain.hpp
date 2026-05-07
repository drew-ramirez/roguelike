/**
 * **************************************************************************
 * @file     BuilderChain.hpp
 * @brief    Declares composable hybrid map builder chains
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class BuilderChain {
  public:
    [[nodiscard]] static MapBuildResult
    buildCavernVaults(int width,
                      int height,
                      Rng& rng,
                      MapBuildOptions options = MapBuildOptions{});

    [[nodiscard]] static MapBuildResult
    buildRuinedRooms(int width,
                     int height,
                     Rng& rng,
                     MapBuildOptions options = MapBuildOptions{});
};

void placeDoors(MapBuildResult& result,
                Rng& rng,
                MapBuildOptions options,
                bool allowFallbackScan = true);

} // namespace rl
