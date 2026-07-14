/**
 * **************************************************************************
 * @file     MainMenu.hpp
 * @brief    Declares title and game-over menu input/render helpers
 * **************************************************************************
 */

#pragma once

#include <filesystem>
#include <vector>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;

enum class MainMenuMode { Title, GameOver };
enum class MainMenuAction { None, NewGame, LoadGame, Gallery, Quit };

[[nodiscard]] MainMenuAction
handleMainMenuInput(const std::vector<InputEvent>& events,
                    bool loadAvailable,
                    MainMenuMode mode,
                    int& selection);

void drawMainMenu(ConsoleBuffer& console,
                  bool loadAvailable,
                  MainMenuMode mode,
                  int selection,
                  const std::filesystem::path& titlePath);

} // namespace rl
