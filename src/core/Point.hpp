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

[[nodiscard]] constexpr int distanceSquared(Point left, Point right) noexcept
{
    const int dx = left.x - right.x;
    const int dy = left.y - right.y;
    return (dx * dx) + (dy * dy);
}

constexpr bool operator==(Point left, Point right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

} // namespace rl
