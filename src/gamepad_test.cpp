#include "pebble_blaster/renderer.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Joystick.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>

namespace {

constexpr sf::Vector2u windowSize{1000, 680};
constexpr sf::Color background{12, 19, 23};
constexpr sf::Color shell{35, 47, 53};
constexpr sf::Color shellOutline{87, 108, 113};
constexpr sf::Color idle{79, 99, 105};
constexpr sf::Color active{114, 245, 183};
constexpr sf::Color textColor{221, 234, 231};
constexpr sf::Color muted{117, 143, 147};

float axisPosition(unsigned int joystick, sf::Joystick::Axis axis) {
    return sf::Joystick::hasAxis(joystick, axis)
               ? sf::Joystick::getAxisPosition(joystick, axis)
               : 0.0F;
}

bool axisActive(unsigned int joystick, sf::Joystick::Axis axis) {
    return sf::Joystick::hasAxis(joystick, axis) &&
           std::abs(sf::Joystick::getAxisPosition(joystick, axis)) > 20.0F;
}

bool buttonPressed(unsigned int joystick, unsigned int button, unsigned int count) {
    return button < count && sf::Joystick::isButtonPressed(joystick, button);
}

void label(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target, std::string value,
           pebble_blaster::Vec2 position, float scale, sf::Color color) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    renderer.text(target, value, position, scale, color);
}

std::string axisLabel(std::string_view name, float value) {
    std::string result(name);
    result += value < 0.0F ? " NEG " : " POS ";
    result += std::to_string(static_cast<int>(std::abs(value)));
    return result;
}

void circle(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color fill,
            sf::Color outline) {
    sf::CircleShape shape(radius);
    shape.setOrigin({radius, radius});
    shape.setPosition(center);
    shape.setFillColor(fill);
    shape.setOutlineThickness(3.0F);
    shape.setOutlineColor(outline);
    target.draw(shape);
}

void rectangle(sf::RenderTarget &target, sf::Vector2f position, sf::Vector2f size,
               sf::Color color) {
    sf::RectangleShape shape(size);
    shape.setPosition(position);
    shape.setFillColor(color);
    target.draw(shape);
}

void drawControllerContour(sf::RenderTarget &target) {
    sf::ConvexShape body(20);
    const std::array<sf::Vector2f, 20> points{{
        {161.0F, 231.0F}, {193.0F, 174.0F}, {286.0F, 141.0F}, {366.0F, 159.0F},
        {424.0F, 202.0F}, {576.0F, 202.0F}, {634.0F, 159.0F}, {714.0F, 141.0F},
        {807.0F, 174.0F}, {839.0F, 231.0F}, {878.0F, 415.0F}, {856.0F, 472.0F},
        {802.0F, 500.0F}, {749.0F, 462.0F}, {688.0F, 390.0F}, {312.0F, 390.0F},
        {251.0F, 462.0F}, {198.0F, 500.0F}, {144.0F, 472.0F}, {122.0F, 415.0F},
    }};
    for (std::size_t index = 0; index < points.size(); ++index) {
        body.setPoint(index, points[index]);
    }
    body.setFillColor(shell);
    body.setOutlineThickness(3.0F);
    body.setOutlineColor(shellOutline);
    target.draw(body);
}

void drawStick(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
         sf::Vector2f center, float x, float y, bool detected, bool clicked,
         std::string_view title, std::string_view xName, std::string_view yName) {
    const bool moved = detected && (std::abs(x) > 20.0F || std::abs(y) > 20.0F);
    circle(target, center, 65.0F, {22, 31, 36}, moved || clicked ? active : shellOutline);
    const sf::Vector2f marker{center.x + x * 0.45F, center.y + y * 0.45F};
    circle(target, marker, 21.0F, moved || clicked ? active : idle,
        moved || clicked ? active : shellOutline);
    label(renderer, target, std::string(title), {center.x - 78.0F, center.y + 83.0F}, 2.0F,
          detected ? textColor : muted);
    label(renderer, target, axisLabel(xName, x), {center.x - 82.0F, center.y + 111.0F}, 1.5F,
          detected ? textColor : muted);
    label(renderer, target, axisLabel(yName, y), {center.x - 82.0F, center.y + 133.0F}, 1.5F,
          detected ? textColor : muted);
}

void drawDpad(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
              unsigned int joystick) {
    constexpr sf::Vector2f center{250.0F, 280.0F};
    const float x = axisPosition(joystick, sf::Joystick::Axis::PovX);
    const float y = axisPosition(joystick, sf::Joystick::Axis::PovY);
    rectangle(target, {center.x - 13.0F, center.y - 45.0F}, {26.0F, 90.0F}, idle);
    rectangle(target, {center.x - 45.0F, center.y - 13.0F}, {90.0F, 26.0F}, idle);
    rectangle(target, {center.x - 13.0F, center.y - 45.0F}, {26.0F, 26.0F},
              y > 50.0F ? active : idle);
    rectangle(target, {center.x - 45.0F, center.y - 13.0F}, {26.0F, 26.0F},
              x < -50.0F ? active : idle);
    rectangle(target, {center.x + 19.0F, center.y - 13.0F}, {26.0F, 26.0F},
              x > 50.0F ? active : idle);
    rectangle(target, {center.x - 13.0F, center.y + 19.0F}, {26.0F, 26.0F},
              y < -50.0F ? active : idle);
    label(renderer, target, "POV DPAD", {center.x - 40.0F, center.y + 62.0F}, 2.0F, textColor);
    label(renderer, target, axisLabel("POVX", x), {center.x - 45.0F, center.y + 90.0F}, 1.5F,
          muted);
    label(renderer, target, axisLabel("POVY", y), {center.x - 45.0F, center.y + 112.0F}, 1.5F,
          muted);
}

    void drawShellButton(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
                   unsigned int joystick, unsigned int buttonCount, unsigned int button,
                   sf::Vector2f center, sf::Vector2f size) {
        const bool available = button < buttonCount;
        const bool pressed = available && buttonPressed(joystick, button, buttonCount);
        rectangle(target, {center.x - size.x * 0.5F, center.y - size.y * 0.5F}, size,
              pressed ? active : (available ? shellOutline : idle));
        label(renderer, target, std::to_string(button), {center.x - 4.0F, center.y - 6.0F},
            1.5F, pressed ? background : textColor);
    }

    void drawFaceButton(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
                  unsigned int joystick, unsigned int buttonCount, unsigned int button,
                  sf::Vector2f center) {
        const bool available = button < buttonCount;
        const bool pressed = available && buttonPressed(joystick, button, buttonCount);
        circle(target, center, 19.0F, pressed ? active : idle,
             available ? shellOutline : muted);
        label(renderer, target, std::to_string(button), {center.x - 4.0F, center.y - 6.0F},
            1.5F, pressed ? background : textColor);
    }

    void drawShellButtons(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
                    unsigned int joystick, unsigned int buttonCount) {
        drawFaceButton(renderer, target, joystick, buttonCount, 0, {724.0F, 279.0F});
        drawFaceButton(renderer, target, joystick, buttonCount, 1, {760.0F, 315.0F});
        drawFaceButton(renderer, target, joystick, buttonCount, 2, {796.0F, 279.0F});
        drawFaceButton(renderer, target, joystick, buttonCount, 3, {760.0F, 243.0F});

        drawShellButton(renderer, target, joystick, buttonCount, 4, {336.0F, 190.0F},
                  {62.0F, 22.0F});
        drawShellButton(renderer, target, joystick, buttonCount, 5, {664.0F, 190.0F},
                  {62.0F, 22.0F});
        drawShellButton(renderer, target, joystick, buttonCount, 6, {336.0F, 232.0F},
                  {48.0F, 20.0F});
        drawShellButton(renderer, target, joystick, buttonCount, 7, {664.0F, 232.0F},
                  {48.0F, 20.0F});
        drawShellButton(renderer, target, joystick, buttonCount, 13, {500.0F, 232.0F},
              {104.0F, 30.0F});
          drawShellButton(renderer, target, joystick, buttonCount, 8, {470.0F, 275.0F},
                {48.0F, 20.0F});
          drawShellButton(renderer, target, joystick, buttonCount, 9, {530.0F, 275.0F},
                {48.0F, 20.0F});
          const bool psAvailable = 12 < buttonCount;
          const bool psPressed = psAvailable && buttonPressed(joystick, 12, buttonCount);
          circle(target, {500.0F, 350.0F}, 19.0F, psPressed ? active : idle,
               psAvailable ? shellOutline : muted);
          label(renderer, target, "PS", {490.0F, 344.0F}, 1.5F,
              psPressed ? background : textColor);
          label(renderer, target, "12", {493.0F, 375.0F}, 1.5F,
              psAvailable ? textColor : muted);
    }

void drawButtons(const pebble_blaster::Renderer &renderer, sf::RenderTarget &target,
                 unsigned int joystick, unsigned int buttonCount) {
    label(renderer, target, "BUTTONS", {48.0F, 515.0F}, 2.0F, muted);
    for (unsigned int button = 0; button < sf::Joystick::ButtonCount; ++button) {
        const float x = 48.0F + static_cast<float>(button % 8) * 114.0F;
        const float y = 545.0F + static_cast<float>(button / 8) * 29.0F;
        const bool available = button < buttonCount;
        const bool pressed = available && buttonPressed(joystick, button, buttonCount);
        rectangle(target, {x, y}, {94.0F, 23.0F}, pressed ? active : (available ? shellOutline : background));
        label(renderer, target, std::to_string(button), {x + 8.0F, y + 3.0F}, 1.5F,
              pressed ? background : muted);
    }
}

void draw(sf::RenderWindow &window, const pebble_blaster::Renderer &renderer) {
    sf::Joystick::update();
    unsigned int joystick = sf::Joystick::Count;
    for (unsigned int index = 0; index < sf::Joystick::Count; ++index) {
        if (sf::Joystick::isConnected(index)) {
            joystick = index;
            break;
        }
    }

    window.clear(background);
    label(renderer, window, "GAMEPAD INPUT TEST", {32.0F, 22.0F}, 3.0F, textColor);
    if (joystick == sf::Joystick::Count) {
        label(renderer, window, "NO CONTROLLER CONNECTED", {32.0F, 76.0F}, 2.0F,
              sf::Color{255, 142, 116});
        window.display();
        return;
    }

    const auto identification = sf::Joystick::getIdentification(joystick);
    const unsigned int buttonCount = sf::Joystick::getButtonCount(joystick);
    const std::string device = "DEVICE " + identification.name.toAnsiString() + " INDEX " +
                               std::to_string(joystick);
    label(renderer, window, device, {32.0F, 72.0F}, 2.0F, active);
    drawControllerContour(window);

    const float leftX = axisPosition(joystick, sf::Joystick::Axis::X);
    const float leftY = axisPosition(joystick, sf::Joystick::Axis::Y);
    const float rightX = axisPosition(joystick, sf::Joystick::Axis::Z);
    const float rightY = axisPosition(joystick, sf::Joystick::Axis::R);
    drawStick(renderer, window, {390.0F, 330.0F}, leftX, leftY,
              sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::X) &&
                  sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::Y),
              buttonPressed(joystick, 10, buttonCount), "LEFT STICK B10", "X", "Y");
    drawDpad(renderer, window, joystick);
    drawStick(renderer, window, {610.0F, 330.0F}, rightX, rightY,
              sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::Z) &&
                  sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::R),
              buttonPressed(joystick, 11, buttonCount), "RIGHT STICK B11", "Z", "R");

        drawShellButtons(renderer, window, joystick, buttonCount);
        label(renderer, window,
            axisLabel("AUX U", axisPosition(joystick, sf::Joystick::Axis::U)),
            {280.0F, 145.0F}, 1.5F,
            sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::U) ? textColor : muted);
        label(renderer, window,
            axisLabel("AUX V", axisPosition(joystick, sf::Joystick::Axis::V)),
            {610.0F, 145.0F}, 1.5F,
            sf::Joystick::hasAxis(joystick, sf::Joystick::Axis::V) ? textColor : muted);
    drawButtons(renderer, window, joystick, buttonCount);
    window.display();
}

} // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode(windowSize), "Pebble Blaster - Gamepad Input Test");
    window.setVerticalSyncEnabled(true);
    pebble_blaster::Renderer renderer;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        draw(window, renderer);
    }
    return 0;
}