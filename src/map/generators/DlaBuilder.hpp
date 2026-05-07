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

enum class DlaAlgorithm { WalkInwards, WalkOutwards, CentralAttractor };

struct DlaSettings {
    DlaAlgorithm algorithm = DlaAlgorithm::WalkInwards;
    int brushSize = 1;
    Symmetry symmetry = Symmetry::None;
    double floorPercent = 0.25;
};

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

    [[nodiscard]] static DlaSettings walkInwardsSettings() noexcept;
    [[nodiscard]] static DlaSettings walkOutwardsSettings() noexcept;
    [[nodiscard]] static DlaSettings centralAttractorSettings() noexcept;
    [[nodiscard]] static DlaSettings insectoidSettings() noexcept;
};

} // namespace rl
