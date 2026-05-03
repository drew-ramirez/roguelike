#pragma once

namespace rl {

enum class Key { Unknown, Up, Down, Left, Right, W, A, S, D };

enum class InputEventType { Quit, KeyDown };

struct InputEvent {
    InputEventType type = InputEventType::KeyDown;
    Key key = Key::Unknown;
};

} // namespace rl
