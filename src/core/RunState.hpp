#pragma once

namespace rl {

enum class RunState {
    MainMenu,
    AwaitingInput,
    PlayerTurn,
    MonsterTurn,
    ShowInventory,
    ShowCamp,
    ShowVendor,
    ShowSpellMenu,
    ShowItemSelection,
    ShowDropItem,
    ShowTargeting,
    ShowWorldMap,
    MapGeneration,
    GameOver
};

} // namespace rl
