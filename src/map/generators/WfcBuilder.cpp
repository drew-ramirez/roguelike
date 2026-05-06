/**
 * **************************************************************************
 * @file     WfcBuilder.cpp
 * @brief    Implements chunk source helpers for WFC map generation
 * **************************************************************************
 */

#include "map/generators/WfcBuilder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>

#include "assets/RexPaintLoader.hpp"
#include "map/MapConnectivity.hpp"

namespace rl {

namespace wfc {

int directionIndex(Direction direction) noexcept
{
    return static_cast<int>(direction);
}

Direction opposite(Direction direction) noexcept
{
    switch (direction) {
    case Direction::North:
        return Direction::South;
    case Direction::East:
        return Direction::West;
    case Direction::South:
        return Direction::North;
    case Direction::West:
        return Direction::East;
    }
    return Direction::North;
}

TileType tileAt(const SourceMap& source, int x, int y)
{
    if (x < 0 || y < 0 || x >= source.width || y >= source.height) {
        throw std::out_of_range("WFC source map coordinate is out of bounds");
    }
    return source.tiles[static_cast<std::size_t>(y * source.width + x)];
}

SourceMap loadSourceMap(const std::filesystem::path& path)
{
    const auto image = loadRexPaintImage(path);
    SourceMap source;
    source.width = image.width;
    source.height = image.height;
    source.tiles.assign(static_cast<std::size_t>(source.width * source.height),
                        TileType::Wall);

    for (const auto& layer : image.layers) {
        for (int y = 0; y < layer.height; ++y) {
            for (int x = 0; x < layer.width; ++x) {
                const auto& cell = layer.cellAt(x, y);
                if (cell.transparentBackground) {
                    continue;
                }
                source.tiles[static_cast<std::size_t>(y * source.width + x)] =
                    cell.glyph == U'#' ? TileType::Wall : TileType::Floor;
            }
        }
    }

    return source;
}

MapChunk makeChunk(const ChunkPattern& pattern)
{
    MapChunk chunk;
    chunk.pattern = pattern;
    for (auto& mask : chunk.exits) {
        mask.fill(false);
    }

    auto setExit = [&chunk](Direction direction, int offset, TileType tile) {
        const bool passable = isConnectivityPassable(tile);
        chunk.exits[static_cast<std::size_t>(directionIndex(direction))]
                   [static_cast<std::size_t>(offset)] = passable;
        chunk.hasExits = chunk.hasExits || passable;
    };

    for (int offset = 0; offset < chunkSize; ++offset) {
        setExit(Direction::North,
                offset,
                pattern[static_cast<std::size_t>(offset)]);
        setExit(Direction::East,
                offset,
                pattern[static_cast<std::size_t>(offset * chunkSize +
                                                 (chunkSize - 1))]);
        setExit(Direction::South,
                offset,
                pattern[static_cast<std::size_t>((chunkSize - 1) * chunkSize +
                                                 offset)]);
        setExit(Direction::West,
                offset,
                pattern[static_cast<std::size_t>(offset * chunkSize)]);
    }

    return chunk;
}

ChunkPattern flipHorizontal(const ChunkPattern& pattern)
{
    ChunkPattern flipped{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            flipped[static_cast<std::size_t>(y * chunkSize + x)] =
                pattern[static_cast<std::size_t>(y * chunkSize +
                                                 (chunkSize - x - 1))];
        }
    }
    return flipped;
}

ChunkPattern flipVertical(const ChunkPattern& pattern)
{
    ChunkPattern flipped{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            flipped[static_cast<std::size_t>(y * chunkSize + x)] =
                pattern[static_cast<std::size_t>(
                    (chunkSize - y - 1) * chunkSize + x)];
        }
    }
    return flipped;
}

namespace {

[[nodiscard]] ChunkPattern
patternFromSourceChunk(const SourceMap& source, int originX, int originY)
{
    ChunkPattern pattern{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            pattern[static_cast<std::size_t>(y * chunkSize + x)] =
                tileAt(source, originX + x, originY + y);
        }
    }
    return pattern;
}

[[nodiscard]] bool samePattern(const MapChunk& chunk,
                               const ChunkPattern& pattern)
{
    return chunk.pattern == pattern;
}

void addUniqueChunk(std::vector<MapChunk>& chunks, const ChunkPattern& pattern)
{
    const auto exists =
        std::any_of(chunks.begin(), chunks.end(), [&pattern](const auto& c) {
            return samePattern(c, pattern);
        });
    if (!exists) {
        chunks.push_back(makeChunk(pattern));
    }
}

constexpr std::array<Direction, 4> directions{
    Direction::North,
    Direction::East,
    Direction::South,
    Direction::West,
};

[[nodiscard]] bool compatibleEdges(const MapChunk& source,
                                   const MapChunk& neighbor,
                                   Direction direction)
{
    const auto sourceIndex =
        static_cast<std::size_t>(directionIndex(direction));
    const auto neighborIndex =
        static_cast<std::size_t>(directionIndex(opposite(direction)));
    // Exact mask equality prevents half-open chunk joins: every passable edge
    // cell on one chunk must meet a passable edge cell on the neighbor.
    return source.exits[sourceIndex] == neighbor.exits[neighborIndex];
}
} // namespace

std::vector<MapChunk> extractChunks(const std::vector<SourceMap>& sources)
{
    std::vector<MapChunk> chunks;
    for (const auto& source : sources) {
        for (int y = 0; y <= source.height - chunkSize; y += chunkSize) {
            for (int x = 0; x <= source.width - chunkSize; x += chunkSize) {
                const auto pattern = patternFromSourceChunk(source, x, y);
                const auto horizontal = flipHorizontal(pattern);
                const auto vertical = flipVertical(pattern);
                addUniqueChunk(chunks, pattern);
                addUniqueChunk(chunks, horizontal);
                addUniqueChunk(chunks, vertical);
                addUniqueChunk(chunks, flipVertical(horizontal));
            }
        }
    }
    return chunks;
}

void buildCompatibility(std::vector<MapChunk>& chunks)
{
    for (auto& chunk : chunks) {
        for (auto& compatible : chunk.compatibleWith) {
            compatible.clear();
        }
    }

    for (int source = 0; source < static_cast<int>(chunks.size()); ++source) {
        for (int neighbor = 0; neighbor < static_cast<int>(chunks.size());
             ++neighbor) {
            for (const auto direction : directions) {
                if (compatibleEdges(chunks[static_cast<std::size_t>(source)],
                                    chunks[static_cast<std::size_t>(neighbor)],
                                    direction)) {
                    chunks[static_cast<std::size_t>(source)]
                        .compatibleWith[static_cast<std::size_t>(
                            directionIndex(direction))]
                        .push_back(neighbor);
                }
            }
        }
    }
}
} // namespace wfc

} // namespace rl
