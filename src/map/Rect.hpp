#pragma once

#include "core/Point.hpp"

namespace rl {

// x1/y1 are inclusive; x2/y2 are exclusive.
struct Rect {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;

    [[nodiscard]] constexpr int width() const noexcept
    {
        return x2 - x1;
    }

    [[nodiscard]] constexpr int height() const noexcept
    {
        return y2 - y1;
    }

    [[nodiscard]] constexpr Point center() const noexcept
    {
        return {(x1 + x2) / 2, (y1 + y2) / 2};
    }

    [[nodiscard]] constexpr bool intersects(const Rect& other) const noexcept
    {
        return x1 < other.x2 && x2 > other.x1 && y1 < other.y2 && y2 > other.y1;
    }
};

} // namespace rl
