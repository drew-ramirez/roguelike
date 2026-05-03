/**
 * **************************************************************************
 * @file     Rng.cpp
 * @brief    Owns seed creation and bounded random rolls
 * **************************************************************************
 */

#include "core/Rng.hpp"

#include <random>
#include <sstream>
#include <stdexcept>

namespace rl {

Rng::Rng() : Rng(0) {}

Rng::Rng(Seed seed) : seed_(seed), engine_(seed) {}

Rng::Seed Rng::makeSeed()
{
    std::random_device device;
    return static_cast<Seed>(device());
}

Rng::Seed Rng::seed() const noexcept
{
    return seed_;
}

std::string Rng::engineState() const
{
    std::ostringstream output;
    output << engine_;
    return output.str();
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

void Rng::restoreEngineState(const std::string& state)
{
    std::istringstream input{state};
    std::mt19937 restored;
    input >> restored;
    if (!input) {
        throw std::invalid_argument("Invalid RNG engine state");
    }
    engine_ = restored;
}

} // namespace rl
