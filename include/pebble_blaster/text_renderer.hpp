#pragma once

#include <SFML/Graphics/RenderTarget.hpp>

#include <string_view>

namespace pebble_blaster {

class TextRenderer {
  public:
    void draw(sf::RenderTarget &target, std::string_view value, sf::Vector2f position, float scale,
              sf::Color color, bool centered = false) const;
    [[nodiscard]] float width(std::string_view value, float scale) const;
};

} // namespace pebble_blaster