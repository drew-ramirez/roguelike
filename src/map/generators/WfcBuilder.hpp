/**
 * **************************************************************************
 * @file     WfcBuilder.hpp
 * @brief    Declares chunk source helpers for WFC map generation
 * **************************************************************************
 */

#pragma once

#include <array>
#include <filesystem>
#include <vector>

#include "map/TileType.hpp"

namespace rl {

namespace wfc {

inline constexpr int chunkSize = 8;
inline constexpr int chunkArea = chunkSize * chunkSize;

enum class Direction { North = 0, East = 1, South = 2, West = 3 };

using ExitMask = std::array<bool, chunkSize>;
using ChunkPattern = std::array<TileType, chunkArea>;

struct SourceMap {
    int width = 0;
    int height = 0;
    std::vector<TileType> tiles;
};

struct MapChunk {
    ChunkPattern pattern{};
    std::array<ExitMask, 4> exits{};
    bool hasExits = false;
    std::array<std::vector<int>, 4> compatibleWith{};
};

[[nodiscard]] int directionIndex(Direction direction) noexcept;
[[nodiscard]] Direction opposite(Direction direction) noexcept;
[[nodiscard]] TileType tileAt(const SourceMap& source, int x, int y);
[[nodiscard]] SourceMap loadSourceMap(const std::filesystem::path& path);
[[nodiscard]] MapChunk makeChunk(const ChunkPattern& pattern);
[[nodiscard]] ChunkPattern flipHorizontal(const ChunkPattern& pattern);
[[nodiscard]] ChunkPattern flipVertical(const ChunkPattern& pattern);
[[nodiscard]] std::vector<MapChunk>
extractChunks(const std::vector<SourceMap>& sources);
void buildCompatibility(std::vector<MapChunk>& chunks);

} // namespace wfc

} // namespace rl
