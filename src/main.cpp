#include "pebble_blaster/core.hpp"
#include "pebble_blaster/renderer.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>
#include <string_view>

namespace {

pebble_blaster::Screen screenFor(const pebble_blaster::Game &game, bool title) {
    if (title) {
        return pebble_blaster::Screen::title;
    }
    switch (game.world().phase) {
    case pebble_blaster::Phase::playing:
        return pebble_blaster::Screen::game;
    case pebble_blaster::Phase::paused:
        return pebble_blaster::Screen::pause;
    case pebble_blaster::Phase::gameOver:
        return pebble_blaster::Screen::gameOver;
    }
    return pebble_blaster::Screen::game;
}

bool runSimulationSmokeTest(pebble_blaster::Game &game) {
    const pebble_blaster::Vec2 original = game.world().ship.position;
    for (int step = 0; step < 80; ++step) {
        game.update(1.0F / 120.0F, {.right = true, .thrust = true, .fire = true});
    }
    if (pebble_blaster::lengthSquared(game.world().ship.position - original) < 1.0F ||
        game.world().bullets.empty()) {
        std::cerr << "Simulation smoke test failed: ship did not move or fire\n";
        return false;
    }
    return true;
}

int simulationSmokeTest() {
    pebble_blaster::Game game(42);
    return runSimulationSmokeTest(game) ? 0 : 1;
}

int smokeTest(const std::filesystem::path &directory) {
    std::filesystem::create_directories(directory);
    pebble_blaster::Game game(42);
    if (!runSimulationSmokeTest(game)) {
        return 1;
    }
    pebble_blaster::Renderer renderer;
    pebble_blaster::World renderWorld = game.world();
    renderWorld.saucer.emplace();
    renderWorld.saucer->position = {game.bounds().x * 0.72F, game.bounds().y * 0.34F};
    const pebble_blaster::Game renderGame(renderWorld, 42, game.bounds());
    for (const sf::Vector2u size : {sf::Vector2u{960, 640}, sf::Vector2u{600, 800}}) {
        sf::RenderTexture frame(size);
        frame.setView(pebble_blaster::Renderer::view(size, game.bounds()));
        for (const auto screen : {pebble_blaster::Screen::title, pebble_blaster::Screen::game,
                                  pebble_blaster::Screen::pause, pebble_blaster::Screen::gameOver}) {
            frame.clear(sf::Color::Black);
            renderer.draw(frame, renderGame, screen, true, 1000);
            frame.display();
            const sf::Image image = frame.getTexture().copyToImage();
            std::size_t litPixels = 0;
            for (unsigned row = 0; row < size.y; ++row) {
                for (unsigned column = 0; column < size.x; ++column) {
                    const auto pixel = image.getPixel({column, row});
                    if (pixel.r > 100 || pixel.g > 100 || pixel.b > 100) {
                        ++litPixels;
                    }
                }
            }
            const std::string filename =
                std::to_string(size.x) + "-" + std::to_string(static_cast<int>(screen)) + ".png";
            if (litPixels < 800 || !image.saveToFile(directory / filename)) {
                std::cerr << "Render smoke test failed: " << filename << '\n';
                return 1;
            }
            std::cout << filename << ": " << litPixels << " visible pixels\n";
        }
    }
    return 0;
}

int play() {
    sf::RenderWindow window(sf::VideoMode({1200, 800}), "Pebble Blaster", sf::Style::Default);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    pebble_blaster::Game game(std::random_device{}());
    pebble_blaster::Renderer renderer;
    window.setView(pebble_blaster::Renderer::view(window.getSize(), game.bounds()));
    sf::Clock clock;
    float accumulator = 0.0F;
    constexpr float step = 1.0F / 120.0F;
    bool title = true;
    bool focused = true;
    int bestScore = 0;
    const auto activate = [&] {
        if (title || game.world().phase == pebble_blaster::Phase::gameOver) {
            game.restart();
            title = false;
        } else if (game.world().phase == pebble_blaster::Phase::paused) {
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
                window.setView(pebble_blaster::Renderer::view(resized->size, game.bounds()));
            }
            if (event->is<sf::Event::FocusLost>()) {
                focused = false;
                if (!title && game.world().phase == pebble_blaster::Phase::playing) {
                    game.togglePause();
                }
            }
            if (event->is<sf::Event::FocusGained>()) {
                focused = true;
            }
            if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
                using Key = sf::Keyboard::Scancode;
                if (key->scancode == Key::Enter) {
                    activate();
                }
                if ((key->scancode == Key::P || key->scancode == Key::Escape) && !title) {
                    game.togglePause();
                    accumulator = 0.0F;
                }
                if (key->scancode == Key::R && !title) {
                    bestScore = std::max(bestScore, game.world().score);
                    game.restart();
                    accumulator = 0.0F;
                }
            }
            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                const auto point = window.mapPixelToCoords(mouse->position);
                if (mouse->button == sf::Mouse::Button::Left &&
                    screenFor(game, title) != pebble_blaster::Screen::game &&
                    renderer.actionContains({point.x, point.y}, game.bounds())) {
                    activate();
                }
            }
        }
        if (!window.isOpen()) {
            break;
        }
        pebble_blaster::Input input;
        if (focused) {
            using Key = sf::Keyboard::Scancode;
            input.left =
                sf::Keyboard::isKeyPressed(Key::Left) || sf::Keyboard::isKeyPressed(Key::A);
            input.right =
                sf::Keyboard::isKeyPressed(Key::Right) || sf::Keyboard::isKeyPressed(Key::D);
            input.thrust =
                sf::Keyboard::isKeyPressed(Key::Up) || sf::Keyboard::isKeyPressed(Key::W);
            input.fire = sf::Keyboard::isKeyPressed(Key::Space);
        }
        if (!title && game.world().phase == pebble_blaster::Phase::playing && focused) {
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

}

int main(int argumentCount, char *arguments[]) {
    try {
        if (argumentCount == 2 && std::string_view(arguments[1]) == "--simulation-smoke-test") {
            return simulationSmokeTest();
        }
        if (argumentCount >= 2 && std::string_view(arguments[1]) == "--smoke-test") {
            return smokeTest(argumentCount >= 3 ? arguments[2] : ".artifacts/screenshots");
        }
        if (argumentCount != 1) {
                std::cerr << "Usage: pebble_blaster [--simulation-smoke-test | --smoke-test [output-directory]]\n";
            return 2;
        }
        return play();
    } catch (const std::exception &error) {
            std::cerr << "Pebble Blaster: " << error.what() << '\n';
        return 1;
    }
}