#pragma once

namespace rl {

enum class Key {
    Unknown,
    Up,
    Down,
    Left,
    Right,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Escape,
    Enter,
    Space,
    F5,
    Descend,
    Ascend
};

enum class InputEventType { Quit, KeyDown };

struct InputEvent {
    InputEventType type = InputEventType::KeyDown;
    Key key = Key::Unknown;
};

} // namespace rl
