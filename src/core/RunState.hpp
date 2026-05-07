#pragma once

namespace rl {

enum class RunState {
    MainMenu,
    AwaitingInput,
    PlayerTurn,
    MonsterTurn,
    ShowInventory,
    ShowDropItem,
    ShowTargeting,
    MapGeneration,
    GameOver
};

} // namespace rl
