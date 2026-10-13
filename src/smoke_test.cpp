#include "pebble_blaster/smoke_test.hpp"

#include "pebble_blaster/core.hpp"
#include "pebble_blaster/renderer.hpp"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace pebble_blaster {
namespace {

bool advanceSimulation(Game &game) {
    const Vec2 original = game.world().ship.position;
    for (int step = 0; step < 80; ++step) {
        game.update(1.0F / 120.0F, {.right = true, .thrust = true, .fire = true});
    }
    if (lengthSquared(game.world().ship.position - original) < 1.0F ||
        game.world().bullets.empty()) {
        std::cerr << "Simulation smoke test failed: ship did not move or fire\n";
        return false;
    }
    return true;
}

} // namespace

// Requirements: FR-SMOKE-001
int runSimulationSmokeTest() {
    Game game(42);
    return advanceSimulation(game) ? 0 : 1;
}

// Requirements: FR-RENDER-001
int runRenderSmokeTest(const std::filesystem::path &directory) {
    std::filesystem::create_directories(directory);
    Game game(42);
    if (!advanceSimulation(game)) {
        return 1;
    }

    Renderer renderer;
    World renderWorld = game.world();
    renderWorld.saucer.emplace();
    renderWorld.saucer->position = {game.bounds().x * 0.72F, game.bounds().y * 0.34F};
    const Game renderGame(renderWorld, 42, game.bounds());
    for (const sf::Vector2u size : {sf::Vector2u{960, 640}, sf::Vector2u{600, 800}}) {
        sf::RenderTexture frame(size);
        frame.setView(Renderer::view(size, game.bounds()));
        for (const auto screen : {Screen::title, Screen::game, Screen::pause, Screen::gameOver}) {
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

} // namespace pebble_blaster