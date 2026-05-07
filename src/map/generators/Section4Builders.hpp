/**
 * **************************************************************************
 * @file     Section4Builders.hpp
 * @brief    Declares authored gameplay region map builders
 * **************************************************************************
 */

#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

struct Section4Builders {
    [[nodiscard]] static MapBuildResult
    buildTown(int width, int height, Rng& rng, MapBuildOptions options = {});
};

} // namespace rl
