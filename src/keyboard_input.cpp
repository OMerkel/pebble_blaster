/**
 * @file keyboard_input.cpp
 * @brief Maps keyboard events and state to game input.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */
#include "pebble_blaster/keyboard_input.hpp"

#include <SFML/Window/Keyboard.hpp>

namespace pebble_blaster {

KeyboardCommand keyboardCommand(const sf::Event &event) {
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    if (!key) {
        return KeyboardCommand::none;
    }

    using Key = sf::Keyboard::Scancode;
    if (key->scancode == Key::Enter) {
        return KeyboardCommand::activate;
    }
    if (key->scancode == Key::P || key->scancode == Key::Escape) {
        return KeyboardCommand::togglePause;
    }
    if (key->scancode == Key::R) {
        return KeyboardCommand::restart;
    }
    return KeyboardCommand::none;
}

Input readKeyboardInput() {
    using Key = sf::Keyboard::Scancode;
    Input input;
    input.left = sf::Keyboard::isKeyPressed(Key::Left) || sf::Keyboard::isKeyPressed(Key::A);
    input.right = sf::Keyboard::isKeyPressed(Key::Right) || sf::Keyboard::isKeyPressed(Key::D);
    input.thrust = sf::Keyboard::isKeyPressed(Key::Up) || sf::Keyboard::isKeyPressed(Key::W);
    input.fire = sf::Keyboard::isKeyPressed(Key::Space);
    return input;
}

} // namespace pebble_blaster