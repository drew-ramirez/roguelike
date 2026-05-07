/**
 * **************************************************************************
 * @file     MapgenPlayback.cpp
 * @brief    Implements snapshot playback for the visual mapgen harness
 * **************************************************************************
 */

#include "debug/MapgenPlayback.hpp"

#include <algorithm>
#include <utility>

namespace rl {
namespace {

[[nodiscard]] bool hasFrames(const MapgenPlayback& playback) noexcept
{
    return !playback.frames.empty();
}

void stepBackward(MapgenPlayback& playback)
{
    if (playback.frameIndex > 0) {
        --playback.frameIndex;
    }
    playback.timerMs = 0;
}

[[nodiscard]] bool stepForward(MapgenPlayback& playback)
{
    if (!hasFrames(playback)) {
        return true;
    }
    if (playback.frameIndex + 1 >= playback.frames.size()) {
        return true;
    }
    ++playback.frameIndex;
    playback.timerMs = 0;
    return false;
}

} // namespace

void resetMapgenPlayback(MapgenPlayback& playback,
                         std::vector<MapSnapshot> frames)
{
    playback.frames = std::move(frames);
    playback.frameIndex = 0;
    playback.timerMs = 0;
    playback.paused = false;
}

const MapSnapshot* currentMapgenFrame(const MapgenPlayback& playback)
{
    if (!hasFrames(playback)) {
        return nullptr;
    }
    const auto index =
        std::min(playback.frameIndex, playback.frames.size() - 1);
    return &playback.frames[index];
}

MapgenPlaybackCommand
advanceMapgenPlayback(MapgenPlayback& playback,
                      const std::vector<InputEvent>& events,
                      int elapsedMs,
                      int frameDurationMs)
{
    if (!hasFrames(playback)) {
        return MapgenPlaybackCommand::Commit;
    }

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (event.key == Key::Escape) {
            return MapgenPlaybackCommand::Cancel;
        }
        if (event.key == Key::Enter) {
            return MapgenPlaybackCommand::Commit;
        }
        if (event.key == Key::Space) {
            playback.paused = !playback.paused;
            playback.timerMs = 0;
        } else if (event.key == Key::Left) {
            stepBackward(playback);
        } else if (event.key == Key::Right) {
            if (stepForward(playback)) {
                return MapgenPlaybackCommand::Commit;
            }
        }
    }

    if (playback.paused) {
        return MapgenPlaybackCommand::None;
    }

    playback.timerMs += std::max(0, elapsedMs);
    if (playback.timerMs >= frameDurationMs) {
        if (stepForward(playback)) {
            return MapgenPlaybackCommand::Commit;
        }
    }

    return MapgenPlaybackCommand::None;
}

} // namespace rl
