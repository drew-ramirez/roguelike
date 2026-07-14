/**
 * **************************************************************************
 * @file     MapgenGallery.cpp
 * @brief    Implements grouped builder browsing and map preview playback
 * **************************************************************************
 */

#include "debug/MapgenGallery.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/MapRenderSystem.hpp"

namespace rl {
namespace {

[[nodiscard]] std::size_t catalogIndexFor(std::string_view id)
{
    const auto catalog = mapBuilderCatalog();
    for (std::size_t index = 0; index < catalog.size(); ++index) {
        if (catalog[index].id == id) {
            return index;
        }
    }
    return 0;
}

void generatePreview(MapgenGalleryState& gallery)
{
    gallery.frames.clear();
    gallery.frameIndex = 0;
    gallery.timerMs = 0;
    gallery.paused = true;
    gallery.validation = MapValidationResult{};
    gallery.metrics = MapQualityMetrics{};
    gallery.error.clear();

    try {
        Rng rng{gallery.seed};
        auto options = gallery.buildOptions;
        options.recordSnapshots = true;
        auto build = buildMapById(selectedMapgenGalleryBuilder(gallery).id,
                                  gallery.width,
                                  gallery.height,
                                  rng,
                                  options);
        gallery.validation = validateMapBuild(
            build.map, build.playerStart, build.stairsPosition);
        gallery.metrics = measureMapQuality(build.map);
        gallery.frames = std::move(build.snapshots);
        gallery.frames.push_back(
            MapSnapshot{"final generated map", std::move(build.map)});
        gallery.frameIndex = gallery.frames.size() - 1U;
    } catch (const std::exception& error) {
        gallery.error = error.what();
    }
}

} // namespace

void openMapgenGallery(MapgenGalleryState& gallery,
                       const MapgenGalleryOptions& options)
{
    gallery = MapgenGalleryState{};
    gallery.seed = options.seed.value_or(Rng::makeSeed());
    gallery.width = options.width;
    gallery.height = options.height;
    gallery.buildOptions = options.buildOptions;
    const std::string_view initialId =
        isKnownMapBuilder(options.initialBuilderId)
            ? std::string_view{options.initialBuilderId}
            : std::string_view{"simple"};
    gallery.selectedCatalogIndex = catalogIndexFor(initialId);
    gallery.pickerCatalogIndex = gallery.selectedCatalogIndex;
    gallery.initialized = true;
    generatePreview(gallery);
}

const MapBuilderDescriptor&
selectedMapgenGalleryBuilder(const MapgenGalleryState& gallery)
{
    return mapBuilderCatalog()[gallery.selectedCatalogIndex];
}

} // namespace rl
