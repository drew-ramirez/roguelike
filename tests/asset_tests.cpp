#include "TestSuites.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <zlib.h>

#include "assets/RexPaintLoader.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/MapTheme.hpp"
#include "ui/MainMenu.hpp"

namespace rl::test {
namespace {

struct FixtureCell {
    char32_t glyph = U' ';
    Color foreground = Color::white();
    Color background = Color::black();
};

[[nodiscard]] std::filesystem::path tempPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() /
           (std::string{"roguelike_"} + std::string{name});
}

void appendInt32(std::vector<unsigned char>& bytes, int value)
{
    const auto bits = static_cast<unsigned int>(value);
    bytes.push_back(static_cast<unsigned char>(bits & 0xFFU));
    bytes.push_back(static_cast<unsigned char>((bits >> 8U) & 0xFFU));
    bytes.push_back(static_cast<unsigned char>((bits >> 16U) & 0xFFU));
    bytes.push_back(static_cast<unsigned char>((bits >> 24U) & 0xFFU));
}

void appendByte(std::vector<unsigned char>& bytes, std::uint8_t value)
{
    bytes.push_back(value);
}

void appendCell(std::vector<unsigned char>& bytes, const FixtureCell& cell)
{
    appendInt32(bytes, static_cast<int>(cell.glyph));
    appendByte(bytes, cell.foreground.r);
    appendByte(bytes, cell.foreground.g);
    appendByte(bytes, cell.foreground.b);
    appendByte(bytes, cell.background.r);
    appendByte(bytes, cell.background.g);
    appendByte(bytes, cell.background.b);
}

void writeFixtureXp(const std::filesystem::path& path,
                    int width,
                    int height,
                    const std::vector<std::vector<FixtureCell>>& layers)
{
    std::vector<unsigned char> bytes;
    appendInt32(bytes, -1);
    appendInt32(bytes, static_cast<int>(layers.size()));

    for (const auto& layer : layers) {
        appendInt32(bytes, width);
        appendInt32(bytes, height);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                appendCell(bytes,
                           layer[static_cast<std::size_t>(y * width + x)]);
            }
        }
    }

    const std::string fileName = path.string();
    gzFile file = gzopen(fileName.c_str(), "wb");
    if (file == nullptr) {
        throw RexPaintError{"Unable to write test REX Paint fixture"};
    }
    const int written =
        gzwrite(file, bytes.data(), static_cast<unsigned int>(bytes.size()));
    static_cast<void>(gzclose(file));
    if (written != static_cast<int>(bytes.size())) {
        throw RexPaintError{"Unable to compress test REX Paint fixture"};
    }
}

[[nodiscard]] std::vector<FixtureCell>
filledLayer(int width, int height, FixtureCell fill)
{
    return std::vector<FixtureCell>(static_cast<std::size_t>(width * height),
                                    fill);
}

void setFixtureCell(
    std::vector<FixtureCell>& layer, int width, int x, int y, FixtureCell cell)
{
    layer[static_cast<std::size_t>(y * width + x)] = cell;
}

[[nodiscard]] bool bufferContains(const ConsoleBuffer& console,
                                  std::string_view text)
{
    for (int y = 0; y < console.height(); ++y) {
        for (int x = 0; x <= console.width() - static_cast<int>(text.size());
             ++x) {
            bool matched = true;
            for (std::size_t index = 0; index < text.size(); ++index) {
                if (console.cellAt(x + static_cast<int>(index), y).glyph !=
                    static_cast<unsigned char>(text[index])) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool sameColor(Color left, Color right) noexcept
{
    return left.r == right.r && left.g == right.g && left.b == right.b &&
           left.a == right.a;
}

} // namespace

void runAssetTests(TestRunner& runner)
{
    {
        const auto path = tempPath("rexpaint_loader_test.xp");
        constexpr int width = 3;
        constexpr int height = 2;
        auto base = filledLayer(
            width, height, FixtureCell{U' ', Color::white(), Color::black()});
        auto overlay = filledLayer(
            width,
            height,
            FixtureCell{U' ', Color::white(), Color{255, 0, 255, 255}});

        setFixtureCell(
            base,
            width,
            0,
            0,
            FixtureCell{U'A', Color{1, 2, 3, 255}, Color{4, 5, 6, 255}});
        setFixtureCell(
            base,
            width,
            0,
            1,
            FixtureCell{U'B', Color{7, 8, 9, 255}, Color{10, 11, 12, 255}});
        setFixtureCell(
            base,
            width,
            1,
            0,
            FixtureCell{U'C', Color{13, 14, 15, 255}, Color{16, 17, 18, 255}});
        setFixtureCell(
            overlay,
            width,
            1,
            0,
            FixtureCell{U'Z', Color{40, 41, 42, 255}, Color{1, 1, 1, 255}});
        writeFixtureXp(path, width, height, {base, overlay});

        const auto image = loadRexPaintImage(path);
        runner.expect(image.width == width && image.height == height,
                      "rexpaint loader reports dimensions");
        runner.expect(image.layers.size() == 2U,
                      "rexpaint loader preserves layers");
        runner.expect(image.cellAt(0, 0, 0).glyph == U'A' &&
                          image.cellAt(0, 0, 1).glyph == U'B' &&
                          image.cellAt(0, 1, 0).glyph == U'C',
                      "rexpaint loader decodes column-major cell order");
        runner.expect(image.cellAt(0, 0, 0).foreground.r == 1U &&
                          image.cellAt(0, 0, 0).background.b == 6U,
                      "rexpaint loader decodes foreground and background");
        runner.expect(image.cellAt(1, 0, 0).transparentBackground &&
                          image.cellAt(1, 1, 0).glyph == U'Z',
                      "rexpaint loader marks transparent layer backgrounds");

        std::filesystem::remove(path);
    }

    {
        const auto titleText = tempPath("menu_title.txt");
        const auto titleXp = tempPath("menu_title.xp");
        {
            std::ofstream output{titleText};
            output << "TITLE\n";
        }

        constexpr int width = 80;
        constexpr int height = 50;
        auto layer = filledLayer(
            width,
            height,
            FixtureCell{U' ', Color{120, 120, 120, 255}, Color{3, 9, 15, 255}});
        setFixtureCell(
            layer,
            width,
            2,
            2,
            FixtureCell{U'*', Color{200, 220, 255, 255}, Color{3, 9, 15, 255}});
        writeFixtureXp(titleXp, width, height, {layer});

        ConsoleBuffer console{width, height};
        console.clear();
        drawMainMenu(console, true, MainMenuMode::Title, 0, titleText);
        runner.expect(console.cellAt(2, 2).glyph == U'*',
                      "main menu renders rexpaint title background");
        runner.expect(bufferContains(console, "> New Game") &&
                          bufferContains(console, "Load Game"),
                      "main menu keeps selectable text over xp background");

        std::filesystem::remove(titleText);
        std::filesystem::remove(titleXp);
    }

    {
        const auto image = loadRexPaintImage("assets/map-theme-druidic.xp");
        runner.expect(image.width == 6 && image.height == 2,
                      "druidic map theme asset is a 6x2 palette sheet");
        runner.expect(image.layers.size() == 1U,
                      "druidic map theme asset uses one palette layer");

        constexpr std::array<char32_t, 6> expectedGlyphs{
            U'#',
            U'.',
            U',',
            U'>',
            U'+',
            U'/',
        };
        bool slotsMatch = true;
        for (std::size_t index = 0; index < expectedGlyphs.size(); ++index) {
            const int x = static_cast<int>(index);
            slotsMatch = slotsMatch &&
                         image.cellAt(0, x, 0).glyph == expectedGlyphs[index] &&
                         image.cellAt(0, x, 1).glyph == expectedGlyphs[index];
        }
        runner.expect(slotsMatch,
                      "druidic map theme asset keeps terrain glyph slots");
        runner.expect(sameColor(image.cellAt(0, 1, 0).foreground,
                                Color{190, 205, 158, 255}) &&
                          sameColor(image.cellAt(0, 0, 1).foreground,
                                    Color{70, 88, 62, 255}),
                      "druidic map theme asset stores matcha and remembered "
                      "lichen colors");

        const auto theme = mapThemeById("druidic");
        runner.expect(
            sameColor(theme.style(MapThemeTile::Floor, true).foreground,
                      Color{190, 205, 158, 255}) &&
                sameColor(theme.style(MapThemeTile::Wall, false).foreground,
                          Color{70, 88, 62, 255}),
            "druidic map theme id loads the REX palette");

        bool rejectedUnknownTheme = false;
        try {
            static_cast<void>(mapThemeById("unknown-theme"));
        } catch (const std::invalid_argument&) {
            rejectedUnknownTheme = true;
        }
        runner.expect(rejectedUnknownTheme,
                      "map theme resolver rejects unknown theme ids");
    }
}

} // namespace rl::test
