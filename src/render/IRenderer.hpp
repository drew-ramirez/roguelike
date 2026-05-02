#pragma once

namespace rl {

class ConsoleBuffer;

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void pollEvents() = 0;
    virtual void present(const ConsoleBuffer& console) = 0;
    [[nodiscard]] virtual bool shouldQuit() const noexcept = 0;
};

} // namespace rl
