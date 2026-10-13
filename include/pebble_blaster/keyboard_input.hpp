#pragma once

#include "pebble_blaster/core.hpp"

#include <SFML/Window/Event.hpp>

namespace pebble_blaster {

enum class KeyboardCommand { none, activate, togglePause, restart };

[[nodiscard]] KeyboardCommand keyboardCommand(const sf::Event &event);
[[nodiscard]] Input readKeyboardInput();

} // namespace pebble_blaster