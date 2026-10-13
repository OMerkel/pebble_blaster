#include "pebble_blaster/text_renderer.hpp"

#include <SFML/Graphics/VertexArray.hpp>

#include <array>

namespace pebble_blaster {
namespace {

constexpr unsigned glyphRows = 7;
constexpr unsigned glyphColumns = 5;
constexpr float glyphAdvance = 6.0F;
constexpr float glyphTrailingGap = 1.0F;

std::array<unsigned, glyphRows> glyph(char character) {
    switch (character) {
    case 'A':
        return {14, 17, 17, 31, 17, 17, 17};
    case 'B':
        return {30, 17, 17, 30, 17, 17, 30};
    case 'C':
        return {14, 17, 16, 16, 16, 17, 14};
    case 'D':
        return {30, 17, 17, 17, 17, 17, 30};
    case 'E':
        return {31, 16, 16, 30, 16, 16, 31};
    case 'F':
        return {31, 16, 16, 30, 16, 16, 16};
    case 'G':
        return {14, 17, 16, 23, 17, 17, 15};
    case 'H':
        return {17, 17, 17, 31, 17, 17, 17};
    case 'I':
        return {14, 4, 4, 4, 4, 4, 14};
    case 'J':
        return {7, 2, 2, 2, 18, 18, 12};
    case 'K':
        return {17, 18, 20, 24, 20, 18, 17};
    case 'L':
        return {16, 16, 16, 16, 16, 16, 31};
    case 'M':
        return {17, 27, 21, 21, 17, 17, 17};
    case 'N':
        return {17, 25, 21, 19, 17, 17, 17};
    case 'O':
        return {14, 17, 17, 17, 17, 17, 14};
    case 'P':
        return {30, 17, 17, 30, 16, 16, 16};
    case 'Q':
        return {14, 17, 17, 17, 21, 18, 13};
    case 'R':
        return {30, 17, 17, 30, 20, 18, 17};
    case 'S':
        return {15, 16, 16, 14, 1, 1, 30};
    case 'T':
        return {31, 4, 4, 4, 4, 4, 4};
    case 'U':
        return {17, 17, 17, 17, 17, 17, 14};
    case 'V':
        return {17, 17, 17, 17, 17, 10, 4};
    case 'W':
        return {17, 17, 17, 21, 21, 21, 10};
    case 'X':
        return {17, 17, 10, 4, 10, 17, 17};
    case 'Y':
        return {17, 17, 10, 4, 4, 4, 4};
    case 'Z':
        return {31, 1, 2, 4, 8, 16, 31};
    case '0':
        return {14, 17, 19, 21, 25, 17, 14};
    case '1':
        return {4, 12, 4, 4, 4, 4, 14};
    case '2':
        return {14, 17, 1, 2, 4, 8, 31};
    case '3':
        return {30, 1, 1, 14, 1, 1, 30};
    case '4':
        return {2, 6, 10, 18, 31, 2, 2};
    case '5':
        return {31, 16, 16, 30, 1, 1, 30};
    case '6':
        return {14, 16, 16, 30, 17, 17, 14};
    case '7':
        return {31, 1, 2, 4, 8, 8, 8};
    case '8':
        return {14, 17, 17, 14, 17, 17, 14};
    case '9':
        return {14, 17, 17, 15, 1, 1, 14};
    default:
        return {};
    }
}

} // namespace

float TextRenderer::width(std::string_view value, float scale) const {
    if (value.empty()) {
        return 0.0F;
    }
    return (static_cast<float>(value.size()) * glyphAdvance - glyphTrailingGap) * scale;
}

void TextRenderer::draw(sf::RenderTarget &target, std::string_view value, sf::Vector2f position,
                        float scale, sf::Color color, bool centered) const {
    if (centered) {
        position.x -= width(value, scale) * 0.5F;
    }

    sf::VertexArray pixels(sf::PrimitiveType::Triangles);
    for (const char character : value) {
        const auto rows = glyph(character);
        for (unsigned row = 0; row < glyphRows; ++row) {
            for (unsigned column = 0; column < glyphColumns; ++column) {
                if ((rows[row] & (1U << (glyphColumns - 1U - column))) == 0) {
                    continue;
                }
                const float left = position.x + static_cast<float>(column) * scale;
                const float top = position.y + static_cast<float>(row) * scale;
                for (const sf::Vector2f corner :
                     std::array<sf::Vector2f, 6>{sf::Vector2f{left, top},
                                                 {left + scale, top},
                                                 {left, top + scale},
                                                 {left, top + scale},
                                                 {left + scale, top},
                                                 {left + scale, top + scale}}) {
                    pixels.append(sf::Vertex{corner, color});
                }
            }
        }
        position.x += glyphAdvance * scale;
    }
    target.draw(pixels);
}

} // namespace pebble_blaster