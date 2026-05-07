/**
 * **************************************************************************
 * @file     MapgenPlayback.hpp
 * @brief    Declares snapshot playback state for the visual mapgen harness
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <vector>

#include "input/InputEvent.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

enum class MapgenPlaybackCommand { None, Commit, Cancel };

struct MapgenPlayback {
    std::vector<MapSnapshot> frames;
    std::size_t frameIndex = 0;
    int timerMs = 0;
    bool paused = false;
};

void resetMapgenPlayback(MapgenPlayback& playback,
                         std::vector<MapSnapshot> frames);

[[nodiscard]] const MapSnapshot*
currentMapgenFrame(const MapgenPlayback& playback);

[[nodiscard]] MapgenPlaybackCommand
advanceMapgenPlayback(MapgenPlayback& playback,
                      const std::vector<InputEvent>& events,
                      int elapsedMs,
                      int frameDurationMs = 300);

} // namespace rl
