#pragma once
/**
 * @file gamepad_input.hpp
 * @brief Declares and implements gamepad state sampling.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */

#include "pebble_blaster/input_mapping.hpp"

#include <SFML/Window/Joystick.hpp>

#include <array>
#include <optional>

namespace pebble_blaster {

struct JoystickState {
    std::optional<unsigned int> index{};
    sf::Joystick::Identification identification{};
    unsigned int buttonCount{};
    std::array<bool, sf::Joystick::ButtonCount> buttons{};
    bool hasX{};
    bool hasY{};
    bool hasPovX{};
    bool hasPovY{};
    float x{};
    float y{};
    float povX{};
    float povY{};
    Input input{};

    [[nodiscard]] bool buttonPressed(unsigned int button) const {
        return button < buttonCount && button < buttons.size() && buttons[button];
    }
};

inline JoystickState readJoystick() {
    sf::Joystick::update();
    JoystickState state;
    for (unsigned int index = 0; index < sf::Joystick::Count; ++index) {
        if (!sf::Joystick::isConnected(index)) {
            continue;
        }

        state.index = index;
        state.identification = sf::Joystick::getIdentification(index);
        state.buttonCount = sf::Joystick::getButtonCount(index);
        state.hasX = sf::Joystick::hasAxis(index, sf::Joystick::Axis::X);
        state.hasY = sf::Joystick::hasAxis(index, sf::Joystick::Axis::Y);
        state.hasPovX = sf::Joystick::hasAxis(index, sf::Joystick::Axis::PovX);
        state.hasPovY = sf::Joystick::hasAxis(index, sf::Joystick::Axis::PovY);
        if (state.hasX) {
            state.x = sf::Joystick::getAxisPosition(index, sf::Joystick::Axis::X);
        }
        if (state.hasY) {
            state.y = sf::Joystick::getAxisPosition(index, sf::Joystick::Axis::Y);
        }
        if (state.hasPovX) {
            state.povX = sf::Joystick::getAxisPosition(index, sf::Joystick::Axis::PovX);
        }
        if (state.hasPovY) {
            state.povY = sf::Joystick::getAxisPosition(index, sf::Joystick::Axis::PovY);
        }
        for (unsigned int button = 0; button < state.buttons.size() &&
                                       button < state.buttonCount;
             ++button) {
            state.buttons[button] = sf::Joystick::isButtonPressed(index, button);
        }

        state.input = mapJoystickInput({state.x, state.y, state.povX, state.povY,
                        state.buttonPressed(0), state.buttonPressed(1)});
        return state;
    }
    return state;
}

} // namespace pebble_blaster