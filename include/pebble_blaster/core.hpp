#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace pebble_blaster {

struct Vec2 {
    float x{};
    float y{};

    [[nodiscard]] Vec2 operator+(Vec2 other) const;
    [[nodiscard]] Vec2 operator-(Vec2 other) const;
    [[nodiscard]] Vec2 operator*(float scalar) const;
};

[[nodiscard]] float lengthSquared(Vec2 vector);
[[nodiscard]] Vec2 direction(float radians);
[[nodiscard]] Vec2 wrap(Vec2 position, Vec2 bounds);
[[nodiscard]] bool overlaps(Vec2 first, float firstRadius, Vec2 second, float secondRadius,
                            Vec2 bounds);

enum class Phase { playing, paused, gameOver };
enum class RockSize { small, medium, large };

struct Input {
    bool left{};
    bool right{};
    bool thrust{};
    bool fire{};
};

struct Ship {
    Vec2 position{};
    Vec2 velocity{};
    float angle{-1.570796327F};
    float protection{3.0F};
};

struct Rock {
    Vec2 position{};
    Vec2 velocity{};
    RockSize size{RockSize::large};
    float angle{};
    float spin{};
    std::uint32_t shape{};
};

struct Bullet {
    Vec2 position{};
    Vec2 velocity{};
    float lifetime{1.1F};
    bool hostile{};
};

struct Particle {
    Vec2 position{};
    Vec2 velocity{};
    float lifetime{};
};

struct Saucer {
    Vec2 position{};
    Vec2 velocity{};
    float lifetime{10.0F};
    float shotTimer{0.8F};
    float oscillationTime{};
    float oscillationPhase{};
};

struct World {
    Ship ship{};
    std::vector<Rock> rocks{};
    std::vector<Bullet> bullets{};
    std::vector<Particle> particles{};
    std::optional<Saucer> saucer{};
    Phase phase{Phase::playing};
    int score{};
    int lives{3};
    int wave{1};
    int nextExtraLife{10000};
    float fireTimer{};
    float waveTimer{2.0F};
    float saucerTimer{18.0F};
    float elapsed{};
};

[[nodiscard]] float radius(RockSize size);
[[nodiscard]] int points(RockSize size);

class Game {
  public:
    explicit Game(std::uint32_t seed = 0, Vec2 bounds = {1200.0F, 800.0F});
    Game(World world, std::uint32_t seed, Vec2 bounds = {1200.0F, 800.0F});

    void restart();
    void togglePause();
    void update(float seconds, Input input = {});

    [[nodiscard]] const World &world() const { return world_; }
    [[nodiscard]] Vec2 bounds() const { return bounds_; }

  private:
    World world_{};
    std::mt19937 random_;
    Vec2 bounds_;

    [[nodiscard]] float random(float minimum, float maximum);
    void spawnWave();
    void updateShip(float seconds, Input input);
    void updateSaucer(float seconds);
    void collide();
    void destroyRock(std::size_t index);
    void hitShip();
    void award(int value);
    void explode(Vec2 position);
};

}