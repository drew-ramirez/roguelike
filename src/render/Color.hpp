#pragma once

#include <cstdint>

namespace rl {

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;

    static constexpr Color black() noexcept { return {0, 0, 0, 255}; }
    static constexpr Color white() noexcept { return {255, 255, 255, 255}; }
    static constexpr Color yellow() noexcept { return {255, 220, 120, 255}; }
    static constexpr Color cyan() noexcept { return {120, 220, 255, 255}; }
};

} // namespace rl
