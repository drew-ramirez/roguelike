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

constexpr std::array<MapBuilderCategory, 4> categories{
    MapBuilderCategory::Algorithm,
    MapBuilderCategory::Composition,
    MapBuilderCategory::Surface,
    MapBuilderCategory::DungeonRegion,
};

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

[[nodiscard]] std::size_t firstIndexIn(MapBuilderCategory category)
{
    const auto catalog = mapBuilderCatalog();
    for (std::size_t index = 0; index < catalog.size(); ++index) {
        if (catalog[index].category == category) {
            return index;
        }
    }
    return 0;
}

[[nodiscard]] std::vector<std::size_t> indicesIn(MapBuilderCategory category)
{
    std::vector<std::size_t> indices;
    const auto catalog = mapBuilderCatalog();
    for (std::size_t index = 0; index < catalog.size(); ++index) {
        if (catalog[index].category == category) {
            indices.push_back(index);
        }
    }
    return indices;
}

[[nodiscard]] std::size_t adjacentBuilder(std::size_t current, int direction)
{
    const auto catalog = mapBuilderCatalog();
    const auto category = catalog[current].category;
    const auto indices = indicesIn(category);
    const auto found =
        std::find(indices.begin(), indices.end(), current) - indices.begin();
    const auto count = static_cast<int>(indices.size());
    const int next = (static_cast<int>(found) + direction + count) % count;
    return indices[static_cast<std::size_t>(next)];
}

[[nodiscard]] MapBuilderCategory adjacentCategory(MapBuilderCategory current,
                                                  int direction)
{
    const auto found = std::find(categories.begin(), categories.end(), current);
    const int position = static_cast<int>(found - categories.begin());
    const int count = static_cast<int>(categories.size());
    const int next = (position + direction + count) % count;
    return categories[static_cast<std::size_t>(next)];
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

void selectAdjacentBuilder(MapgenGalleryState& gallery, int direction)
{
    gallery.selectedCatalogIndex =
        adjacentBuilder(gallery.selectedCatalogIndex, direction);
    gallery.pickerCatalogIndex = gallery.selectedCatalogIndex;
    generatePreview(gallery);
}

void chooseNewSeed(MapgenGalleryState& gallery)
{
    const auto previous = gallery.seed;
    do {
        gallery.seed = Rng::makeSeed();
    } while (gallery.seed == previous);
    generatePreview(gallery);
}

void stepFrame(MapgenGalleryState& gallery, int direction)
{
    if (gallery.frames.empty()) {
        return;
    }
    const int count = static_cast<int>(gallery.frames.size());
    const int current = static_cast<int>(gallery.frameIndex);
    gallery.frameIndex =
        static_cast<std::size_t>(std::clamp(current + direction, 0, count - 1));
    gallery.paused = true;
    gallery.timerMs = 0;
}

void showFinalFrame(MapgenGalleryState& gallery)
{
    if (!gallery.frames.empty()) {
        gallery.frameIndex = gallery.frames.size() - 1U;
    }
    gallery.paused = true;
    gallery.timerMs = 0;
}

void togglePlayback(MapgenGalleryState& gallery)
{
    if (gallery.frames.empty()) {
        return;
    }
    if (gallery.paused && gallery.frameIndex + 1U == gallery.frames.size()) {
        gallery.frameIndex = 0;
    }
    gallery.paused = !gallery.paused;
    gallery.timerMs = 0;
}

void updatePlayback(MapgenGalleryState& gallery,
                    int elapsedMs,
                    int frameDurationMs)
{
    if (gallery.paused || gallery.frames.empty()) {
        return;
    }
    gallery.timerMs += std::max(0, elapsedMs);
    while (!gallery.paused && gallery.timerMs >= frameDurationMs) {
        gallery.timerMs -= frameDurationMs;
        if (gallery.frameIndex + 1U >= gallery.frames.size()) {
            gallery.paused = true;
            gallery.timerMs = 0;
        } else {
            ++gallery.frameIndex;
        }
    }
}

void drawCentered(ConsoleBuffer& console,
                  int y,
                  std::string_view text,
                  Color color)
{
    const int x =
        std::max(0, (console.width() - static_cast<int>(text.size())) / 2);
    console.print(x, y, std::string{text}, color);
}

void drawPicker(ConsoleBuffer& console, const MapgenGalleryState& gallery)
{
    const auto& selected = pickerMapgenGalleryBuilder(gallery);
    drawCentered(console, 1, "Map Generation Gallery", Color::yellow());
    drawCentered(console,
                 3,
                 "< " + std::string{mapBuilderCategoryName(selected.category)} +
                     " >",
                 Color{120, 220, 255, 255});

    int row = 5;
    for (const auto index : indicesIn(selected.category)) {
        const auto& descriptor = mapBuilderCatalog()[index];
        const bool highlighted = index == gallery.pickerCatalogIndex;
        drawCentered(console,
                     row,
                     std::string{highlighted ? "> " : "  "} +
                         std::string{descriptor.id},
                     highlighted ? Color::yellow() : Color{190, 190, 190, 255});
        ++row;
    }
    drawCentered(console,
                 console.height() - 2,
                 "Left/Right category  Up/Down builder  Enter preview",
                 Color{170, 170, 170, 255});
    drawCentered(console,
                 console.height() - 1,
                 "Esc close picker",
                 Color{170, 170, 170, 255});
}

void drawPreview(ConsoleBuffer& console, const MapgenGalleryState& gallery)
{
    constexpr int statusRows = 4;
    const auto* frame = currentMapgenGalleryFrame(gallery);
    if (frame != nullptr) {
        Camera camera{console.width(),
                      std::max(1, console.height() - statusRows)};
        camera.centerOn(Point{frame->map.width() / 2, frame->map.height() / 2},
                        frame->map);
        renderMapDebug(frame->map, console, camera);
    } else {
        drawCentered(console,
                     console.height() / 2,
                     "Generation failed: " + gallery.error,
                     Color{255, 100, 100, 255});
    }

    const auto& builder = selectedMapgenGalleryBuilder(gallery);
    const std::string frameStatus =
        gallery.frames.empty()
            ? "no frames"
            : "frame " + std::to_string(gallery.frameIndex + 1U) + "/" +
                  std::to_string(gallery.frames.size());
    const std::string status =
        "Gallery [" + std::string{mapBuilderCategoryName(builder.category)} +
        "] " + std::string{builder.id} + " seed " +
        std::to_string(gallery.seed) + " " + frameStatus;
    console.print(1, console.height() - statusRows, status, Color::yellow());

    std::ostringstream quality;
    quality << (gallery.validation.valid ? "valid" : "invalid") << std::fixed
            << std::setprecision(1) << " coverage "
            << gallery.metrics.walkableCoverage * 100.0 << "% regions "
            << gallery.metrics.disconnectedRegions + 1 << " chokepoints "
            << gallery.metrics.chokepoints;
    console.print(1,
                  console.height() - 3,
                  quality.str(),
                  gallery.validation.valid ? Color{120, 220, 150, 255}
                                           : Color{255, 100, 100, 255});
    console.print(1,
                  console.height() - 2,
                  "Up/Down builder  Left/Right frame  Space play  Enter final",
                  Color{170, 170, 170, 255});
    console.print(1,
                  console.height() - 1,
                  "N new seed  B builders  Esc title",
                  Color{170, 170, 170, 255});
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

MapgenGalleryCommand updateMapgenGallery(MapgenGalleryState& gallery,
                                         const std::vector<InputEvent>& events,
                                         int elapsedMs,
                                         int frameDurationMs)
{
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (gallery.pickerOpen) {
            if (event.key == Key::Escape) {
                gallery.pickerOpen = false;
            } else if (event.key == Key::Left || event.key == Key::Right) {
                const int direction = event.key == Key::Left ? -1 : 1;
                const auto category = adjacentCategory(
                    pickerMapgenGalleryBuilder(gallery).category, direction);
                gallery.pickerCatalogIndex = firstIndexIn(category);
            } else if (event.key == Key::Up || event.key == Key::Down) {
                const int direction = event.key == Key::Up ? -1 : 1;
                gallery.pickerCatalogIndex =
                    adjacentBuilder(gallery.pickerCatalogIndex, direction);
            } else if (event.key == Key::Enter) {
                gallery.selectedCatalogIndex = gallery.pickerCatalogIndex;
                gallery.pickerOpen = false;
                generatePreview(gallery);
            }
            continue;
        }

        if (event.key == Key::Escape) {
            return MapgenGalleryCommand::ReturnToTitle;
        }
        if (event.key == Key::B) {
            gallery.pickerCatalogIndex = gallery.selectedCatalogIndex;
            gallery.pickerOpen = true;
        } else if (event.key == Key::Up || event.key == Key::Down) {
            selectAdjacentBuilder(gallery, event.key == Key::Up ? -1 : 1);
        } else if (event.key == Key::Left || event.key == Key::Right) {
            stepFrame(gallery, event.key == Key::Left ? -1 : 1);
        } else if (event.key == Key::Space) {
            togglePlayback(gallery);
        } else if (event.key == Key::Enter) {
            showFinalFrame(gallery);
        } else if (event.key == Key::N) {
            chooseNewSeed(gallery);
        }
    }

    updatePlayback(gallery, elapsedMs, std::max(1, frameDurationMs));
    return MapgenGalleryCommand::None;
}

void drawMapgenGallery(ConsoleBuffer& console,
                       const MapgenGalleryState& gallery)
{
    console.clear();
    if (gallery.pickerOpen) {
        drawPicker(console, gallery);
    } else {
        drawPreview(console, gallery);
    }
}

const MapBuilderDescriptor&
selectedMapgenGalleryBuilder(const MapgenGalleryState& gallery)
{
    return mapBuilderCatalog()[gallery.selectedCatalogIndex];
}

const MapBuilderDescriptor&
pickerMapgenGalleryBuilder(const MapgenGalleryState& gallery)
{
    return mapBuilderCatalog()[gallery.pickerCatalogIndex];
}

const MapSnapshot* currentMapgenGalleryFrame(const MapgenGalleryState& gallery)
{
    if (gallery.frames.empty()) {
        return nullptr;
    }
    const auto index = std::min(gallery.frameIndex, gallery.frames.size() - 1U);
    return &gallery.frames[index];
}

} // namespace rl
