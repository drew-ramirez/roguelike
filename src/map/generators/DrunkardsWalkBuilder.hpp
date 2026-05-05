/**
 * **************************************************************************
 * @file     DrunkardsWalkBuilder.hpp
 * @brief    Declares random-walker cave generation
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBrush.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

enum class DrunkSpawnMode { StartingPoint, Random };

struct DrunkardSettings {
    DrunkSpawnMode spawnMode = DrunkSpawnMode::StartingPoint;
    int drunkenLifetime = 400;
    double floorPercent = 0.5;
    int brushSize = 1;
    Symmetry symmetry = Symmetry::None;
};

class DrunkardsWalkBuilder {
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
                      const DrunkardSettings& settings,
                      MapBuildOptions options = MapBuildOptions{});

    [[nodiscard]] static DrunkardSettings openAreaSettings() noexcept;
    [[nodiscard]] static DrunkardSettings openHallsSettings() noexcept;
    [[nodiscard]] static DrunkardSettings windingPassagesSettings() noexcept;
    [[nodiscard]] static DrunkardSettings fatPassagesSettings() noexcept;
    [[nodiscard]] static DrunkardSettings fearfulSymmetrySettings() noexcept;
};

} // namespace rl
