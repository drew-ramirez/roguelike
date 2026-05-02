#pragma once

namespace rl {

struct Point {
    int x = 0;
    int y = 0;
};

constexpr Point operator+(Point left, Point right) noexcept
{
    return {left.x + right.x, left.y + right.y};
}

constexpr bool operator==(Point left, Point right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

} // namespace rl
