#pragma once

#include "render/Color.hpp"

namespace rl {

class ConsoleBuffer;

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
