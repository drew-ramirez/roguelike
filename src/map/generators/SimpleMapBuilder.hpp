#pragma once

#include "core/Rng.hpp"
#include "map/MapBuilderTypes.hpp"

namespace rl {

class SimpleMapBuilder {
  public:
    [[nodiscard]] static MapBuildResult build(int width, int height, Rng& rng);
};

} // namespace rl
