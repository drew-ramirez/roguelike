/**
 * **************************************************************************
 * @file     ConsoleBuffer.hpp
 * @brief    Declares the logical console buffer used before presentation
 * **************************************************************************
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "render/Color.hpp"

namespace rl {

struct Cell {
    char32_t glyph = U' ';
    Color foreground = Color::white();
    Color background = Color::black();
};

class ConsoleBuffer {
  public:
    ConsoleBuffer(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    void clear(Color foreground = Color::white(),
               Color background = Color::black(),
               char32_t glyph = U' ');
    void set(int x,
             int y,
             char32_t glyph,
             Color foreground = Color::white(),
             Color background = Color::black());
    void print(int x,
               int y,
               std::string_view text,
               Color foreground = Color::white(),
               Color background = Color::black());

    [[nodiscard]] const Cell& cellAt(int x, int y) const;

  private:
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;
    [[nodiscard]] int indexOf(int x, int y) const;

    int width_ = 0;
    int height_ = 0;
    std::vector<Cell> cells_;
};

} // namespace rl
