/**
 * **************************************************************************
 * @file     RexPaintLoader.cpp
 * @brief    Loads gzip-compressed REX Paint .xp image data
 * **************************************************************************
 */

#include "assets/RexPaintLoader.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include <zlib.h>

namespace rl {
namespace {

constexpr int gzipBufferSize = 8192;
constexpr int maxRexPaintLayers = 9;

struct GzipCloser {
    void operator()(gzFile_s* file) const noexcept
    {
        if (file != nullptr) {
            static_cast<void>(gzclose(file));
        }
    }
};

[[nodiscard]] std::string pathString(const std::filesystem::path& path)
{
    return path.string();
}

[[nodiscard]] std::vector<std::uint8_t>
decompressGzipFile(const std::filesystem::path& path)
{
    const std::string fileName = pathString(path);
    std::unique_ptr<gzFile_s, GzipCloser> file{gzopen(fileName.c_str(), "rb")};
    if (!file) {
        throw RexPaintError{"Unable to open REX Paint file: " + fileName};
    }

    std::vector<std::uint8_t> bytes;
    std::array<std::uint8_t, gzipBufferSize> buffer{};
    while (true) {
        const int read = gzread(file.get(),
                                buffer.data(),
                                static_cast<unsigned int>(buffer.size()));
        if (read < 0) {
            int errorCode = Z_OK;
            const char* const errorText = gzerror(file.get(), &errorCode);
            throw RexPaintError{
                std::string{"Unable to decompress REX Paint file: "} +
                (errorText == nullptr ? "unknown zlib error" : errorText)};
        }
        if (read == 0) {
            break;
        }
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + read);
    }

    if (bytes.empty()) {
        throw RexPaintError{"REX Paint file has no decompressed data: " +
                            fileName};
    }
    return bytes;
}

class BinaryReader {
  public:
    explicit BinaryReader(std::vector<std::uint8_t> bytes)
        : bytes_(std::move(bytes))
    {
    }

    [[nodiscard]] std::int32_t readInt32()
    {
        constexpr std::size_t int32Size = 4;
        ensureRemaining(int32Size);
        const std::uint32_t value =
            static_cast<std::uint32_t>(bytes_[offset_]) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 8U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 16U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 3U]) << 24U);
        offset_ += int32Size;
        return static_cast<std::int32_t>(value);
    }

    [[nodiscard]] std::uint8_t readByte()
    {
        ensureRemaining(1U);
        const auto value = bytes_[offset_];
        ++offset_;
        return value;
    }

  private:
    void ensureRemaining(std::size_t count) const
    {
        if (count > bytes_.size() - offset_) {
            throw RexPaintError{"REX Paint file ended unexpectedly"};
        }
    }

    std::vector<std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

[[nodiscard]] std::size_t checkedCellCount(int width, int height)
{
    if (width <= 0 || height <= 0) {
        throw RexPaintError{"REX Paint layer dimensions must be positive"};
    }

    const auto cellCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (cellCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw RexPaintError{"REX Paint layer is too large"};
    }
    return cellCount;
}

[[nodiscard]] RexPaintCell readCell(BinaryReader& reader)
{
    const std::int32_t glyph = reader.readInt32();
    RexPaintCell cell;
    cell.glyph = glyph >= 0 ? static_cast<char32_t>(glyph) : U'?';
    cell.foreground =
        Color{reader.readByte(), reader.readByte(), reader.readByte(), 255};
    cell.background =
        Color{reader.readByte(), reader.readByte(), reader.readByte(), 255};
    cell.transparentBackground =
        isTransparentRexPaintBackground(cell.background);
    return cell;
}

} // namespace

const RexPaintCell& RexPaintLayer::cellAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width || y >= height) {
        throw RexPaintError{"REX Paint cell coordinate is out of bounds"};
    }
    return cells[static_cast<std::size_t>(y * width + x)];
}

const RexPaintCell& RexPaintImage::cellAt(int layer, int x, int y) const
{
    if (layer < 0 || layer >= static_cast<int>(layers.size())) {
        throw RexPaintError{"REX Paint layer index is out of bounds"};
    }
    return layers[static_cast<std::size_t>(layer)].cellAt(x, y);
}

bool isTransparentRexPaintBackground(Color color) noexcept
{
    return color.r == 255U && color.g == 0U && color.b == 255U;
}

RexPaintImage loadRexPaintImage(const std::filesystem::path& path)
{
    BinaryReader reader{decompressGzipFile(path)};
    RexPaintImage image;
    image.version = reader.readInt32();

    const std::int32_t layerCount = reader.readInt32();
    if (layerCount <= 0 || layerCount > maxRexPaintLayers) {
        throw RexPaintError{"REX Paint layer count is invalid"};
    }
    image.layers.reserve(static_cast<std::size_t>(layerCount));

    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
        RexPaintLayer layer;
        layer.width = static_cast<int>(reader.readInt32());
        layer.height = static_cast<int>(reader.readInt32());
        const auto cellCount = checkedCellCount(layer.width, layer.height);

        if (layerIndex == 0) {
            image.width = layer.width;
            image.height = layer.height;
        } else if (layer.width != image.width || layer.height != image.height) {
            throw RexPaintError{
                "REX Paint layers must use matching dimensions"};
        }

        layer.cells.resize(cellCount);
        for (std::size_t index = 0; index < cellCount; ++index) {
            const int x = static_cast<int>(
                index / static_cast<std::size_t>(layer.height));
            const int y = static_cast<int>(
                index % static_cast<std::size_t>(layer.height));
            layer.cells[static_cast<std::size_t>(y * layer.width + x)] =
                readCell(reader);
        }
        image.layers.push_back(std::move(layer));
    }

    return image;
}

} // namespace rl
