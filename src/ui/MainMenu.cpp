/**
 * **************************************************************************
 * @file     MainMenu.cpp
 * @brief    Draws data-driven title art and handles menu selection
 * **************************************************************************
 */

#include "ui/MainMenu.hpp"

#include <algorithm>
#include <fstream>
#include <string>

#include "render/ConsoleBuffer.hpp"

namespace rl {
namespace {

struct MenuOption {
    std::string label;
    MainMenuAction action = MainMenuAction::None;
};

[[nodiscard]] std::vector<MenuOption> menuOptions(bool loadAvailable,
                                                  MainMenuMode mode)
{
    std::vector<MenuOption> options;
    options.push_back(
        MenuOption{mode == MainMenuMode::Title ? "New Game" : "Restart",
                   MainMenuAction::NewGame});
    if (loadAvailable) {
        options.push_back(MenuOption{"Load Game", MainMenuAction::LoadGame});
    }
    options.push_back(MenuOption{"Quit", MainMenuAction::Quit});
    return options;
}

[[nodiscard]] std::vector<std::string>
loadTitleArt(const std::filesystem::path& titlePath)
{
    std::ifstream input{titlePath};
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }

    if (!lines.empty()) {
        return lines;
    }

    return {
        " ____                         _ _ _        ",
        "|  _ \\ ___   __ _ _   _  ___| (_) | _____ ",
        "| |_) / _ \\ / _` | | | |/ _ \\ | | |/ / _ \\",
        "|  _ < (_) | (_| | |_| |  __/ | |   <  __/",
        "|_| \\_\\___/ \\__, |\\__,_|\\___|_|_|_|\\_\\___|",
        "            |___/                         ",
    };
}

void drawCentered(ConsoleBuffer& console,
                  int y,
                  const std::string& text,
                  Color color)
{
    const int x =
        std::max(0, (console.width() - static_cast<int>(text.size())) / 2);
    console.print(x, y, text, color);
}

} // namespace

MainMenuAction handleMainMenuInput(const std::vector<InputEvent>& events,
                                   bool loadAvailable,
                                   MainMenuMode mode,
                                   int& selection)
{
    const auto options = menuOptions(loadAvailable, mode);
    if (options.empty()) {
        return MainMenuAction::None;
    }

    selection = std::clamp(selection, 0, static_cast<int>(options.size()) - 1);
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }

        if (event.key == Key::Up || event.key == Key::W ||
            event.key == Key::Numpad8) {
            selection = std::max(0, selection - 1);
        } else if (event.key == Key::Down || event.key == Key::S ||
                   event.key == Key::Numpad2) {
            selection =
                std::min(static_cast<int>(options.size()) - 1, selection + 1);
        } else if (event.key == Key::Enter) {
            return options[static_cast<std::size_t>(selection)].action;
        } else if (event.key == Key::N) {
            return MainMenuAction::NewGame;
        } else if (event.key == Key::L && loadAvailable) {
            return MainMenuAction::LoadGame;
        } else if (event.key == Key::Q || event.key == Key::Escape) {
            return MainMenuAction::Quit;
        }
    }

    return MainMenuAction::None;
}

void drawMainMenu(ConsoleBuffer& console,
                  bool loadAvailable,
                  MainMenuMode mode,
                  int selection,
                  const std::filesystem::path& titlePath)
{
    const auto art = loadTitleArt(titlePath);
    int row = mode == MainMenuMode::Title ? 8 : 6;
    for (const auto& line : art) {
        drawCentered(console, row, line, Color{120, 220, 255, 255});
        ++row;
    }

    if (mode == MainMenuMode::GameOver) {
        drawCentered(console, row + 1, "GAME OVER", Color{255, 80, 80, 255});
        row += 3;
    } else {
        row += 2;
    }

    const auto options = menuOptions(loadAvailable, mode);
    selection = std::clamp(selection, 0, static_cast<int>(options.size()) - 1);
    for (std::size_t index = 0; index < options.size(); ++index) {
        const bool selected = static_cast<int>(index) == selection;
        drawCentered(console,
                     row,
                     std::string{selected ? "> " : "  "} + options[index].label,
                     selected ? Color::yellow() : Color{190, 190, 190, 255});
        ++row;
    }
}

} // namespace rl
