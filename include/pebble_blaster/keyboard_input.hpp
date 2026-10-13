#pragma once
/**
 * @file keyboard_input.hpp
 * @brief Declares keyboard input reading and command mapping.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */

#include "pebble_blaster/core.hpp"

#include <SFML/Window/Event.hpp>

namespace pebble_blaster {

enum class KeyboardCommand { none, activate, togglePause, restart };

[[nodiscard]] KeyboardCommand keyboardCommand(const sf::Event &event);
[[nodiscard]] Input readKeyboardInput();

} // namespace pebble_blaster