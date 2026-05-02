#pragma once

#include <cstdint>
#include <random>

namespace rl {

class Rng {
public:
    using Seed = std::uint32_t;

    Rng();
    explicit Rng(Seed seed);

    [[nodiscard]] static Seed makeSeed();
    [[nodiscard]] Seed seed() const noexcept;
    [[nodiscard]] int rangeInclusive(int min, int max);
    [[nodiscard]] bool coinFlip();

    void reseed(Seed seed);

private:
    Seed seed_ = 0;
    std::mt19937 engine_;
};

} // namespace rl
