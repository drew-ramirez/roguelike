/**
 * **************************************************************************
 * @file     LayeredConsoleBuffer.hpp
 * @brief    Declares renderer-independent console layer compositing
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "render/ConsoleBuffer.hpp"
#include "render/ConsoleLayer.hpp"

namespace rl {

class LayeredConsoleBuffer {
  public:
    LayeredConsoleBuffer(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    [[nodiscard]] ConsoleBuffer& layer(ConsoleLayer layer);
    [[nodiscard]] const ConsoleBuffer& layer(ConsoleLayer layer) const;

    void clearLayer(ConsoleLayer layer);
    void clearAll();

    [[nodiscard]] ConsoleBuffer flatten() const;
    void flattenInto(ConsoleBuffer& destination) const;

  private:
    int width_ = 0;
    int height_ = 0;
    std::vector<ConsoleBuffer> layers_;
};

} // namespace rl
