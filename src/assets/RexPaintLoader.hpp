/**
 * **************************************************************************
 * @file     RexPaintLoader.hpp
 * @brief    Declares project-owned REX Paint .xp loading helpers
 * **************************************************************************
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "render/Color.hpp"

namespace rl {

class RexPaintError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

struct RexPaintCell {
    char32_t glyph = U' ';
    Color foreground = Color::white();
    Color background = Color::black();
    bool transparentBackground = false;
};

struct RexPaintLayer {
    int width = 0;
    int height = 0;
    std::vector<RexPaintCell> cells;

    [[nodiscard]] const RexPaintCell& cellAt(int x, int y) const;
};

struct RexPaintImage {
    std::int32_t version = 0;
    int width = 0;
    int height = 0;
    std::vector<RexPaintLayer> layers;

    [[nodiscard]] const RexPaintCell& cellAt(int layer, int x, int y) const;
};

[[nodiscard]] bool isTransparentRexPaintBackground(Color color) noexcept;
[[nodiscard]] RexPaintImage
loadRexPaintImage(const std::filesystem::path& path);

} // namespace rl
