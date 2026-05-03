#pragma once

namespace rl {

enum class MenuResultType {
    NoResponse,
    Cancel,
    Selected
};

struct MenuResult {
    MenuResultType type = MenuResultType::NoResponse;
    int selected = -1;
};

} // namespace rl
