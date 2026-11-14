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

/** Orders base builders and refinements into named, diagnosable map styles. */
class BuilderChain {
  public:
    /** Layers a depth-appropriate prefab vault over a cellular cave base. */
    [[nodiscard]] static MapBuildResult
    buildCavernVaults(int width,
                      int height,
                      Rng& rng,
                      MapBuildOptions options = MapBuildOptions{});

    /** Layers a shrine, rubble, and doors over the classic room baseline. */
    [[nodiscard]] static MapBuildResult
    buildRuinedRooms(int width,
                     int height,
                     Rng& rng,
                     MapBuildOptions options = MapBuildOptions{});
};

/** Adds doors only where known corridor or fallback topology supports them. */
void placeDoors(MapBuildResult& result,
                Rng& rng,
                MapBuildOptions options,
                bool allowFallbackScan = true);

} // namespace rl
