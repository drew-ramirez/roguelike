#include "ui/Widgets.hpp"

#include <algorithm>

#include "render/ConsoleBuffer.hpp"

namespace rl {

std::string clippedLabel(std::string text, int width)
{
    if (width <= 0) {
        return {};
    }
    const auto maximum = static_cast<std::size_t>(width);
    if (text.size() <= maximum) {
        return text;
    }
    if (maximum <= 3U) {
        text.resize(maximum);
        return text;
    }
    text.resize(maximum - 3U);
    text += "...";
    return text;
}

void drawMenuFrame(ConsoleBuffer& console, int x, int y, int width, int height)
{
    for (int row = y; row < y + height; ++row) {
        for (int column = x; column < x + width; ++column) {
            const bool border = column == x || column == x + width - 1 ||
                                row == y || row == y + height - 1;
            console.set(column,
                        row,
                        border ? U'#' : U' ',
                        Color{180, 180, 180, 255},
                        Color::black());
        }
    }
}

void drawPanel(ConsoleBuffer& console, int top, int height)
{
    for (int y = top; y < top + height && y < console.height(); ++y) {
        for (int x = 0; x < console.width(); ++x) {
            const char32_t glyph = y == top ? U'-' : U' ';
            console.set(x, y, glyph, Color{120, 120, 120, 255}, Color::black());
        }
    }
}

void drawBar(ConsoleBuffer& console,
             int x,
             int y,
             int width,
             int current,
             int maximum,
             Color fill,
             Color empty)
{
    const int clampedMaximum = std::max(1, maximum);
    const int clampedCurrent = std::clamp(current, 0, clampedMaximum);
    const int filled = clampedCurrent * width / clampedMaximum;

    for (int offset = 0; offset < width; ++offset) {
        const Color foreground = offset < filled ? fill : empty;
        console.set(x + offset, y, U'#', foreground, Color::black());
    }
}

} // namespace rl
