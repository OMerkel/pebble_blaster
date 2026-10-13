#include "gamepad_input.hpp"
#include "pebble_blaster/text_renderer.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>

namespace {

constexpr sf::Vector2u windowSize{960, 640};
constexpr sf::Color background{12, 19, 23};
constexpr sf::Color shell{35, 47, 53};
constexpr sf::Color shellOutline{87, 108, 113};
constexpr sf::Color idle{79, 99, 105};
constexpr sf::Color active{114, 245, 183};
constexpr sf::Color textColor{221, 234, 231};
constexpr sf::Color muted{117, 143, 147};

void rectangle(sf::RenderTarget &target, sf::Vector2f position, sf::Vector2f size,
               sf::Color color) {
    sf::RectangleShape shape(size);
    shape.setPosition(position);
    shape.setFillColor(color);
    target.draw(shape);
}

void circle(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color,
            sf::Color outline = sf::Color::Transparent) {
    sf::CircleShape shape(radius);
    shape.setPosition({center.x - radius, center.y - radius});
    shape.setFillColor(color);
    if (outline != sf::Color::Transparent) {
        shape.setOutlineThickness(3.0F);
        shape.setOutlineColor(outline);
    }
    target.draw(shape);
}

void label(const pebble_blaster::TextRenderer &font, sf::RenderTarget &target, std::string value,
           pebble_blaster::Vec2 position, float scale, sf::Color color) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    font.draw(target, value, {position.x, position.y}, scale, color);
}

void drawDpad(const pebble_blaster::TextRenderer &font, sf::RenderTarget &target,
              const pebble_blaster::JoystickState &state) {
    const sf::Vector2f center{262.0F, 278.0F};
    const auto directionColor = [&](bool pressed) { return pressed ? active : idle; };
    rectangle(target, {center.x - 13.0F, center.y - 48.0F}, {26.0F, 96.0F},
              idle);
    rectangle(target, {center.x - 48.0F, center.y - 13.0F}, {96.0F, 26.0F},
              idle);
    rectangle(target, {center.x - 13.0F, center.y - 48.0F}, {26.0F, 27.0F},
              directionColor(state.hasPovY && state.povY < -50.0F));
    rectangle(target, {center.x - 48.0F, center.y - 13.0F}, {27.0F, 26.0F},
              directionColor(state.hasPovX && state.povX < -50.0F));
    rectangle(target, {center.x + 21.0F, center.y - 13.0F}, {27.0F, 26.0F},
              directionColor(state.hasPovX && state.povX > 50.0F));
    rectangle(target, {center.x - 13.0F, center.y + 21.0F}, {26.0F, 27.0F},
              directionColor(state.hasPovY && state.povY > 50.0F));
    label(font, target, "DPAD", {center.x - 24.0F, center.y + 67.0F}, 2.0F, textColor);
}

void drawStick(const pebble_blaster::TextRenderer &font, sf::RenderTarget &target,
               const pebble_blaster::JoystickState &state) {
    const sf::Vector2f center{385.0F, 268.0F};
    const float normalizedX = state.x / 100.0F;
    const float normalizedY = state.y / 100.0F;
    const bool moved = std::abs(normalizedX) > 0.2F || std::abs(normalizedY) > 0.2F;
    circle(target, center, 48.0F, {22, 31, 36}, moved ? active : shellOutline);
    circle(target, {center.x + normalizedX * 30.0F, center.y + normalizedY * 30.0F},
           17.0F, moved ? active : idle);
    label(font, target, "LEFT STICK", {center.x - 48.0F, center.y + 67.0F}, 2.0F,
          textColor);
}

void drawFaceButton(const pebble_blaster::TextRenderer &font, sf::RenderTarget &target,
                    const pebble_blaster::JoystickState &state, unsigned int button,
                    sf::Vector2f center) {
    const bool pressed = state.buttonPressed(button);
    circle(target, center, 22.0F, pressed ? active : idle,
           button == 0 ? active : sf::Color::Transparent);
    label(font, target, std::to_string(button), {center.x - 6.0F, center.y - 7.0F}, 2.0F,
          pressed ? background : textColor);
}

void drawController(const pebble_blaster::TextRenderer &font, sf::RenderWindow &window,
                    const pebble_blaster::JoystickState &state) {
    sf::ConvexShape body(18);
    const std::array<sf::Vector2f, 18> points{{
        {188.0F, 243.0F}, {234.0F, 198.0F}, {333.0F, 184.0F}, {394.0F, 208.0F},
        {566.0F, 208.0F}, {627.0F, 184.0F}, {726.0F, 198.0F}, {772.0F, 243.0F},
        {807.0F, 366.0F}, {785.0F, 423.0F}, {748.0F, 437.0F}, {703.0F, 414.0F},
        {646.0F, 352.0F}, {314.0F, 352.0F}, {257.0F, 414.0F}, {212.0F, 437.0F},
        {175.0F, 423.0F}, {153.0F, 366.0F},
    }};
    for (std::size_t index = 0; index < points.size(); ++index) {
        body.setPoint(index, points[index]);
    }
    body.setFillColor(shell);
    body.setOutlineThickness(3.0F);
    body.setOutlineColor(shellOutline);
    window.draw(body);

    drawDpad(font, window, state);
    drawStick(font, window, state);

    circle(window, {580.0F, 280.0F}, 45.0F, {27, 37, 42}, shellOutline);
    circle(window, {580.0F, 280.0F}, 19.0F, idle);

    drawFaceButton(font, window, state, 0, {705.0F, 306.0F});
    drawFaceButton(font, window, state, 1, {744.0F, 267.0F});
    drawFaceButton(font, window, state, 2, {666.0F, 267.0F});
    drawFaceButton(font, window, state, 3, {705.0F, 228.0F});

    const bool startPressed = state.buttonPressed(7);
        rectangle(window, {455.0F, 250.0F}, {54.0F, 22.0F}, startPressed ? active : idle);
        label(font, window, "7", {477.0F, 254.0F}, 2.0F,
          startPressed ? background : textColor);
        label(font, window, "START", {458.0F, 280.0F}, 2.0F, textColor);
}

void drawButtonGrid(const pebble_blaster::TextRenderer &font, sf::RenderWindow &window,
                    const pebble_blaster::JoystickState &state) {
    label(font, window, "DETECTED BUTTONS", {30.0F, 492.0F}, 2.0F, muted);
    for (unsigned int button = 0; button < sf::Joystick::ButtonCount; ++button) {
        const float x = 30.0F + static_cast<float>(button % 8) * 112.0F;
        const float y = 514.0F + static_cast<float>(button / 8) * 27.0F;
        const bool detected = state.buttonPressed(button);
        const sf::Color fill = detected ? active : (button < state.buttonCount ? shellOutline : background);
        rectangle(window, {x, y}, {92.0F, 22.0F}, fill);
        const sf::Color ink = detected ? background : muted;
        label(font, window, std::to_string(button), {x + 7.0F, y + 3.0F}, 2.0F, ink);
    }
}

void drawStatus(const pebble_blaster::TextRenderer &font, sf::RenderWindow &window,
                const pebble_blaster::JoystickState &state) {
    const sf::Color connectedColor = state.index ? active : sf::Color{255, 142, 116};
    label(font, window, "GAMEPAD INPUT TEST", {30.0F, 22.0F}, 3.0F, textColor);
    if (!state.index) {
        label(font, window, "NO CONTROLLER CONNECTED", {30.0F, 72.0F}, 2.0F, connectedColor);
        label(font, window, "CONNECT A GAMEPAD TO SEE LIVE INPUT", {30.0F, 101.0F}, 2.0F,
              muted);
        return;
    }

    std::string device = "DEVICE ";
    device += state.identification.name.toAnsiString();
    device += " INDEX ";
    device += std::to_string(*state.index);
    label(font, window, device, {30.0F, 67.0F}, 2.0F, connectedColor);

    std::string axes = "X ";
    axes += std::to_string(static_cast<int>(state.x));
    axes += " Y ";
    axes += std::to_string(static_cast<int>(state.y));
    axes += " POVX ";
    axes += std::to_string(static_cast<int>(state.povX));
    axes += " POVY ";
    axes += std::to_string(static_cast<int>(state.povY));
    label(font, window, axes, {30.0F, 96.0F}, 2.0F, muted);

    const auto mappingColor = [](bool activeState) { return activeState ? active : muted; };
        const std::string turnLabel = state.input.turn < 0.0F
                              ? "TURN LEFT"
                              : (state.input.turn > 0.0F ? "TURN RIGHT" : "TURN");
        label(font, window, turnLabel, {30.0F, 130.0F}, 2.0F,
            mappingColor(state.input.turn != 0.0F));
    label(font, window, "THRUST", {250.0F, 130.0F}, 2.0F,
          mappingColor(state.input.thrust));
    label(font, window, "FIRE B0 B1", {420.0F, 130.0F}, 2.0F,
          mappingColor(state.input.fire));
    label(font, window, "START B7", {590.0F, 130.0F}, 2.0F,
          mappingColor(state.buttonPressed(7)));

    std::string buttonInfo = "BUTTON COUNT ";
    buttonInfo += std::to_string(state.buttonCount);
    label(font, window, buttonInfo, {30.0F, 160.0F}, 2.0F, muted);
}

} // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode(windowSize), "Pebble Blaster - Gamepad Test");
    window.setVerticalSyncEnabled(true);
    pebble_blaster::TextRenderer font;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        const pebble_blaster::JoystickState state = pebble_blaster::readJoystick();
        window.clear(background);
        drawStatus(font, window, state);
        drawController(font, window, state);
        drawButtonGrid(font, window, state);
        window.display();
    }

    return 0;
}