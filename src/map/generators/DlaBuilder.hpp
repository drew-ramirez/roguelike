/**
 * **************************************************************************
 * @file     DlaBuilder.hpp
 * @brief    Declares diffusion-limited aggregation cave generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBrush.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Selects how new growth approaches or leaves the existing floor mass. */
enum class DlaAlgorithm { WalkInwards, WalkOutwards, CentralAttractor };

/** Controls DLA growth bias, brush width, symmetry, and target coverage. */
struct DlaSettings {
    DlaAlgorithm algorithm = DlaAlgorithm::WalkInwards;
    int brushSize = 1;
    Symmetry symmetry = Symmetry::None;
    double floorPercent = 0.25;
};

/** Builds organic seeded growth for special caves and erosion layers. */
class DlaBuilder {
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
                      const DlaSettings& settings,
                      MapBuildOptions options = MapBuildOptions{});

    /** Grows branching tendrils inward from random exterior points. */
    [[nodiscard]] static DlaSettings walkInwardsSettings() noexcept;
    /** Expands a wider frontier out from the central seed mass. */
    [[nodiscard]] static DlaSettings walkOutwardsSettings() noexcept;
    /** Produces radial growth suited to a central arena or hoard. */
    [[nodiscard]] static DlaSettings centralAttractorSettings() noexcept;
    /** Mirrors radial growth for an intentionally designed or alien space. */
    [[nodiscard]] static DlaSettings insectoidSettings() noexcept;
};

} // namespace rl
