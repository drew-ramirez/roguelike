/**
 * **************************************************************************
 * @file     ConsoleBuffer.cpp
 * @brief    Stores glyph and color cells for renderer-independent drawing
 * **************************************************************************
 */

#include "render/ConsoleBuffer.hpp"

#include <stdexcept>

namespace rl {

ConsoleBuffer::ConsoleBuffer(int width, int height)
    : width_(width), height_(height),
      cells_(width > 0 && height > 0 ? static_cast<std::size_t>(width * height)
                                     : 0U)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(
            "ConsoleBuffer dimensions must be positive");
    }
}

int ConsoleBuffer::width() const noexcept
{
    return width_;
}

int ConsoleBuffer::height() const noexcept
{
    return height_;
}

void ConsoleBuffer::clear(Color foreground, Color background, char32_t glyph)
{
    for (auto& cell : cells_) {
        cell = Cell{glyph, foreground, background};
    }
}

void ConsoleBuffer::clearTransparent()
{
    clear(Color::white(), Color::black(), transparentCellGlyph);
}

void ConsoleBuffer::setCell(int x, int y, const Cell& cell)
{
    if (!inBounds(x, y)) {
        return;
    }

    cells_[static_cast<std::size_t>(indexOf(x, y))] = cell;
}

void ConsoleBuffer::set(
    int x, int y, char32_t glyph, Color foreground, Color background)
{
    setCell(x, y, Cell{glyph, foreground, background});
}

void ConsoleBuffer::print(
    int x, int y, std::string_view text, Color foreground, Color background)
{
    for (char ch : text) {
        set(x, y, static_cast<unsigned char>(ch), foreground, background);
        ++x;
    }
}

const Cell& ConsoleBuffer::cellAt(int x, int y) const
{
    if (!inBounds(x, y)) {
        throw std::out_of_range(
            "ConsoleBuffer cell coordinate is out of bounds");
    }

    return cells_[static_cast<std::size_t>(indexOf(x, y))];
}

bool ConsoleBuffer::inBounds(int x, int y) const noexcept
{
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

int ConsoleBuffer::indexOf(int x, int y) const
{
    return y * width_ + x;
}

} // namespace rl
