/**
 * @file application.cpp
 * @brief Runs the Pebble Blaster game application and its main loop.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */
#include "pebble_blaster/application.hpp"

#include "gamepad_input.hpp"
#include "pebble_blaster/core.hpp"
#include "pebble_blaster/keyboard_input.hpp"
#include "pebble_blaster/renderer.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <random>

namespace pebble_blaster {
namespace {

Screen screenFor(const Game &game, bool title) {
    if (title) {
        return Screen::title;
    }
    switch (game.world().phase) {
    case Phase::playing:
        return Screen::game;
    case Phase::paused:
        return Screen::pause;
    case Phase::gameOver:
        return Screen::gameOver;
    }
    return Screen::game;
}

} // namespace

int runGame() {
    sf::RenderWindow window(sf::VideoMode({1200, 800}), "Pebble Blaster", sf::Style::Default);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    Game game(std::random_device{}());
    Renderer renderer;
    window.setView(Renderer::view(window.getSize(), game.bounds()));
    sf::Clock clock;
    float accumulator = 0.0F;
    constexpr float step = 1.0F / 120.0F;
    bool title = true;
    bool focused = true;
    int bestScore = 0;
    bool controllerStartWasPressed = false;
    const auto activate = [&] {
        if (title || game.world().phase == Phase::gameOver) {
            game.restart();
            title = false;
        } else if (game.world().phase == Phase::paused) {
            game.togglePause();
        }
        accumulator = 0.0F;
    };

    while (window.isOpen()) {
        const float frameTime = std::min(clock.restart().asSeconds(), 0.1F);
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto *resized = event->getIf<sf::Event::Resized>()) {
                window.setView(Renderer::view(resized->size, game.bounds()));
            }
            if (event->is<sf::Event::FocusLost>()) {
                focused = false;
                if (!title && game.world().phase == Phase::playing) {
                    game.togglePause();
                }
            }
            if (event->is<sf::Event::FocusGained>()) {
                focused = true;
            }

            switch (keyboardCommand(*event)) {
            case KeyboardCommand::activate:
                activate();
                break;
            case KeyboardCommand::togglePause:
                if (!title) {
                    game.togglePause();
                    accumulator = 0.0F;
                }
                break;
            case KeyboardCommand::restart:
                if (!title) {
                    bestScore = std::max(bestScore, game.world().score);
                    game.restart();
                    accumulator = 0.0F;
                }
                break;
            case KeyboardCommand::none:
                break;
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                const auto point = window.mapPixelToCoords(mouse->position);
                if (mouse->button == sf::Mouse::Button::Left &&
                    screenFor(game, title) != Screen::game &&
                    renderer.actionContains({point.x, point.y}, game.bounds())) {
                    activate();
                }
            }
        }
        if (!window.isOpen()) {
            break;
        }

        const JoystickState controller = readJoystick();
        const bool controllerStartPressed = controller.buttonPressed(7);
        if (focused && controllerStartPressed && !controllerStartWasPressed) {
            if (title || game.world().phase != Phase::playing) {
                activate();
            } else {
                game.togglePause();
                accumulator = 0.0F;
            }
        }
        controllerStartWasPressed = controllerStartPressed;

        Input input;
        if (focused) {
            input = combineInputs(readKeyboardInput(), controller.input);
        }
        if (!title && game.world().phase == Phase::playing && focused) {
            accumulator += frameTime;
            while (accumulator >= step) {
                game.update(step, input);
                accumulator -= step;
            }
        } else {
            accumulator = 0.0F;
        }

        bestScore = std::max(bestScore, game.world().score);
        window.clear(sf::Color::Black);
        renderer.draw(window, game, screenFor(game, title), input.thrust, bestScore);
        window.display();
    }
    return 0;
}

} // namespace pebble_blaster