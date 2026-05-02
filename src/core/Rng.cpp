#include "core/Rng.hpp"

#include <random>
#include <stdexcept>

namespace rl {

Rng::Rng()
    : Rng(0)
{
}

Rng::Rng(Seed seed)
    : seed_(seed),
      engine_(seed)
{
}

Rng::Seed Rng::makeSeed()
{
    std::random_device device;
    return static_cast<Seed>(device());
}

Rng::Seed Rng::seed() const noexcept
{
    return seed_;
}

int Rng::rangeInclusive(int min, int max)
{
    if (min > max) {
        throw std::invalid_argument("RNG range minimum cannot exceed maximum");
    }

    std::uniform_int_distribution<int> distribution{min, max};
    return distribution(engine_);
}

bool Rng::coinFlip()
{
    return rangeInclusive(0, 1) == 1;
}

void Rng::reseed(Seed seed)
{
    seed_ = seed;
    engine_.seed(seed_);
}

} // namespace rl
