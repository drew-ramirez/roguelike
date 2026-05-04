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

namespace rl {

struct MapgenViewerOptions {
    std::string builderId = "simple";
    std::optional<Rng::Seed> seed;
    bool showSnapshots = false;
};

[[nodiscard]] std::vector<std::string> availableMapBuilders();
[[nodiscard]] bool isKnownMapBuilder(std::string_view builderId);
[[nodiscard]] MapBuildResult buildMapById(std::string_view builderId,
                                          int width,
                                          int height,
                                          Rng& rng,
                                          MapBuildOptions options);

int runMapgenViewer(const MapgenViewerOptions& options, std::ostream& output);

} // namespace rl
