#pragma once

#include "pebble_blaster/core.hpp"

#include <SFML/Graphics.hpp>

#include <string_view>

namespace pebble_blaster {

enum class Screen { title, game, pause, gameOver };

class Renderer {
  public:
    void draw(sf::RenderTarget &target, const Game &game, Screen screen, bool thrust,
              int bestScore) const;
    [[nodiscard]] bool actionContains(Vec2 point, Vec2 bounds) const;
    [[nodiscard]] static sf::View view(sf::Vector2u pixels, Vec2 bounds);

  private:
    void text(sf::RenderTarget &target, std::string_view value, Vec2 position, float scale,
              sf::Color color, bool centered = false) const;
    void ship(sf::RenderTarget &target, const Ship &ship, Vec2 bounds, bool thrust,
              float time) const;
    void rock(sf::RenderTarget &target, const Rock &rock, Vec2 bounds) const;
};

}