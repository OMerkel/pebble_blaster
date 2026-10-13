/**
 * @file core_test.cpp
 * @brief Tests game core behavior and input mapping.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */
#include "pebble_blaster/core.hpp"
#include "pebble_blaster/input_mapping.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <numbers>

namespace pebble_blaster {
namespace {

// Requirements: FR-GEO-001
TEST(Geometry, VectorArithmetic) {
    const Vec2 vector{3.0F, 4.0F};
    EXPECT_FLOAT_EQ(lengthSquared(vector), 25.0F);
    EXPECT_FLOAT_EQ((vector + Vec2{2.0F, 1.0F}).x, 5.0F);
    EXPECT_FLOAT_EQ((vector - Vec2{2.0F, 1.0F}).y, 3.0F);
    EXPECT_FLOAT_EQ((vector * 2.0F).y, 8.0F);
    EXPECT_NEAR(direction(std::numbers::pi_v<float> / 2.0F).y, 1.0F, 0.0001F);
}

// Requirements: FR-GEO-002
TEST(Geometry, WrapsNegativePositiveAndMultipleScreenCrossings) {
    const Vec2 bounds{100.0F, 80.0F};
    EXPECT_FLOAT_EQ(wrap({-1.0F, 81.0F}, bounds).x, 99.0F);
    EXPECT_FLOAT_EQ(wrap({-1.0F, 81.0F}, bounds).y, 1.0F);
    EXPECT_FLOAT_EQ(wrap({300.0F, -161.0F}, bounds).x, 0.0F);
    EXPECT_FLOAT_EQ(wrap({300.0F, -161.0F}, bounds).y, 79.0F);
}

// Requirements: FR-GEO-003
TEST(Geometry, CollidesAcrossEdgesAndIncludesTangency) {
    const Vec2 bounds{100.0F, 80.0F};
    EXPECT_TRUE(overlaps({1.0F, 40.0F}, 2.0F, {99.0F, 40.0F}, 2.0F, bounds));
    EXPECT_TRUE(overlaps({50.0F, 1.0F}, 2.0F, {50.0F, 79.0F}, 2.0F, bounds));
    EXPECT_TRUE(overlaps({10.0F, 10.0F}, 2.0F, {14.0F, 10.0F}, 2.0F, bounds));
    EXPECT_FALSE(overlaps({10.0F, 10.0F}, 2.0F, {14.1F, 10.0F}, 2.0F, bounds));
}

// Requirements: FR-INPUT-001, NFR-PURE-001
TEST(InputMapping, MapsStickDeadZoneAndDpadPriority) {
    const JoystickInputSample neutral{.x = 19.0F, .y = -19.0F};
    const Input neutralInput = mapJoystickInput(neutral);
    EXPECT_FLOAT_EQ(neutralInput.turn, 0.0F);
    EXPECT_FALSE(neutralInput.thrust);
    EXPECT_FALSE(neutralInput.fire);

    const JoystickInputSample stick{.x = 60.0F, .y = -50.0F, .povX = 49.0F, .button0 = true};
    const Input stickInput = mapJoystickInput(stick);
    EXPECT_FLOAT_EQ(stickInput.turn, 0.5F);
    EXPECT_FALSE(stickInput.thrust);
    EXPECT_TRUE(stickInput.fire);

    const JoystickInputSample dpad{.x = 60.0F,
                                   .y = -80.0F,
                                   .povX = -50.0F,
                                   .povY = -51.0F,
                                   .button1 = true};
    const Input dpadInput = mapJoystickInput(dpad);
    EXPECT_FLOAT_EQ(dpadInput.turn, -1.0F);
    EXPECT_TRUE(dpadInput.thrust);
    EXPECT_TRUE(dpadInput.fire);

    const JoystickInputSample leftStick{.x = -60.0F};
    EXPECT_FLOAT_EQ(mapJoystickInput(leftStick).turn, -0.5F);

    const JoystickInputSample povThrust{.povY = -51.0F};
    EXPECT_TRUE(mapJoystickInput(povThrust).thrust);
    EXPECT_FLOAT_EQ(stick.x, 60.0F);
}

// Requirements: FR-INPUT-002, NFR-PURE-001
TEST(InputMapping, CombinesSourcesWithoutMutatingThem) {
    const Input keyboard{.left = true, .fire = true};
    const Input joystick{.right = true, .turn = 0.4F, .thrust = true, .fire = true};

    const Input combined = combineInputs(keyboard, joystick);

    EXPECT_TRUE(combined.left);
    EXPECT_TRUE(combined.right);
    EXPECT_FLOAT_EQ(combined.turn, 0.4F);
    EXPECT_TRUE(combined.thrust);
    EXPECT_TRUE(combined.fire);
    EXPECT_TRUE(keyboard.left);
    EXPECT_FALSE(keyboard.right);
    EXPECT_FLOAT_EQ(keyboard.turn, 0.0F);
    EXPECT_TRUE(joystick.right);
    EXPECT_FLOAT_EQ(joystick.turn, 0.4F);

    const Input joystickOnly{.left = true, .fire = true};
    const Input joystickOnlyCombined = combineInputs({}, joystickOnly);
    EXPECT_TRUE(joystickOnlyCombined.left);
    EXPECT_TRUE(joystickOnlyCombined.fire);

    const Input keyboardOnly{.right = true, .thrust = true, .fire = true};
    const Input keyboardOnlyCombined = combineInputs(keyboardOnly, {});
    EXPECT_TRUE(keyboardOnlyCombined.right);
    EXPECT_TRUE(keyboardOnlyCombined.thrust);
    EXPECT_TRUE(keyboardOnlyCombined.fire);

    const Input neutralCombined = combineInputs({}, {});
    EXPECT_FALSE(neutralCombined.left);
    EXPECT_FALSE(neutralCombined.right);
    EXPECT_FALSE(neutralCombined.thrust);
    EXPECT_FALSE(neutralCombined.fire);
}

World scenario() {
    World world;
    world.ship.position = {600.0F, 400.0F};
    world.rocks.push_back({{200.0F, 200.0F}, {}, RockSize::small});
    return world;
}

void advance(Game &game, int steps, Input input = {}) {
    for (int step = 0; step < steps; ++step) {
        game.update(0.01F, input);
    }
}

// Requirements: FR-CORE-001
TEST(Game, StartsWithThreeLivesAndFourSafeRocks) {
    Game game(42);
    EXPECT_EQ(game.world().lives, 3);
    EXPECT_EQ(game.world().wave, 1);
    EXPECT_EQ(game.world().score, 0);
    EXPECT_EQ(game.world().rocks.size(), 4U);
    EXPECT_FLOAT_EQ(game.bounds().x, 1200.0F);
    for (const auto &rock : game.world().rocks) {
        EXPECT_FALSE(overlaps(rock.position, radius(rock.size), game.world().ship.position, 180.0F,
                              game.bounds()));
    }
}

// Requirements: FR-CORE-002
TEST(Game, RejectsInvalidPlayfields) {
    EXPECT_THROW(Game(0, {399.0F, 800.0F}), std::invalid_argument);
    EXPECT_THROW(Game(0, {800.0F, 299.0F}), std::invalid_argument);
    EXPECT_THROW(Game(0, {std::numeric_limits<float>::infinity(), 800.0F}), std::invalid_argument);
    EXPECT_THROW(Game(0, {800.0F, std::numeric_limits<float>::quiet_NaN()}), std::invalid_argument);
}

// Requirements: FR-CORE-003
TEST(Game, SeedReproducesWave) {
    Game first(123);
    Game second(123);
    EXPECT_FLOAT_EQ(first.world().rocks.front().velocity.x,
                    second.world().rocks.front().velocity.x);
    EXPECT_EQ(first.world().rocks.front().shape, second.world().rocks.front().shape);
}

// Requirements: FR-CORE-004
TEST(Game, PausingFreezesSimulationAndResumes) {
    Game game(1);
    game.togglePause();
    EXPECT_EQ(game.world().phase, Phase::paused);
    game.update(0.05F, {.thrust = true, .fire = true});
    EXPECT_FLOAT_EQ(game.world().elapsed, 0.0F);
    EXPECT_TRUE(game.world().bullets.empty());
    game.togglePause();
    game.update(0.01F);
    EXPECT_EQ(game.world().phase, Phase::playing);
    EXPECT_GT(game.world().elapsed, 0.0F);
}

// Requirements: FR-CORE-005
TEST(Game, IgnoresInvalidTimeAndCapsLargeSteps) {
    Game game(1);
    game.update(0.0F);
    game.update(-1.0F);
    game.update(std::numeric_limits<float>::quiet_NaN());
    game.update(std::numeric_limits<float>::infinity());
    EXPECT_FLOAT_EQ(game.world().elapsed, 0.0F);
    game.update(2.0F);
    EXPECT_FLOAT_EQ(game.world().elapsed, 0.05F);
}

// Requirements: FR-CORE-006
TEST(Ship, TurnsBothWaysAndOpposedControlsCancel) {
    Game game(scenario(), 1);
    const float original = game.world().ship.angle;
    game.update(0.05F, {.left = true});
    EXPECT_LT(game.world().ship.angle, original);
    game.update(0.05F, {.right = true});
    EXPECT_NEAR(game.world().ship.angle, original, 0.00001F);
    game.update(0.05F, {.left = true, .right = true});
    EXPECT_NEAR(game.world().ship.angle, original, 0.00001F);
}

// Requirements: FR-CORE-006
TEST(Ship, SupportsAnalogTurningAndClampsInput) {
    Game game(scenario(), 1);
    const float original = game.world().ship.angle;
    game.update(0.05F, {.turn = 0.5F});
    EXPECT_GT(game.world().ship.angle, original);
    const float partialTurn = game.world().ship.angle;
    game.update(0.05F, {.turn = 4.0F});
    EXPECT_NEAR(game.world().ship.angle - partialTurn, 4.2F * 0.05F, 0.00001F);
}

// Requirements: FR-CORE-007
TEST(Ship, AcceleratesCoastsAndCapsSpeed) {
    Game game(scenario(), 1);
    advance(game, 500, {.thrust = true});
    EXPECT_LE(lengthSquared(game.world().ship.velocity), 420.0F * 420.0F);
    EXPECT_GT(lengthSquared(game.world().ship.velocity), 400.0F * 400.0F);
    const float speed = lengthSquared(game.world().ship.velocity);
    game.update(0.05F);
    EXPECT_LT(lengthSquared(game.world().ship.velocity), speed);
    EXPECT_GT(lengthSquared(game.world().ship.velocity), 0.0F);
}

// Requirements: FR-CORE-008
TEST(Ship, WrapsAtThePlayfieldBoundary) {
    World world = scenario();
    world.ship.position = {1199.0F, 799.0F};
    world.ship.velocity = {100.0F, 100.0F};
    Game game(world, 1);
    game.update(0.05F);
    EXPECT_LT(game.world().ship.position.x, 5.0F);
    EXPECT_LT(game.world().ship.position.y, 5.0F);
}

// Requirements: FR-CORE-009
TEST(Weapons, FiresForwardInheritsVelocityAndHonorsCooldown) {
    World world = scenario();
    world.ship.angle = 0.0F;
    world.ship.velocity = {100.0F, 0.0F};
    Game game(world, 1);
    game.update(0.01F, {.fire = true});
    ASSERT_EQ(game.world().bullets.size(), 1U);
    EXPECT_GT(game.world().bullets.front().velocity.x, 690.0F);
    EXPECT_GT(game.world().bullets.front().position.x, game.world().ship.position.x);
    game.update(0.01F, {.fire = true});
    EXPECT_EQ(game.world().bullets.size(), 1U);
    advance(game, 16, {.fire = true});
    EXPECT_EQ(game.world().bullets.size(), 2U);
}

// Requirements: FR-CORE-010
TEST(Weapons, LimitsPlayerShotsButNotHostileShots) {
    World world = scenario();
    for (int index = 0; index < 6; ++index) {
        world.bullets.push_back({{900.0F, 600.0F}, {}, 1.0F, false});
    }
    Game game(world, 1);
    game.update(0.01F, {.fire = true});
    EXPECT_EQ(game.world().bullets.size(), 6U);
    world.bullets.front().hostile = true;
    Game other(world, 1);
    other.update(0.01F, {.fire = true});
    EXPECT_EQ(other.world().bullets.size(), 7U);
}

// Requirements: FR-CORE-011
TEST(Weapons, BulletsWrapAndExpire) {
    World world = scenario();
    world.bullets = {{{1199.0F, 799.0F}, {100.0F, 100.0F}, 0.08F, false}};
    Game game(world, 1);
    game.update(0.05F);
    ASSERT_EQ(game.world().bullets.size(), 1U);
    EXPECT_FLOAT_EQ(game.world().bullets.front().position.x, 4.0F);
    game.update(0.05F);
    EXPECT_TRUE(game.world().bullets.empty());
}

class RockDestruction : public testing::TestWithParam<RockSize> {};

// Requirements: FR-CORE-012
TEST_P(RockDestruction, AwardsPointsAndSplitsIntoCorrectSize) {
    World world = scenario();
    world.rocks.front().size = GetParam();
    world.bullets = {{{200.0F, 200.0F}, {}, 1.0F, false}};
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().score, points(GetParam()));
    EXPECT_TRUE(game.world().bullets.empty());
    EXPECT_EQ(game.world().particles.size(), 18U);
    if (GetParam() == RockSize::small) {
        EXPECT_TRUE(game.world().rocks.empty());
    } else {
        ASSERT_EQ(game.world().rocks.size(), 2U);
        const auto expected = GetParam() == RockSize::large ? RockSize::medium : RockSize::small;
        EXPECT_EQ(game.world().rocks.front().size, expected);
        EXPECT_EQ(game.world().rocks.back().size, expected);
        EXPECT_GT(lengthSquared(game.world().rocks.front().velocity), 0.0F);
    }
}

INSTANTIATE_TEST_SUITE_P(AllSizes, RockDestruction,
                         testing::Values(RockSize::large, RockSize::medium, RockSize::small));

// Requirements: FR-CORE-028
TEST(Rocks, MoveRotateAndWrap) {
    World world = scenario();
    world.rocks = {{{1199.0F, 799.0F}, {100.0F, 100.0F}, RockSize::large, 1.0F, 2.0F}};
    Game game(world, 1);
    game.update(0.05F);
    EXPECT_FLOAT_EQ(game.world().rocks.front().position.x, 4.0F);
    EXPECT_FLOAT_EQ(game.world().rocks.front().angle, 1.1F);
}

// Requirements: FR-CORE-012
TEST(Rocks, OneBulletDestroysOnlyOneRock) {
    World world = scenario();
    world.rocks.push_back(world.rocks.front());
    world.bullets = {{{200.0F, 200.0F}, {}, 1.0F, false}};
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().rocks.size(), 1U);
    EXPECT_EQ(game.world().score, 100);
}

// Requirements: FR-CORE-013
TEST(Rocks, InvalidSizeIsRejected) {
    EXPECT_THROW(static_cast<void>(radius(static_cast<RockSize>(99))), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(points(static_cast<RockSize>(99))), std::invalid_argument);
}

// Requirements: FR-CORE-014
TEST(Lives, ProtectionPreventsCollisionsThenOneHitRespawns) {
    World world = scenario();
    world.rocks.front().position = world.ship.position;
    world.rocks.push_back(world.rocks.front());
    world.ship.velocity = {100.0F, 0.0F};
    world.ship.protection = 0.015F;
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 3);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 2);
    EXPECT_FLOAT_EQ(game.world().ship.position.x, 600.0F);
    EXPECT_FLOAT_EQ(game.world().ship.velocity.x, 0.0F);
    EXPECT_GT(game.world().ship.protection, 0.0F);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 2);
}

// Requirements: FR-CORE-015
TEST(Lives, LastHitEndsGamePauseCannotReviveAndRestartResets) {
    World world = scenario();
    world.lives = 1;
    world.ship.protection = 0.0F;
    world.rocks.front().position = world.ship.position;
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 0);
    EXPECT_EQ(game.world().phase, Phase::gameOver);
    game.togglePause();
    game.update(0.05F);
    EXPECT_FLOAT_EQ(game.world().elapsed, 0.01F);
    game.restart();
    EXPECT_EQ(game.world().phase, Phase::playing);
    EXPECT_EQ(game.world().lives, 3);
    EXPECT_EQ(game.world().score, 0);
    EXPECT_EQ(game.world().wave, 1);
    EXPECT_TRUE(game.world().bullets.empty());
    EXPECT_TRUE(game.world().particles.empty());
}

// Requirements: FR-CORE-016
TEST(Lives, AwardsExtraLifeOncePerTenThousandPoints) {
    World world = scenario();
    world.score = 9900;
    world.bullets = {{{200.0F, 200.0F}, {}, 1.0F, false}};
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().score, 10000);
    EXPECT_EQ(game.world().lives, 4);
    EXPECT_EQ(game.world().nextExtraLife, 20000);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 4);
}

// Requirements: FR-CORE-017
TEST(Waves, EmptyFieldWaitsThenSpawnsNextWave) {
    World world = scenario();
    world.rocks.clear();
    world.waveTimer = 0.02F;
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_TRUE(game.world().rocks.empty());
    game.update(0.02F);
    EXPECT_EQ(game.world().wave, 2);
    EXPECT_EQ(game.world().rocks.size(), 5U);
}

// Requirements: FR-CORE-017
TEST(Waves, DifficultyCapsRockCount) {
    World world = scenario();
    world.rocks.clear();
    world.wave = 30;
    world.waveTimer = 0.0F;
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().rocks.size(), 12U);
}

// Requirements: FR-CORE-018
TEST(Particles, MoveWrapAndExpire) {
    World world = scenario();
    world.particles = {{{1199.0F, 799.0F}, {100.0F, 100.0F}, 0.08F}};
    Game game(world, 1);
    game.update(0.05F);
    EXPECT_FLOAT_EQ(game.world().particles.front().position.x, 4.0F);
    game.update(0.05F);
    EXPECT_TRUE(game.world().particles.empty());
}

// Requirements: FR-CORE-019
TEST(Saucers, TimerSpawnsSaucerAndItFiresAndExpires) {
    World world = scenario();
    world.saucerTimer = 0.0F;
    Game game(world, 1);
    game.update(0.01F);
    ASSERT_TRUE(game.world().saucer);
    EXPECT_GT(game.world().saucer->position.x, 0.0F);
    advance(game, 85);
    ASSERT_FALSE(game.world().bullets.empty());
    EXPECT_TRUE(game.world().bullets.front().hostile);
    advance(game, 950);
    EXPECT_FALSE(game.world().saucer);
}

// Requirements: FR-CORE-019
TEST(Saucers, WeavesSinusoidallyAroundItsBaseline) {
    World world = scenario();
    world.saucer = Saucer{{300.0F, 300.0F}, {}, 10.0F, 10.0F};
    Game game(world, 1);
    float highestDisplacement = 0.0F;
    float lowestDisplacement = 0.0F;

    for (int step = 0; step < 200; ++step) {
        game.update(0.01F);
        ASSERT_TRUE(game.world().saucer);
        const float displacement = game.world().saucer->position.y - 300.0F;
        if (displacement > highestDisplacement) {
            highestDisplacement = displacement;
        }
        if (displacement < lowestDisplacement) {
            lowestDisplacement = displacement;
        }
    }

    EXPECT_NEAR(highestDisplacement, 24.0F, 0.1F);
    EXPECT_NEAR(lowestDisplacement, -24.0F, 0.1F);
}

// Requirements: FR-CORE-020
TEST(Saucers, PlayerShotDestroysSaucerAndAwardsPoints) {
    World world = scenario();
    world.saucer = Saucer{{300.0F, 300.0F}, {}};
    world.bullets = {{{300.0F, 300.0F}, {}, 1.0F, false}};
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_FALSE(game.world().saucer);
    EXPECT_EQ(game.world().score, 200);
    EXPECT_TRUE(game.world().bullets.empty());
}

// Requirements: FR-CORE-020
TEST(Saucers, ContactDamagesShip) {
    World world = scenario();
    world.ship.protection = 0.0F;
    world.saucer = Saucer{world.ship.position, {}};
    Game game(world, 1);
    game.update(0.01F);
    EXPECT_EQ(game.world().lives, 2);
}

// Requirements: FR-CORE-020
TEST(Saucers, HostileShotsDamageShipButNotRocksAndRespectProtection) {
    World world = scenario();
    world.bullets = {{world.ship.position, {}, 1.0F, true},
                     {world.rocks.front().position, {}, 1.0F, true}};
    Game protectedGame(world, 1);
    protectedGame.update(0.01F);
    EXPECT_EQ(protectedGame.world().lives, 3);
    EXPECT_EQ(protectedGame.world().rocks.size(), 1U);
    world.ship.protection = 0.0F;
    Game vulnerable(world, 1);
    vulnerable.update(0.01F);
    EXPECT_EQ(vulnerable.world().lives, 2);
    EXPECT_EQ(vulnerable.world().bullets.size(), 1U);
    world.lives = 1;
    Game lastLife(world, 1);
    lastLife.update(0.01F);
    EXPECT_EQ(lastLife.world().phase, Phase::gameOver);
}

// Requirements: FR-CORE-021
TEST(Saucers, AimUsesShortestWrappedDistanceOnBothAxes) {
    for (const Vec2 position : {Vec2{1190.0F, 790.0F}, Vec2{10.0F, 10.0F}}) {
        World world = scenario();
        world.ship.position = position;
        world.saucer = Saucer{{1200.0F - position.x, 800.0F - position.y}, {}, 10.0F, 0.0F};
        Game game(world, 2);
        game.update(0.01F);
        ASSERT_EQ(game.world().bullets.size(), 1U);
        const auto velocity = game.world().bullets.front().velocity;
        if (position.x > 600.0F) {
            EXPECT_LT(velocity.x, 0.0F);
            EXPECT_LT(velocity.y, 0.0F);
        } else {
            EXPECT_GT(velocity.x, 0.0F);
            EXPECT_GT(velocity.y, 0.0F);
        }
    }
}

}
}