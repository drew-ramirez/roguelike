/**
 * **************************************************************************
 * @file     MapgenGallery.hpp
 * @brief    Declares the interactive, view-only map builder gallery
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/Rng.hpp"
#include "input/InputEvent.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MapBuilderTypes.hpp"
#include "map/MapQualityMetrics.hpp"
#include "map/MapValidation.hpp"

namespace rl {

class ConsoleBuffer;

enum class MapgenGalleryCommand { None, ReturnToTitle };

struct MapgenGalleryOptions {
    std::string initialBuilderId = "simple";
    std::optional<Rng::Seed> seed;
    int width = 80;
    int height = 43;
    MapBuildOptions buildOptions;
};

struct MapgenGalleryState {
    Rng::Seed seed = 0;
    int width = 80;
    int height = 43;
    MapBuildOptions buildOptions;
    std::size_t selectedCatalogIndex = 0;
    std::size_t pickerCatalogIndex = 0;
    std::vector<MapSnapshot> frames;
    std::size_t frameIndex = 0;
    int timerMs = 0;
    bool paused = true;
    bool pickerOpen = false;
    bool initialized = false;
    MapValidationResult validation;
    MapQualityMetrics metrics;
    std::string error;
};

/** Opens or resets the gallery and generates its initial final-map preview. */
void openMapgenGallery(MapgenGalleryState& gallery,
                       const MapgenGalleryOptions& options);

/** Updates gallery navigation and deterministic snapshot playback. */
[[nodiscard]] MapgenGalleryCommand
updateMapgenGallery(MapgenGalleryState& gallery,
                    const std::vector<InputEvent>& events,
                    int elapsedMs,
                    int frameDurationMs = 300);

/** Draws either the grouped builder picker or current map preview. */
void drawMapgenGallery(ConsoleBuffer& console,
                       const MapgenGalleryState& gallery);

[[nodiscard]] const MapBuilderDescriptor&
selectedMapgenGalleryBuilder(const MapgenGalleryState& gallery);
[[nodiscard]] const MapBuilderDescriptor&
pickerMapgenGalleryBuilder(const MapgenGalleryState& gallery);
[[nodiscard]] const MapSnapshot*
currentMapgenGalleryFrame(const MapgenGalleryState& gallery);

} // namespace rl
