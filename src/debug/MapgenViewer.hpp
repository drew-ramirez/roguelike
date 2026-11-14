/**
 * **************************************************************************
 * @file     MapgenViewer.hpp
 * @brief    Declares the CLI map generation inspection harness
 * **************************************************************************
 */

#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"
#include "world/WorldTypes.hpp"

namespace rl {

struct MapgenViewerOptions {
    std::string builderId = "simple";
    std::optional<Rng::Seed> seed;
    bool showSnapshots = false;
    int width = 40;
    int height = 25;
    std::optional<BiomeVariant> biomeVariant;
};

int runMapgenViewer(const MapgenViewerOptions& options, std::ostream& output);

} // namespace rl
