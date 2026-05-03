#pragma once

#include <vector>

#include "input/InputEvent.hpp"

namespace rl {

class ConsoleBuffer;

class IRenderer {
  public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual std::vector<InputEvent> pollEvents() = 0;
    virtual void present(const ConsoleBuffer& console) = 0;
    [[nodiscard]] virtual bool shouldQuit() const noexcept = 0;
};

} // namespace rl
