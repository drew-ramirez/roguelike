/**
 * **************************************************************************
 * @file     LayeredConsoleBuffer.cpp
 * @brief    Flattens logical console layers into a normal ConsoleBuffer
 * **************************************************************************
 */

#include "render/LayeredConsoleBuffer.hpp"

#include <stdexcept>

namespace rl {

LayeredConsoleBuffer::LayeredConsoleBuffer(int width, int height)
    : width_(width), height_(height)
{
    layers_.reserve(consoleLayerStack.size());
    for ([[maybe_unused]] const auto layer : consoleLayerStack) {
        auto& buffer = layers_.emplace_back(width, height);
        buffer.clearTransparent();
    }
}

int LayeredConsoleBuffer::width() const noexcept
{
    return width_;
}

int LayeredConsoleBuffer::height() const noexcept
{
    return height_;
}

ConsoleBuffer& LayeredConsoleBuffer::layer(ConsoleLayer layer)
{
    return layers_.at(consoleLayerIndex(layer));
}

const ConsoleBuffer& LayeredConsoleBuffer::layer(ConsoleLayer layer) const
{
    return layers_.at(consoleLayerIndex(layer));
}

void LayeredConsoleBuffer::clearLayer(ConsoleLayer layer)
{
    this->layer(layer).clearTransparent();
}

void LayeredConsoleBuffer::clearAll()
{
    for (auto& buffer : layers_) {
        buffer.clearTransparent();
    }
}

ConsoleBuffer LayeredConsoleBuffer::flatten() const
{
    ConsoleBuffer result{width_, height_};
    flattenInto(result);
    return result;
}

void LayeredConsoleBuffer::flattenInto(ConsoleBuffer& destination) const
{
    if (destination.width() != width_ || destination.height() != height_) {
        throw std::invalid_argument(
            "LayeredConsoleBuffer destination dimensions must match");
    }

    destination.clear();
    for (const auto& buffer : layers_) {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const auto& cell = buffer.cellAt(x, y);
                if (!isTransparentCell(cell)) {
                    destination.setCell(x, y, cell);
                }
            }
        }
    }
}

} // namespace rl
