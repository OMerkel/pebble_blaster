#include "pebble_blaster/input_mapping.hpp"

#include <cmath>

namespace pebble_blaster {
namespace {

float deadZonedAxis(float position) {
    position /= 100.0F;
    constexpr float deadZone = 0.2F;
    if (position > deadZone) {
        return (position - deadZone) / (1.0F - deadZone);
    }
    if (position < -deadZone) {
        return (position + deadZone) / (1.0F - deadZone);
    }
    return 0.0F;
}

} // namespace

Input mapJoystickInput(const JoystickInputSample &sample) {
    Input input;
    input.turn = std::abs(sample.povX) >= 50.0F
                     ? std::copysign(1.0F, sample.povX)
                     : deadZonedAxis(sample.x);
    input.thrust = deadZonedAxis(sample.y) < -0.5F || sample.povY < -50.0F;
    input.fire = sample.button0 || sample.button1;
    return input;
}

Input combineInputs(const Input &keyboard, const Input &joystick) {
    return {
        .left = keyboard.left || joystick.left,
        .right = keyboard.right || joystick.right,
        .turn = keyboard.turn + joystick.turn,
        .thrust = keyboard.thrust || joystick.thrust,
        .fire = keyboard.fire || joystick.fire,
    };
}

} // namespace pebble_blaster