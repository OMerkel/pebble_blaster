#include "pebble_blaster/core.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace pebble_blaster {
namespace {

constexpr float saucerWeaveAmplitude = 24.0F;
constexpr float saucerWeaveFrequency = 3.6F;

}

Vec2 Vec2::operator+(Vec2 other) const {
    return {x + other.x, y + other.y};
}
Vec2 Vec2::operator-(Vec2 other) const {
    return {x - other.x, y - other.y};
}
Vec2 Vec2::operator*(float scalar) const {
    return {x * scalar, y * scalar};
}

float lengthSquared(Vec2 vector) {
    return vector.x * vector.x + vector.y * vector.y;
}

Vec2 direction(float radians) {
    return {std::cos(radians), std::sin(radians)};
}

Vec2 wrap(Vec2 position, Vec2 bounds) {
    const auto axis = [](float value, float extent) {
        const float remainder = std::fmod(value, extent);
        return remainder < 0.0F ? remainder + extent : remainder;
    };
    return {axis(position.x, bounds.x), axis(position.y, bounds.y)};
}

bool overlaps(Vec2 first, float firstRadius, Vec2 second, float secondRadius, Vec2 bounds) {
    Vec2 distance = wrap(first - second, bounds);
    distance.x = std::min(distance.x, bounds.x - distance.x);
    distance.y = std::min(distance.y, bounds.y - distance.y);
    const float radius = firstRadius + secondRadius;
    return lengthSquared(distance) <= radius * radius;
}

float radius(RockSize size) {
    switch (size) {
    case RockSize::large:
        return 44.0F;
    case RockSize::medium:
        return 25.0F;
    case RockSize::small:
        return 13.0F;
    }
    throw std::invalid_argument("Invalid rock size");
}

int points(RockSize size) {
    switch (size) {
    case RockSize::large:
        return 20;
    case RockSize::medium:
        return 50;
    case RockSize::small:
        return 100;
    }
    throw std::invalid_argument("Invalid rock size");
}

Game::Game(std::uint32_t seed, Vec2 bounds) : Game(World{}, seed, bounds) {
    restart();
}

Game::Game(World world, std::uint32_t seed, Vec2 bounds)
    : world_(std::move(world)), random_(seed), bounds_(bounds) {
    if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || bounds.x < 400.0F ||
        bounds.y < 300.0F) {
        throw std::invalid_argument("Playfield must be finite and at least 400 by 300");
    }
}

float Game::random(float minimum, float maximum) {
    return std::uniform_real_distribution<float>(minimum, maximum)(random_);
}

void Game::restart() {
    world_ = World{};
    world_.ship.position = bounds_ * 0.5F;
    spawnWave();
}

void Game::togglePause() {
    if (world_.phase != Phase::gameOver) {
        world_.phase = world_.phase == Phase::playing ? Phase::paused : Phase::playing;
    }
}

void Game::spawnWave() {
    const int count = std::min(3 + world_.wave, 12);
    for (int index = 0; index < count; ++index) {
        const float angle = random(0.0F, 2.0F * std::numbers::pi_v<float>);
        const float speed = random(35.0F, 65.0F) + 5.0F * static_cast<float>(world_.wave);
        const Vec2 position = index % 2 == 0 ? Vec2{random(0.0F, bounds_.x), 35.0F}
                                             : Vec2{35.0F, random(0.0F, bounds_.y)};
        world_.rocks.push_back({position, direction(angle) * speed, RockSize::large, angle,
                                random(-0.7F, 0.7F), random_()});
    }
    world_.waveTimer = 2.0F;
}

void Game::updateShip(float seconds, Input input) {
    auto &ship = world_.ship;
    ship.protection = std::max(0.0F, ship.protection - seconds);
    const float turn = std::clamp(input.turn + static_cast<float>(input.right) -
                                      static_cast<float>(input.left),
                                  -1.0F, 1.0F);
    ship.angle += turn * 4.2F * seconds;
    if (input.thrust) {
        ship.velocity = ship.velocity + direction(ship.angle) * (310.0F * seconds);
    }
    const float speedSquared = lengthSquared(ship.velocity);
    if (speedSquared > 420.0F * 420.0F) {
        ship.velocity = ship.velocity * (420.0F / std::sqrt(speedSquared));
    }
    ship.velocity = ship.velocity * std::exp(-0.12F * seconds);
    ship.position = wrap(ship.position + ship.velocity * seconds, bounds_);
    world_.fireTimer = std::max(0.0F, world_.fireTimer - seconds);
    const auto shots = std::count_if(world_.bullets.begin(), world_.bullets.end(),
                                     [](const Bullet &bullet) { return !bullet.hostile; });
    if (input.fire && world_.fireTimer == 0.0F && shots < 6) {
        const Vec2 forward = direction(ship.angle);
        world_.bullets.push_back({wrap(ship.position + forward * 19.0F, bounds_),
                                  ship.velocity + forward * 600.0F, 1.1F, false});
        world_.fireTimer = 0.16F;
    }
}

void Game::updateSaucer(float seconds) {
    world_.saucerTimer -= seconds;
    if (!world_.saucer && world_.saucerTimer <= 0.0F) {
        world_.saucer = Saucer{{0.0F, random(80.0F, bounds_.y - 80.0F)},
                               {110.0F, 18.0F}, 10.0F, 0.8F, 0.0F,
                               random(0.0F, 2.0F * std::numbers::pi_v<float>)};
        world_.saucerTimer = random(18.0F, 26.0F);
    }
    if (!world_.saucer) {
        return;
    }
    auto &saucer = *world_.saucer;
    const float previousOffset =
        saucerWeaveAmplitude *
        std::sin(saucer.oscillationTime * saucerWeaveFrequency + saucer.oscillationPhase);
    saucer.oscillationTime += seconds;
    const float currentOffset =
        saucerWeaveAmplitude *
        std::sin(saucer.oscillationTime * saucerWeaveFrequency + saucer.oscillationPhase);
    saucer.position = wrap(saucer.position + saucer.velocity * seconds +
                               Vec2{0.0F, currentOffset - previousOffset},
                           bounds_);
    saucer.lifetime -= seconds;
    saucer.shotTimer -= seconds;
    if (saucer.lifetime <= 0.0F) {
        world_.saucer.reset();
        return;
    }
    if (saucer.shotTimer <= 0.0F) {
        Vec2 delta = world_.ship.position - saucer.position;
        if (delta.x > bounds_.x * 0.5F) {
            delta.x -= bounds_.x;
        }
        if (delta.x < -bounds_.x * 0.5F) {
            delta.x += bounds_.x;
        }
        if (delta.y > bounds_.y * 0.5F) {
            delta.y -= bounds_.y;
        }
        if (delta.y < -bounds_.y * 0.5F) {
            delta.y += bounds_.y;
        }
        const float angle = std::atan2(delta.y, delta.x) + random(-0.18F, 0.18F);
        world_.bullets.push_back({saucer.position, direction(angle) * 270.0F, 2.4F, true});
        saucer.shotTimer = 1.25F;
    }
}

void Game::update(float seconds, Input input) {
    if (world_.phase != Phase::playing || !std::isfinite(seconds) || seconds <= 0.0F) {
        return;
    }
    seconds = std::min(seconds, 0.05F);
    world_.elapsed += seconds;
    updateShip(seconds, input);
    for (auto &rock : world_.rocks) {
        rock.position = wrap(rock.position + rock.velocity * seconds, bounds_);
        rock.angle += rock.spin * seconds;
    }
    for (auto &bullet : world_.bullets) {
        bullet.position = wrap(bullet.position + bullet.velocity * seconds, bounds_);
        bullet.lifetime -= seconds;
    }
    std::erase_if(world_.bullets, [](const Bullet &bullet) { return bullet.lifetime <= 0.0F; });
    for (auto &particle : world_.particles) {
        particle.position = wrap(particle.position + particle.velocity * seconds, bounds_);
        particle.lifetime -= seconds;
    }
    std::erase_if(world_.particles,
                  [](const Particle &particle) { return particle.lifetime <= 0.0F; });
    updateSaucer(seconds);
    collide();
    if (world_.phase == Phase::playing && world_.rocks.empty()) {
        world_.waveTimer -= seconds;
        if (world_.waveTimer <= 0.0F) {
            ++world_.wave;
            spawnWave();
        }
    }
}

void Game::explode(Vec2 position) {
    for (int index = 0; index < 18; ++index) {
        const float angle = random(0.0F, 2.0F * std::numbers::pi_v<float>);
        world_.particles.push_back(
            {position, direction(angle) * random(30.0F, 180.0F), random(0.25F, 0.8F)});
    }
}

void Game::award(int value) {
    world_.score += value;
    while (world_.score >= world_.nextExtraLife) {
        ++world_.lives;
        world_.nextExtraLife += 10000;
    }
}

void Game::destroyRock(std::size_t index) {
    const Rock rock = world_.rocks[index];
    world_.rocks.erase(world_.rocks.begin() + static_cast<std::ptrdiff_t>(index));
    award(points(rock.size));
    explode(rock.position);
    if (rock.size == RockSize::small) {
        return;
    }
    const RockSize child = rock.size == RockSize::large ? RockSize::medium : RockSize::small;
    for (int fragment = 0; fragment < 2; ++fragment) {
        const float angle = rock.angle + (fragment == 0 ? -0.8F : 0.8F);
        const float speed = std::sqrt(lengthSquared(rock.velocity)) + 45.0F;
        world_.rocks.push_back({rock.position, direction(angle) * speed, child, angle,
                                random(-1.8F, 1.8F), random_()});
    }
}

void Game::hitShip() {
    explode(world_.ship.position);
    --world_.lives;
    if (world_.lives <= 0) {
        world_.phase = Phase::gameOver;
    } else {
        world_.ship = Ship{};
        world_.ship.position = bounds_ * 0.5F;
    }
}

void Game::collide() {
    for (auto &bullet : world_.bullets) {
        if (bullet.hostile) {
            if (world_.ship.protection <= 0.0F &&
                overlaps(bullet.position, 2.0F, world_.ship.position, 11.0F, bounds_)) {
                bullet.lifetime = 0.0F;
                hitShip();
                if (world_.phase == Phase::gameOver) {
                    break;
                }
            }
            continue;
        }
        if (world_.saucer &&
            overlaps(bullet.position, 2.0F, world_.saucer->position, 22.0F, bounds_)) {
            explode(world_.saucer->position);
            world_.saucer.reset();
            award(200);
            bullet.lifetime = 0.0F;
            continue;
        }
        for (std::size_t index = 0; index < world_.rocks.size(); ++index) {
            if (overlaps(bullet.position, 2.0F, world_.rocks[index].position,
                         radius(world_.rocks[index].size), bounds_)) {
                bullet.lifetime = 0.0F;
                destroyRock(index);
                break;
            }
        }
    }
    std::erase_if(world_.bullets, [](const Bullet &bullet) { return bullet.lifetime <= 0.0F; });
    if (world_.ship.protection > 0.0F || world_.phase == Phase::gameOver) {
        return;
    }
    if (world_.saucer &&
        overlaps(world_.ship.position, 11.0F, world_.saucer->position, 22.0F, bounds_)) {
        hitShip();
        return;
    }
    for (const auto &rock : world_.rocks) {
        if (overlaps(world_.ship.position, 11.0F, rock.position, radius(rock.size), bounds_)) {
            hitShip();
            return;
        }
    }
}

}