#pragma once

#include <string>

#include "render/Color.hpp"

namespace rl {

class ConsoleBuffer;

[[nodiscard]] std::string clippedLabel(std::string text, int width);
void drawMenuFrame(ConsoleBuffer& console, int x, int y, int width, int height);
void drawPanel(ConsoleBuffer& console, int top, int height);
void drawBar(ConsoleBuffer& console,
             int x,
             int y,
             int width,
             int current,
             int maximum,
             Color fill,
             Color empty);

} // namespace rl
