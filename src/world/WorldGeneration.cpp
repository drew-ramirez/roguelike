/**
 * **************************************************************************
 * @file     WorldGeneration.cpp
 * @brief    Builds deterministic macro-world cells and local screen roles
 * **************************************************************************
 */

#include "world/WorldGeneration.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

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

[[nodiscard]] WorldCellFields sampleFields(Rng::Seed seed, Point point)
{
    const double x = static_cast<double>(point.x);
    const double y = static_cast<double>(point.y);
    return WorldCellFields{
        fbmNoise2d(seed ^ 0x10203040U, x * 0.43, y * 0.43),
        fbmNoise2d(seed ^ 0x20304010U, x * 0.39 + 19.0, y * 0.39 - 11.0),
        fbmNoise2d(seed ^ 0x30401020U, x * 0.51 - 7.0, y * 0.51 + 5.0),
        fbmNoise2d(seed ^ 0x40102030U, x * 0.47 + 23.0, y * 0.47 + 17.0),
        fbmNoise2d(seed ^ 0xA0B0C0D0U, x * 0.59 - 31.0, y * 0.59 + 29.0),
        fbmNoise2d(seed ^ 0x0D0C0B0AU, x * 0.61 + 37.0, y * 0.61 - 41.0)};
}

void applyAnchor(WorldCellDescriptor& cell)
{
    if (cell.position == greenholtWorldPosition()) {
        cell.fields = WorldCellFields{0.34, 0.42, 0.62, 0.22, 0.12, 0.08};
        cell.biome = BiomeVariant::DryWoods;
        cell.name = "Greenholt";
        cell.glyph = U'H';
        cell.foreground = Color{120, 220, 130, 255};
        cell.discovered = true;
        cell.landmark = WorldLandmark{"greenholt", "Greenholt", false};
        return;
    }

    if (cell.position == reedfenWorldPosition()) {
        cell.fields = WorldCellFields{0.18, 0.78, 0.44, 0.84, 0.18, 0.36};
        cell.biome = BiomeVariant::ReedMarsh;
        cell.name = "Reedfen";
        cell.glyph = U'R';
        cell.foreground = Color{120, 205, 185, 255};
        cell.landmark = WorldLandmark{"reedfen", "Reedfen", false};
        return;
    }

    if (cell.position == limestoneWorldPosition()) {
        cell.fields = WorldCellFields{0.82, 0.36, 0.34, 0.18, 0.28, 0.42};
        cell.biome = BiomeVariant::LimestoneRise;
        cell.name = "Limestone";
        cell.glyph = U'L';
        cell.foreground = Color{210, 210, 185, 255};
        cell.landmark = WorldLandmark{"limestone", "Limestone", false};
    }
}

[[nodiscard]] std::string generatedCellName(BiomeVariant biome, Point point)
{
    const std::array<std::string_view, 4> woods{
        "Ashwood", "Hearthwood", "Greybough", "Foxglade"};
    const std::array<std::string_view, 4> wet{
        "Mosswater", "Willowmere", "Rainbough", "Mistgrove"};
    const std::array<std::string_view, 4> marsh{
        "Reedwash", "Fenwale", "Mireglass", "Sedgewater"};
    const std::array<std::string_view, 4> stone{
        "Chalkrise", "Paleledge", "Karstway", "Whitebarrow"};
    const std::array<std::string_view, 4> ruin{
        "Oldroot", "Broken Copse", "Mossfallen", "Ashen Ruins"};
    const auto pick = static_cast<std::size_t>(
        std::abs((point.x * 17) + (point.y * 31)) % 4);

    switch (biome) {
    case BiomeVariant::DryWoods:
    case BiomeVariant::DeepGrove:
        return std::string{woods[pick]};
    case BiomeVariant::WetWoods:
        return std::string{wet[pick]};
    case BiomeVariant::ReedMarsh:
    case BiomeVariant::Sinklands:
        return std::string{marsh[pick]};
    case BiomeVariant::LimestoneRise:
        return std::string{stone[pick]};
    case BiomeVariant::RuinedWoodland:
        return std::string{ruin[pick]};
    case BiomeVariant::DangerousWilds:
        return "Red Wilds";
    }
    return "Wilds";
}

[[nodiscard]] bool isCardinalScreen(Point screen) noexcept
{
    return (screen.x == 1 && screen.y != 1) ||
           (screen.y == 1 && screen.x != 1);
}

[[nodiscard]] bool isCornerScreen(Point screen) noexcept
{
    return screen.x != 1 && screen.y != 1;
}

[[nodiscard]] int deterministicRoleRoll(const WorldCellDescriptor& cell,
                                         Point screen) noexcept
{
    const auto hash = hash2d(static_cast<Rng::Seed>(
                                 (cell.position.x + 31) * 101 +
                                 (cell.position.y + 17) * 313),
                             screen.x,
                             screen.y);
    return static_cast<int>(hash % 100U);
}

[[nodiscard]] int exitMaskFor(Point macro, Point screen) noexcept
{
    int mask = 0;
    if (screen.y > 0 || macro.y > 0) {
        mask |= exitNorth;
    }
    if (screen.x < localScreenGridSize - 1 ||
        macro.x < worldMapWidth - 1) {
        mask |= exitEast;
    }
    if (screen.y < localScreenGridSize - 1 ||
        macro.y < worldMapHeight - 1) {
        mask |= exitSouth;
    }
    if (screen.x > 0 || macro.x > 0) {
        mask |= exitWest;
    }
    return mask;
}

[[nodiscard]] std::string builderForDescriptor(
    const WorldCellDescriptor& cell,
    LocalScreenRole role)
{
    if (role == LocalScreenRole::CenterLandmark &&
        cell.landmark.has_value()) {
        if (cell.landmark->id == "greenholt") {
            return "town";
        }
        if (cell.landmark->id == "limestone") {
            return "surface-cave-mouth";
        }
        return "surface-camp-shrine";
    }

    switch (role) {
    case LocalScreenRole::CenterLandmark:
        return "surface-resource-heavy";
    case LocalScreenRole::CardinalTransition:
        return "surface-cardinal-transition";
    case LocalScreenRole::CornerWilderness:
        return "surface-corner-wilderness";
    case LocalScreenRole::RuinPocket:
        return "surface-ruin-pocket";
    case LocalScreenRole::CaveMouth:
        return "surface-cave-mouth";
    case LocalScreenRole::CampShrine:
        return "surface-camp-shrine";
    case LocalScreenRole::ResourceHeavy:
        return "surface-resource-heavy";
    case LocalScreenRole::TownOutskirts:
        return "surface-town-outskirts";
    case LocalScreenRole::DangerEncounter:
        return "surface-danger-encounter";
    }
    return "surface-corner-wilderness";
}

[[nodiscard]] LocalScreenRole roleForScreen(const WorldCellDescriptor& cell,
                                            Point screen,
                                            bool dangerEncounter)
{
    if (dangerEncounter) {
        return LocalScreenRole::DangerEncounter;
    }

    if (screen == Point{1, 1} && cell.landmark.has_value()) {
        return LocalScreenRole::CenterLandmark;
    }

    if (cell.landmark.has_value() && cell.landmark->id == "greenholt" &&
        isCardinalScreen(screen)) {
        return LocalScreenRole::TownOutskirts;
    }

    if (isCornerScreen(screen)) {
        if (cell.fields.ruinDensity > 0.66 &&
            deterministicRoleRoll(cell, screen) > 45) {
            return LocalScreenRole::RuinPocket;
        }
        return LocalScreenRole::CornerWilderness;
    }

    if (screen == Point{1, 1}) {
        if (cell.biome == BiomeVariant::Sinklands ||
            cell.biome == BiomeVariant::LimestoneRise) {
            return LocalScreenRole::CaveMouth;
        }
        if (cell.fields.ruinDensity > 0.62) {
            return LocalScreenRole::RuinPocket;
        }
        if (deterministicRoleRoll(cell, screen) < 18) {
            return LocalScreenRole::CampShrine;
        }
        return LocalScreenRole::ResourceHeavy;
    }

    if ((cell.biome == BiomeVariant::Sinklands ||
         cell.biome == BiomeVariant::LimestoneRise) &&
        screen.y == 2) {
        return LocalScreenRole::CaveMouth;
    }

    if (cell.fields.ruinDensity > 0.68 &&
        deterministicRoleRoll(cell, screen) > 55) {
        return LocalScreenRole::RuinPocket;
    }

    return LocalScreenRole::CardinalTransition;
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

WorldCellDescriptor* worldCellAt(WorldMapState& world, Point point)
{
    if (!worldPointInBounds(point) ||
        world.cells.size() !=
            static_cast<std::size_t>(worldMapWidth * worldMapHeight)) {
        return nullptr;
    }
    return &world.cells[worldCellIndex(point)];
}

const WorldCellDescriptor* worldCellAt(const WorldMapState& world, Point point)
{
    if (!worldPointInBounds(point) ||
        world.cells.size() !=
            static_cast<std::size_t>(worldMapWidth * worldMapHeight)) {
        return nullptr;
    }
    return &world.cells[worldCellIndex(point)];
}

double valueNoise2d(Rng::Seed seed, double x, double y) noexcept
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
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
    double amplitude = 0.5;
    double frequency = 1.0;
    double total = 0.0;
    double normalization = 0.0;
    for (int octave = 0; octave < octaves; ++octave) {
        total += valueNoise2d(seed + static_cast<Rng::Seed>(octave * 9973),
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
    if (fields.marshPressure > 0.68 && fields.moisture > 0.58) {
        return fields.elevation < 0.28 ? BiomeVariant::Sinklands
                                       : BiomeVariant::ReedMarsh;
    }
    if (fields.woodlandDensity > 0.72 && fields.moisture > 0.48) {
        return BiomeVariant::DeepGrove;
    }
    if (fields.moisture > 0.55) {
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

std::string_view biomeDisplayName(BiomeVariant biome) noexcept
{
    switch (biome) {
    case BiomeVariant::DryWoods:
        return "Dry woods";
    case BiomeVariant::WetWoods:
        return "Wet woods";
    case BiomeVariant::DeepGrove:
        return "Deep grove";
    case BiomeVariant::ReedMarsh:
        return "Reed marsh";
    case BiomeVariant::Sinklands:
        return "Sinklands";
    case BiomeVariant::LimestoneRise:
        return "Limestone rise";
    case BiomeVariant::RuinedWoodland:
        return "Ruined woodland";
    case BiomeVariant::DangerousWilds:
        return "Dangerous wilds";
    }
    return "Dry woods";
}

char32_t biomeGlyph(BiomeVariant biome) noexcept
{
    switch (biome) {
    case BiomeVariant::DryWoods:
        return U't';
    case BiomeVariant::WetWoods:
        return U'T';
    case BiomeVariant::DeepGrove:
        return U'G';
    case BiomeVariant::ReedMarsh:
        return U',';
    case BiomeVariant::Sinklands:
        return U'~';
    case BiomeVariant::LimestoneRise:
        return U'^';
    case BiomeVariant::RuinedWoodland:
        return U'&';
    case BiomeVariant::DangerousWilds:
        return U'!';
    }
    return U'.';
}

Color biomeColor(BiomeVariant biome) noexcept
{
    switch (biome) {
    case BiomeVariant::DryWoods:
        return Color{110, 190, 110, 255};
    case BiomeVariant::WetWoods:
        return Color{80, 175, 135, 255};
    case BiomeVariant::DeepGrove:
        return Color{70, 145, 90, 255};
    case BiomeVariant::ReedMarsh:
        return Color{125, 190, 150, 255};
    case BiomeVariant::Sinklands:
        return Color{85, 155, 175, 255};
    case BiomeVariant::LimestoneRise:
        return Color{210, 210, 185, 255};
    case BiomeVariant::RuinedWoodland:
        return Color{155, 145, 105, 255};
    case BiomeVariant::DangerousWilds:
        return Color{220, 90, 75, 255};
    }
    return Color::white();
}

std::string_view localScreenRoleId(LocalScreenRole role) noexcept
{
    switch (role) {
    case LocalScreenRole::CenterLandmark:
        return "center_landmark";
    case LocalScreenRole::CardinalTransition:
        return "cardinal_transition";
    case LocalScreenRole::CornerWilderness:
        return "corner_wilderness";
    case LocalScreenRole::RuinPocket:
        return "ruin_pocket";
    case LocalScreenRole::CaveMouth:
        return "cave_mouth";
    case LocalScreenRole::CampShrine:
        return "camp_shrine";
    case LocalScreenRole::ResourceHeavy:
        return "resource_heavy";
    case LocalScreenRole::TownOutskirts:
        return "town_outskirts";
    case LocalScreenRole::DangerEncounter:
        return "danger_encounter";
    }
    return "corner_wilderness";
}

LocalScreenRole localScreenRoleFromId(std::string_view id) noexcept
{
    if (id == "center_landmark") {
        return LocalScreenRole::CenterLandmark;
    }
    if (id == "cardinal_transition") {
        return LocalScreenRole::CardinalTransition;
    }
    if (id == "ruin_pocket") {
        return LocalScreenRole::RuinPocket;
    }
    if (id == "cave_mouth") {
        return LocalScreenRole::CaveMouth;
    }
    if (id == "camp_shrine") {
        return LocalScreenRole::CampShrine;
    }
    if (id == "resource_heavy") {
        return LocalScreenRole::ResourceHeavy;
    }
    if (id == "town_outskirts") {
        return LocalScreenRole::TownOutskirts;
    }
    if (id == "danger_encounter") {
        return LocalScreenRole::DangerEncounter;
    }
    return LocalScreenRole::CornerWilderness;
}

WorldMapState generateWorldMap(Rng::Seed seed)
{
    WorldMapState world;
    world.active = true;
    world.onSurface = true;
    world.currentMacro = greenholtWorldPosition();
    world.currentScreen = Point{1, 1};
    world.cursor = world.currentMacro;
    world.cells.reserve(
        static_cast<std::size_t>(worldMapWidth * worldMapHeight));

    for (int y = 0; y < worldMapHeight; ++y) {
        for (int x = 0; x < worldMapWidth; ++x) {
            WorldCellDescriptor cell;
            cell.position = Point{x, y};
            cell.fields = sampleFields(seed, cell.position);
            cell.biome = deriveBiomeVariant(cell.fields);
            cell.name = generatedCellName(cell.biome, cell.position);
            cell.glyph = biomeGlyph(cell.biome);
            cell.foreground = biomeColor(cell.biome);
            applyAnchor(cell);
            world.cells.push_back(std::move(cell));
        }
    }

    return world;
}

LocalScreenDescriptor describeLocalScreen(const WorldMapState& world,
                                          Point macro,
                                          Point screen,
                                          bool dangerEncounter)
{
    if (!worldPointInBounds(macro) || !localScreenInBounds(screen)) {
        throw std::out_of_range("Local screen coordinate is out of bounds");
    }

    const auto* cell = worldCellAt(world, macro);
    if (cell == nullptr) {
        throw std::runtime_error("World map has no cells to describe");
    }

    LocalScreenDescriptor descriptor;
    descriptor.macro = macro;
    descriptor.screen = screen;
    descriptor.role = roleForScreen(*cell, screen, dangerEncounter);
    descriptor.exitMask = exitMaskFor(macro, screen);
    descriptor.builderId = builderForDescriptor(*cell, descriptor.role);
    descriptor.dangerEncounter = dangerEncounter;
    if (cell->landmark.has_value()) {
        descriptor.landmarkId = cell->landmark->id;
    }
    return descriptor;
}

Point greenholtWorldPosition() noexcept
{
    return Point{2, 2};
}

Point reedfenWorldPosition() noexcept
{
    return Point{6, 3};
}

Point limestoneWorldPosition() noexcept
{
    return Point{1, 4};
}

} // namespace rl
