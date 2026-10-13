/**
 * @file renderer.cpp
 * @brief Implements rendering for game scenes and interface elements.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */
#include "pebble_blaster/renderer.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace pebble_blaster {
namespace {

namespace hmi {

constexpr float edgeInset = 32.0F;
constexpr float labelY = 26.0F;
constexpr float valueY = 50.0F;
constexpr float livesLabelY = 52.0F;
constexpr float lifeIconY = 92.0F;
constexpr float lifeIconRightInset = 44.0F;
constexpr float lifeIconSpacing = 28.0F;
constexpr int noLives = 0;
constexpr float noProtection = 0.0F;
constexpr int maxLifeIcons = 8;
constexpr float labelScale = 2.0F;
constexpr float highScoreScale = 3.0F;
constexpr float scoreScale = 4.0F;
constexpr float bandHeight = 120.0F;
constexpr float sectorClearOffset = 28.0F;
constexpr float sectorClearScale = 4.0F;

constexpr float actionHalfWidth = 140.0F;
constexpr float actionWidth = actionHalfWidth * 2.0F;
constexpr float actionHeight = 56.0F;
constexpr float actionTopOffset = 60.0F;
constexpr float actionTextOffset = 77.0F;
constexpr float actionTextScale = 3.0F;
constexpr float menuHeadingOffset = 76.0F;
constexpr float menuHeadingScale = 8.0F;
constexpr float gameOverScoreOffset = 8.0F;
constexpr float gameOverScoreScale = 3.0F;
constexpr float engineFlameLength = 17.0F;
constexpr float engineFlamePulse = 8.0F;
constexpr float engineFlameFrequency = 60.0F;
constexpr float engineRootX = -11.0F;
constexpr float engineUpperY = -5.0F;
constexpr float engineLowerY = 5.0F;
constexpr float engineUpperTipY = -8.0F;
constexpr float engineLowerTipY = 8.0F;
constexpr float engineUpperReturnY = -3.0F;
constexpr float engineLowerReturnY = 3.0F;
constexpr float engineReturnX = -17.0F;

const std::vector<Vec2> playerCraftOutline{
    {24.0F, 0.0F},  {8.0F, -4.0F},  {3.0F, -15.0F}, {-2.0F, -10.0F}, {-14.0F, -13.0F},
    {-10.0F, -5.0F}, {-20.0F, -2.0F}, {-13.0F, 0.0F}, {-20.0F, 2.0F}, {-10.0F, 5.0F},
    {-14.0F, 13.0F}, {-2.0F, 10.0F}, {3.0F, 15.0F},  {8.0F, 4.0F},   {24.0F, 0.0F}};
const std::vector<Vec2> playerCockpit{
    {8.0F, 0.0F}, {2.0F, -4.0F}, {-5.0F, 0.0F}, {2.0F, 4.0F}, {8.0F, 0.0F}};
const std::vector<Vec2> enemyMantaHull{
    {22.0F, 0.0F}, {8.0F, -7.0F}, {-4.0F, -9.0F}, {-14.0F, -6.0F}, {-9.0F, 0.0F},
    {-14.0F, 6.0F}, {-4.0F, 9.0F}, {8.0F, 7.0F}, {22.0F, 0.0F}};
const std::vector<Vec2> enemyMantaCanopy{
    {10.0F, 0.0F}, {4.0F, -3.0F}, {-3.0F, -2.0F}, {-6.0F, 0.0F}, {-3.0F, 2.0F},
    {4.0F, 3.0F}, {10.0F, 0.0F}};
const std::vector<Vec2> enemyMantaEngines{
    {-9.0F, -3.0F}, {-14.0F, -3.0F}, {-9.0F, 3.0F}, {-14.0F, 3.0F}};

const sf::Color background{7, 13, 15};
const sf::Color hudBand{7, 13, 15, 230};
const sf::Color overlay{7, 13, 15, 190};
const sf::Color ink{220, 233, 232};
const sf::Color accent{115, 246, 187};
const sf::Color muted{106, 134, 139};
const sf::Color warning{255, 183, 95};
const sf::Color hostileBullet{255, 116, 115};

}

void polyline(sf::RenderTarget &target, const std::vector<Vec2> &points, Vec2 position, float angle,
              sf::Color color) {
    sf::VertexArray vertices(sf::PrimitiveType::LineStrip, points.size());
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    for (std::size_t index = 0; index < points.size(); ++index) {
        const Vec2 local = points[index];
        vertices[index].position = {position.x + local.x * cosine - local.y * sine,
                                    position.y + local.x * sine + local.y * cosine};
        vertices[index].color = color;
    }
    target.draw(vertices);
}

template <typename Draw> void tiled(Vec2 position, Vec2 bounds, Draw draw) {
    for (const float horizontal : {-bounds.x, 0.0F, bounds.x}) {
        for (const float vertical : {-bounds.y, 0.0F, bounds.y}) {
            draw(position + Vec2{horizontal, vertical});
        }
    }
}

void rectangle(sf::RenderTarget &target, Vec2 position, Vec2 size, sf::Color color) {
    sf::RectangleShape shape({size.x, size.y});
    shape.setPosition({position.x, position.y});
    shape.setFillColor(color);
    target.draw(shape);
}

}

sf::View Renderer::view(sf::Vector2u pixels, Vec2 bounds) {
    sf::View camera(sf::FloatRect({0.0F, 0.0F}, {bounds.x, bounds.y}));
    if (pixels.x == 0 || pixels.y == 0) {
        return camera;
    }
    const float windowRatio = static_cast<float>(pixels.x) / static_cast<float>(pixels.y);
    const float worldRatio = bounds.x / bounds.y;
    if (windowRatio > worldRatio) {
        const float width = worldRatio / windowRatio;
        camera.setViewport(sf::FloatRect({(1.0F - width) * 0.5F, 0.0F}, {width, 1.0F}));
    } else {
        const float height = windowRatio / worldRatio;
        camera.setViewport(sf::FloatRect({0.0F, (1.0F - height) * 0.5F}, {1.0F, height}));
    }
    return camera;
}

bool Renderer::actionContains(Vec2 point, Vec2 bounds) const {
    return point.x >= bounds.x * 0.5F - hmi::actionHalfWidth &&
           point.x <= bounds.x * 0.5F + hmi::actionHalfWidth &&
           point.y >= bounds.y * 0.5F + hmi::actionTopOffset &&
           point.y <= bounds.y * 0.5F + hmi::actionTopOffset + hmi::actionHeight;
}

void Renderer::ship(sf::RenderTarget &target, const Ship &shipState, Vec2 bounds, bool thrust,
                    float time) const {
    if (shipState.protection > 0.0F && static_cast<int>(time * 10.0F) % 2 != 0) {
        return;
    }
    tiled(shipState.position, bounds, [&](Vec2 position) {
        polyline(target, hmi::playerCraftOutline, position, shipState.angle, hmi::ink);
        polyline(target, hmi::playerCockpit, position, shipState.angle, hmi::accent);
        if (thrust) {
            const float flame = hmi::engineFlameLength +
                                hmi::engineFlamePulse * std::sin(time * hmi::engineFlameFrequency);
            polyline(target,
                     {{hmi::engineRootX, hmi::engineUpperY}, {-flame, hmi::engineUpperTipY},
                      {hmi::engineReturnX, hmi::engineUpperReturnY}},
                     position, shipState.angle, hmi::warning);
            polyline(target,
                     {{hmi::engineRootX, hmi::engineLowerY}, {-flame, hmi::engineLowerTipY},
                      {hmi::engineReturnX, hmi::engineLowerReturnY}},
                     position, shipState.angle, hmi::warning);
        }
    });
}

void Renderer::rock(sf::RenderTarget &target, const Rock &rockState, Vec2 bounds) const {
    std::vector<Vec2> outline;
    for (unsigned index = 0; index < 12; ++index) {
        const std::uint32_t hash = (rockState.shape ^ (index * 2654435761U)) * 2246822519U;
        const float irregularity = 0.72F + static_cast<float>(hash % 29U) * 0.01F;
        const float angle = static_cast<float>(index) * std::numbers::pi_v<float> / 6.0F;
        outline.push_back(direction(angle) * (radius(rockState.size) * irregularity));
    }
    outline.push_back(outline.front());
    tiled(rockState.position, bounds,
          [&](Vec2 position) { polyline(target, outline, position, rockState.angle, hmi::ink); });
}

void Renderer::draw(sf::RenderTarget &target, const Game &game, Screen screen, bool thrust,
                    int bestScore) const {
    const auto &world = game.world();
    const Vec2 bounds = game.bounds();
    rectangle(target, {}, bounds, hmi::background);
    for (unsigned index = 0; index < 150; ++index) {
        const float horizontal =
            static_cast<float>((index * 733U + 91U) % 1200U) / 1200.0F * bounds.x;
        const float vertical = static_cast<float>((index * 397U + 17U) % 800U) / 800.0F * bounds.y;
        const auto brightness = static_cast<std::uint8_t>(35U + index % 45U);
        rectangle(target, {horizontal, vertical}, {1.0F, 1.0F},
                  {brightness, brightness, brightness});
    }
    for (const auto &rockState : world.rocks) {
        rock(target, rockState, bounds);
    }
    for (const auto &bullet : world.bullets) {
        tiled(bullet.position, bounds, [&](Vec2 position) {
            rectangle(target, position - Vec2{1.5F, 1.5F}, {3.0F, 3.0F},
                      bullet.hostile ? hmi::hostileBullet : hmi::accent);
        });
    }
    for (const auto &particle : world.particles) {
        const auto alpha =
            static_cast<std::uint8_t>(std::min(particle.lifetime / 0.8F, 1.0F) * 255.0F);
        sf::Color particleColor = hmi::ink;
        particleColor.a = alpha;
        rectangle(target, particle.position, {2.0F, 2.0F}, particleColor);
    }
    if (world.saucer) {
        tiled(world.saucer->position, bounds, [&](Vec2 position) {
            polyline(target, hmi::enemyMantaHull, position, 0.0F, hmi::warning);
            polyline(target, hmi::enemyMantaCanopy, position, 0.0F, hmi::hostileBullet);
            polyline(target, hmi::enemyMantaEngines, position, 0.0F, hmi::accent);
        });
    }
    if (screen == Screen::game || screen == Screen::pause) {
        ship(target, world.ship, bounds, thrust && screen == Screen::game, world.elapsed);
    }
        rectangle(target, {}, {bounds.x, hmi::bandHeight}, hmi::hudBand);
        textRenderer_.draw(target, "HIGH SCORE", {hmi::edgeInset, hmi::labelY}, hmi::labelScale,
                           hmi::muted);
        textRenderer_.draw(target, std::to_string(bestScore), {hmi::edgeInset, hmi::valueY},
                           hmi::highScoreScale, hmi::ink);
        textRenderer_.draw(target, "SCORE", {bounds.x * 0.5F, hmi::labelY}, hmi::labelScale,
                           hmi::muted, true);
        textRenderer_.draw(target, std::to_string(world.score), {bounds.x * 0.5F, hmi::valueY},
                           hmi::scoreScale, hmi::ink, true);
    const std::string waveLabel = "WAVE " + std::to_string(world.wave);
    const std::string livesLabel = "LIVES " + std::to_string(std::max(world.lives, hmi::noLives));
    const auto rightAlignedX = [this, bounds](std::string_view value, float scale) {
        return bounds.x - hmi::edgeInset - textRenderer_.width(value, scale);
    };
        textRenderer_.draw(target, waveLabel, {rightAlignedX(waveLabel, hmi::labelScale), hmi::labelY},
                       hmi::labelScale, hmi::accent);
        textRenderer_.draw(target, livesLabel, {rightAlignedX(livesLabel, hmi::labelScale),
                                        hmi::livesLabelY},
                       hmi::labelScale, hmi::muted);
    const int visibleLives = std::min(world.lives, hmi::maxLifeIcons);
    for (int life = 0; life < visibleLives; ++life) {
        Ship icon;
        icon.position = {bounds.x - hmi::lifeIconRightInset -
                             static_cast<float>(visibleLives - 1 - life) * hmi::lifeIconSpacing,
                         hmi::lifeIconY};
        icon.protection = hmi::noProtection;
        ship(target, icon, bounds, false, 0.0F);
    }
    if (screen == Screen::game && world.rocks.empty()) {
           textRenderer_.draw(target, "SECTOR CLEAR",
                              {bounds.x * 0.5F, bounds.y * 0.5F - hmi::sectorClearOffset},
                              hmi::sectorClearScale, hmi::accent, true);
    }
    if (screen != Screen::game) {
           rectangle(target, {}, bounds, hmi::overlay);
        const std::string_view heading = screen == Screen::title   ? "PEBBLE BLASTER"
                                         : screen == Screen::pause ? "PAUSED"
                                                                   : "GAME OVER";
        textRenderer_.draw(target, heading,
                   {bounds.x * 0.5F, bounds.y * 0.5F - hmi::menuHeadingOffset},
                   hmi::menuHeadingScale, hmi::ink, true);
        if (screen == Screen::gameOver) {
              textRenderer_.draw(target, "SCORE " + std::to_string(world.score),
                             {bounds.x * 0.5F, bounds.y * 0.5F + hmi::gameOverScoreOffset},
                             hmi::gameOverScoreScale, hmi::accent, true);
        }
        rectangle(target,
                  {bounds.x * 0.5F - hmi::actionHalfWidth,
                   bounds.y * 0.5F + hmi::actionTopOffset},
                  {hmi::actionWidth, hmi::actionHeight}, hmi::accent);
        const std::string_view action = screen == Screen::title   ? "START"
                                        : screen == Screen::pause ? "RESUME"
                                                                  : "PLAY AGAIN";
           textRenderer_.draw(target, action,
                              {bounds.x * 0.5F, bounds.y * 0.5F + hmi::actionTextOffset},
                              hmi::actionTextScale, hmi::background, true);
    }
}

}