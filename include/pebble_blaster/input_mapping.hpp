#pragma once

#include "pebble_blaster/core.hpp"

namespace pebble_blaster {

struct JoystickInputSample {
    float x{};
    float y{};
    float povX{};
    float povY{};
    bool button0{};
    bool button1{};
};

[[nodiscard]] Input mapJoystickInput(const JoystickInputSample &sample);
[[nodiscard]] Input combineInputs(const Input &keyboard, const Input &joystick);

} // namespace pebble_blaster