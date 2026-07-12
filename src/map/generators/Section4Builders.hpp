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

/** Builds routed regions whose authored layout outranks algorithm purity. */
struct Section4Builders {
    [[nodiscard]] static MapBuildResult
    buildTown(int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult
    buildWoods(int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildTownOutskirts(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceCardinalTransition(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceCornerWilderness(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceRuinPocket(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceCaveMouth(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceCampShrine(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceResourceHeavy(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildSurfaceDangerEncounter(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildLimestoneCaverns(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDeepCaverns(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDwarfTransition(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDragonLair(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildMushroomEntrance(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildMushroomForest(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildMushroomExit(
        int width, int height, Rng& rng, MapBuildOptions options = {});
    [[nodiscard]] static MapBuildResult buildDarkElfCity(
        int width, int height, Rng& rng, MapBuildOptions options = {});
};

} // namespace rl
