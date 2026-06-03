/**
 * **************************************************************************
 * @file     Section4Builders.hpp
 * @brief    Declares authored gameplay region map builders
 * **************************************************************************
 */

#pragma once

#include "map/MapBuilderTypes.hpp"

namespace rl {

class Rng;

struct Section4Builders {
    [[nodiscard]] static MapBuildResult
    buildTown(int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult
    buildWoods(int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildLimestoneCaverns(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDeepCaverns(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDwarfTransition(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDragonLair(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildMushroomForest(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDarkElfCity(
        int width, int height, Rng& rng, MapBuildOptions options = {});
};

} // namespace rl
