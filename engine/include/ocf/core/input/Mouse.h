// SPDX-License-Identifier: MIT
#pragma once 

namespace ocf {

enum class MouseButton {
    Button1 = (1 << 0),
    Button2 = (1 << 1),
    Button3 = (1 << 2),
    Button4 = (1 << 3),
    Button5 = (1 << 4),
    Button6 = (1 << 5),
    Button7 = (1 << 6),
    Button8 = (1 << 7),
    ButtonMax = (1 << 8),
    Left = Button1,
    Right = Button2,
    Middle = Button3
};

} // namespace ocf
