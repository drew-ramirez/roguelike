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

/** Selects whether each digger begins centrally or on reachable frontier. */
enum class DrunkSpawnMode { StartingPoint, Random };

/** Controls coverage, passage width, and symmetry for walker-carved caves. */
struct DrunkardSettings {
    DrunkSpawnMode spawnMode = DrunkSpawnMode::StartingPoint;
    int drunkenLifetime = 400;
    double floorPercent = 0.5;
    int brushSize = 1;
    Symmetry symmetry = Symmetry::None;
};

/** Builds contiguous water-carved caverns with a selected passage identity. */
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

    /** Favors one broad cave grown from the central start. */
    [[nodiscard]] static DrunkardSettings openAreaSettings() noexcept;
    /** Distributes long-lived diggers across reachable open halls. */
    [[nodiscard]] static DrunkardSettings openHallsSettings() noexcept;
    /** Uses short-lived diggers for narrow, winding passages. */
    [[nodiscard]] static DrunkardSettings windingPassagesSettings() noexcept;
    /** Widens winding passages with a larger carving brush. */
    [[nodiscard]] static DrunkardSettings fatPassagesSettings() noexcept;
    /** Mirrors winding excavation across both axes for a designed space. */
    [[nodiscard]] static DrunkardSettings fearfulSymmetrySettings() noexcept;
};

} // namespace rl
