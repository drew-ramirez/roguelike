/**
 * **************************************************************************
 * @file     WorldGeneration.cpp
 * @brief    Implements deterministic macro-world field helpers
 * **************************************************************************
 */

#include "world/WorldGeneration.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace rl {
namespace {

[[nodiscard]] std::uint32_t mixHash(std::uint32_t value) noexcept
{
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    return value;
}

[[nodiscard]] std::uint32_t hash2d(Rng::Seed seed, int x, int y) noexcept
{
    auto value = static_cast<std::uint32_t>(seed);
    value ^= static_cast<std::uint32_t>(x) * 0x9E3779B9U;
    value ^= static_cast<std::uint32_t>(y) * 0x85EBCA6BU;
    return mixHash(value);
}

[[nodiscard]] double latticeValue(Rng::Seed seed, int x, int y) noexcept
{
    constexpr double denominator = 16777215.0;
    return static_cast<double>(hash2d(seed, x, y) & 0x00FFFFFFU) /
           denominator;
}

[[nodiscard]] double smoothstep(double value) noexcept
{
    return value * value * (3.0 - (2.0 * value));
}

[[nodiscard]] double lerp(double from, double to, double amount) noexcept
{
    return from + ((to - from) * amount);
}

[[nodiscard]] double clamped01(double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

} // namespace

bool worldPointInBounds(Point point) noexcept
{
    return point.x >= 0 && point.y >= 0 && point.x < worldMapWidth &&
           point.y < worldMapHeight;
}

bool localScreenInBounds(Point point) noexcept
{
    return point.x >= 0 && point.y >= 0 && point.x < localScreenGridSize &&
           point.y < localScreenGridSize;
}

std::size_t worldCellIndex(Point point)
{
    if (!worldPointInBounds(point)) {
        throw std::out_of_range("World cell coordinate is out of bounds");
    }
    return static_cast<std::size_t>(point.y * worldMapWidth + point.x);
}

double valueNoise2d(Rng::Seed seed, double x, double y) noexcept
{
    const auto x0 = static_cast<int>(std::floor(x));
    const auto y0 = static_cast<int>(std::floor(y));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const double sx = smoothstep(x - static_cast<double>(x0));
    const double sy = smoothstep(y - static_cast<double>(y0));

    const double north = lerp(latticeValue(seed, x0, y0),
                              latticeValue(seed, x1, y0),
                              sx);
    const double south = lerp(latticeValue(seed, x0, y1),
                              latticeValue(seed, x1, y1),
                              sx);
    return clamped01(lerp(north, south, sy));
}

double fbmNoise2d(Rng::Seed seed, double x, double y, int octaves) noexcept
{
    double total = 0.0;
    double amplitude = 0.5;
    double frequency = 1.0;
    double normalization = 0.0;
    for (int octave = 0; octave < octaves; ++octave) {
        total += valueNoise2d(seed + static_cast<Rng::Seed>(octave * 7919),
                              x * frequency,
                              y * frequency) *
                 amplitude;
        normalization += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return normalization > 0.0 ? clamped01(total / normalization) : 0.0;
}

BiomeVariant deriveBiomeVariant(const WorldCellFields& fields)
{
    if (fields.dangerPressure > 0.78) {
        return BiomeVariant::DangerousWilds;
    }
    if (fields.ruinDensity > 0.68 && fields.woodlandDensity > 0.38) {
        return BiomeVariant::RuinedWoodland;
    }
    if (fields.elevation > 0.68 && fields.moisture < 0.58) {
        return BiomeVariant::LimestoneRise;
    }
    if (fields.marshPressure > 0.64 && fields.moisture > 0.55) {
        return fields.elevation < 0.28 ? BiomeVariant::Sinklands
                                       : BiomeVariant::ReedMarsh;
    }
    if (fields.woodlandDensity > 0.72) {
        return BiomeVariant::DeepGrove;
    }
    if (fields.moisture > 0.62) {
        return BiomeVariant::WetWoods;
    }
    return BiomeVariant::DryWoods;
}

std::string_view biomeVariantId(BiomeVariant biome) noexcept
{
    switch (biome) {
    case BiomeVariant::DryWoods:
        return "dry_woods";
    case BiomeVariant::WetWoods:
        return "wet_woods";
    case BiomeVariant::DeepGrove:
        return "deep_grove";
    case BiomeVariant::ReedMarsh:
        return "reed_marsh";
    case BiomeVariant::Sinklands:
        return "sinklands";
    case BiomeVariant::LimestoneRise:
        return "limestone_rise";
    case BiomeVariant::RuinedWoodland:
        return "ruined_woodland";
    case BiomeVariant::DangerousWilds:
        return "dangerous_wilds";
    }
    return "dry_woods";
}

BiomeVariant biomeVariantFromId(std::string_view id) noexcept
{
    if (id == "wet_woods") {
        return BiomeVariant::WetWoods;
    }
    if (id == "deep_grove") {
        return BiomeVariant::DeepGrove;
    }
    if (id == "reed_marsh") {
        return BiomeVariant::ReedMarsh;
    }
    if (id == "sinklands") {
        return BiomeVariant::Sinklands;
    }
    if (id == "limestone_rise") {
        return BiomeVariant::LimestoneRise;
    }
    if (id == "ruined_woodland") {
        return BiomeVariant::RuinedWoodland;
    }
    if (id == "dangerous_wilds") {
        return BiomeVariant::DangerousWilds;
    }
    return BiomeVariant::DryWoods;
}

} // namespace rl