/**
 * **************************************************************************
 * @file     MapBuilderRegistry.hpp
 * @brief    Declares registered map builder lookup and dispatch
 * **************************************************************************
 */

#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

/** Stable gallery grouping without changing a builder's dispatch ID. */
enum class MapBuilderCategory {
    Algorithm,
    Composition,
    Surface,
    DungeonRegion
};

/** Identifies one registered builder and its inspection category. */
struct MapBuilderDescriptor {
    std::string_view id;
    MapBuilderCategory category = MapBuilderCategory::Algorithm;
};

/** Returns every registered builder in grouped display order. */
[[nodiscard]] std::span<const MapBuilderDescriptor>
mapBuilderCatalog() noexcept;
/** Finds catalog metadata without allocating or dispatching a build. */
[[nodiscard]] const MapBuilderDescriptor*
mapBuilderDescriptor(std::string_view builderId) noexcept;
/** Returns the user-facing label for a gallery category. */
[[nodiscard]] std::string_view
mapBuilderCategoryName(MapBuilderCategory category) noexcept;

[[nodiscard]] std::vector<std::string> availableMapBuilders();
[[nodiscard]] bool isKnownMapBuilder(std::string_view builderId);
[[nodiscard]] MapBuildResult buildMapById(std::string_view builderId,
                                          int width,
                                          int height,
                                          Rng& rng,
                                          MapBuildOptions options);
[[nodiscard]] MapBuildResult buildLevelForDepth(
    int depth, int width, int height, Rng& rng, MapBuildOptions options);

} // namespace rl
