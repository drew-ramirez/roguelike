#pragma once

#include "fov/IFovProvider.hpp"

namespace rl {

class ShadowcastingFov final : public IFovProvider {
public:
    [[nodiscard]] std::vector<Point> compute(const Map& map, Point origin, int range) const override;
};

} // namespace rl
