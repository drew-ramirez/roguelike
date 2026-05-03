#include "ui/Widgets.hpp"

#include <algorithm>

#include "render/ConsoleBuffer.hpp"

namespace rl {

void drawPanel(ConsoleBuffer& console, int top, int height)
{
    for (int y = top; y < top + height && y < console.height(); ++y) {
        for (int x = 0; x < console.width(); ++x) {
            const char32_t glyph = y == top ? U'-' : U' ';
            console.set(x, y, glyph, Color{120, 120, 120, 255}, Color::black());
        }
    }
}

void drawBar(ConsoleBuffer& console, int x, int y, int width, int current, int maximum, Color fill, Color empty)
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
