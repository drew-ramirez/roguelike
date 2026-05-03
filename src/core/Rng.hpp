/**
 * **************************************************************************
 * @file     Rng.hpp
 * @brief    Declares the central deterministic RNG wrapper
 * **************************************************************************
 */

#pragma once

#include <cstdint>
#include <random>
#include <string>

namespace rl {

class Rng {
  public:
    using Seed = std::uint32_t;

    Rng();
    explicit Rng(Seed seed);

    [[nodiscard]] static Seed makeSeed();
    [[nodiscard]] Seed seed() const noexcept;
    [[nodiscard]] std::string engineState() const;
    [[nodiscard]] int rangeInclusive(int min, int max);
    [[nodiscard]] bool coinFlip();

    void reseed(Seed seed);
    void restoreEngineState(const std::string& state);

  private:
    Seed seed_ = 0;
    std::mt19937 engine_;
};

} // namespace rl
