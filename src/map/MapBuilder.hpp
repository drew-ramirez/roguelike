/**
 * **************************************************************************
 * @file     MapBuilder.hpp
 * @brief    Declares the map builder interface used by generation pipelines
 * **************************************************************************
 */

#pragma once

#include <string_view>

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class MapBuilder {
  public:
    virtual ~MapBuilder() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual MapBuildResult
    build(int width, int height, Rng& rng, MapBuildOptions options) = 0;
};

} // namespace rl
