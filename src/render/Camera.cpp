/**
 * **************************************************************************
 * @file     Camera.cpp
 * @brief    Implements viewport conversion between world and screen space
 * **************************************************************************
 */

#include "render/Camera.hpp"

#include <algorithm>
#include <stdexcept>

#include "map/Map.hpp"

namespace rl {

Camera::Camera(int width, int height)
{
    resize(width, height);
}

int Camera::left() const noexcept
{
    return left_;
}

int Camera::top() const noexcept
{
    return top_;
}

int Camera::width() const noexcept
{
    return width_;
}

int Camera::height() const noexcept
{
    return height_;
}

void Camera::resize(int width, int height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Camera dimensions must be positive");
    }
    width_ = width;
    height_ = height;
}

void Camera::centerOn(Point worldPosition, const Map& map)
{
    const int maxLeft = std::max(0, map.width() - width_);
    const int maxTop = std::max(0, map.height() - height_);
    // desired origin centers the world point, clamping keeps the viewport
    // inside the map when the player approaches an edge or the map is smaller
    // than the screen
    left_ = std::clamp(worldPosition.x - (width_ / 2), 0, maxLeft);
    top_ = std::clamp(worldPosition.y - (height_ / 2), 0, maxTop);
}

bool Camera::containsWorld(Point world) const noexcept
{
    return world.x >= left_ && world.y >= top_ && world.x < left_ + width_ &&
           world.y < top_ + height_;
}

std::optional<Point> Camera::worldToScreen(Point world) const
{
    if (!containsWorld(world)) {
        return std::nullopt;
    }
    // Screen space is just world space relative to the camera origin.
    return Point{world.x - left_, world.y - top_};
}

Point Camera::screenToWorld(Point screen) const noexcept
{
    // input/debug rendering convert back by restoring the camera origin
    return Point{screen.x + left_, screen.y + top_};
}

} // namespace rl
