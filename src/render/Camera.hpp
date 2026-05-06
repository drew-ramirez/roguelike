/**
 * **************************************************************************
 * @file     Camera.hpp
 * @brief    Declares viewport conversion between world and screen space
 * **************************************************************************
 */

#pragma once

#include <optional>

#include "core/Point.hpp"

namespace rl {

class Map;

class Camera {
  public:
    Camera(int width = 80, int height = 43);

    [[nodiscard]] int left() const noexcept;
    [[nodiscard]] int top() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    void resize(int width, int height);
    void centerOn(Point worldPosition, const Map& map);

    [[nodiscard]] bool containsWorld(Point world) const noexcept;
    [[nodiscard]] std::optional<Point> worldToScreen(Point world) const;
    [[nodiscard]] Point screenToWorld(Point screen) const noexcept;

  private:
    int left_ = 0;
    int top_ = 0;
    int width_ = 80;
    int height_ = 43;
};

} // namespace rl
