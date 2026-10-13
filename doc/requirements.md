# Requirements and Traceability

This document defines functional (FR) and non-functional (NFR) requirements for
the current modules in `BUILD.bazel`. Requirement IDs are stable: keep an ID when
wording changes without changing intent; assign a new ID when intent changes.

Each scenario uses Gherkin-style `GIVEN`, `WHEN`, and `THEN` steps. The
verification line names the automated test(s), smoke entry point, or manual
check associated with the requirement. Corresponding automated test declarations
carry the same requirement ID in a `// Requirements:` comment.

## Coverage KPI

Calculate requirement test coverage as:

`100 * requirement IDs with at least one passing linked automated test / all requirement IDs`

Count each requirement once, even when multiple tests verify it. Requirements
marked `Not automated` remain in the denominator and therefore expose coverage
gaps. Track manual verification separately; it does not count as automated test
coverage. Recalculate after changing tests or requirements.

## Functional Requirements

### Core Simulation

Feature: Core world construction and deterministic state.

Scenario: FR-CORE-001 New games start in a safe initial state
GIVEN a new game is constructed with valid default bounds
WHEN the game initializes
THEN it shall have three lives, score zero, wave one, a centered ship, and four safe large rocks.
Verification: [Game.StartsWithThreeLivesAndFourSafeRocks](../tests/core_test.cpp)

Scenario: FR-CORE-002 Invalid playfields are rejected
GIVEN bounds that are too small, infinite, or not finite
WHEN a game is constructed
THEN construction shall throw `std::invalid_argument`.
Verification: [Game.RejectsInvalidPlayfields](../tests/core_test.cpp)

Scenario: FR-CORE-003 A seed reproduces the initial wave
GIVEN two games constructed with the same seed and bounds
WHEN their initial waves are generated
THEN corresponding initial rock velocities and shapes shall match.
Verification: [Game.SeedReproducesWave](../tests/core_test.cpp)

Scenario: FR-CORE-004 Pausing freezes simulation and game-over cannot be paused back into play
GIVEN a playing, paused, or game-over world
WHEN pause is toggled or a simulation tick is attempted
THEN paused/game-over worlds shall not advance, and toggling pause shall not leave game-over.
Verification: [Game.PausingFreezesSimulationAndResumes](../tests/core_test.cpp), [Lives.LastHitEndsGamePauseCannotReviveAndRestartResets](../tests/core_test.cpp)

Scenario: FR-CORE-005 Invalid time steps are ignored and long steps are capped
GIVEN a playing game
WHEN `Game::update` receives non-finite, non-positive, or greater-than-50-ms time
THEN invalid time shall not advance the world and accepted time shall be capped at 50 ms.
Verification: [Game.IgnoresInvalidTimeAndCapsLargeSteps](../tests/core_test.cpp)

Feature: Core geometry and movement.

Scenario: FR-GEO-001 Vector operations and direction are calculated
GIVEN two vectors and an angle
WHEN arithmetic, squared length, or direction is requested
THEN the returned values shall match the corresponding vector operations.
Verification: [Geometry.VectorArithmetic](../tests/core_test.cpp)

Scenario: FR-GEO-002 Positions wrap in both directions and across multiple bounds
GIVEN positions outside either playfield edge
WHEN `wrap` is applied
THEN each coordinate shall be mapped into its corresponding bound interval.
Verification: [Geometry.WrapsNegativePositiveAndMultipleScreenCrossings](../tests/core_test.cpp)

Scenario: FR-GEO-003 Collisions use wrapped distance and include tangency
GIVEN circles near opposite playfield edges or exactly at combined-radius distance
WHEN overlap is tested
THEN wrapped neighbors and tangent circles shall overlap, while separated circles shall not.
Verification: [Geometry.CollidesAcrossEdgesAndIncludesTangency](../tests/core_test.cpp)

Scenario: FR-CORE-006 Keyboard and analog turn inputs turn in both directions
GIVEN left/right digital input or signed analog turn input
WHEN the ship is updated
THEN the ship shall turn in the requested direction, opposing digital inputs shall cancel, and analog input shall be clamped to `[-1, 1]`.
Verification: [Ship.TurnsBothWaysAndOpposedControlsCancel](../tests/core_test.cpp), [Ship.SupportsAnalogTurningAndClampsInput](../tests/core_test.cpp)

Scenario: FR-CORE-007 Thrust accelerates, drag slows, and ship speed is capped
GIVEN a ship receiving thrust over repeated updates
WHEN thrust is released
THEN velocity shall accelerate, remain within the speed limit, and decay under drag.
Verification: [Ship.AcceleratesCoastsAndCapsSpeed](../tests/core_test.cpp)

Scenario: FR-CORE-008 Ships wrap across playfield boundaries
GIVEN a moving ship at a playfield edge
WHEN an update moves it beyond the edge
THEN its position shall wrap to the opposite edge.
Verification: [Ship.WrapsAtThePlayfieldBoundary](../tests/core_test.cpp)

Feature: Weapons and destruction.

Scenario: FR-CORE-009 Player fire respects direction, inherited velocity, and cooldown
GIVEN a ship with a known angle and velocity
WHEN fire is held across updates
THEN a shot shall travel forward with inherited ship velocity and firing shall obey cooldown.
Verification: [Weapons.FiresForwardInheritsVelocityAndHonorsCooldown](../tests/core_test.cpp)

Scenario: FR-CORE-010 Player shots are limited independently of hostile shots
GIVEN six active player shots or a mixture containing hostile shots
WHEN another player shot is requested
THEN the six-player-shot limit shall apply only to non-hostile shots.
Verification: [Weapons.LimitsPlayerShotsButNotHostileShots](../tests/core_test.cpp)

Scenario: FR-CORE-011 Bullets wrap and expire
GIVEN a bullet near an edge with a finite lifetime
WHEN it is updated past the edge and lifetime
THEN it shall wrap and later be removed when expired.
Verification: [Weapons.BulletsWrapAndExpire](../tests/core_test.cpp)

Scenario: FR-CORE-012 Destroyed rocks award size-based points and split correctly
GIVEN a colliding player bullet and a large, medium, or small rock
WHEN the rock is destroyed
THEN points and particles shall be awarded, large/medium rocks shall split into two smaller rocks, and small rocks shall not split.
Verification: [AllSizes/RockDestruction.AwardsPointsAndSplitsIntoCorrectSize](../tests/core_test.cpp), [Rocks.OneBulletDestroysOnlyOneRock](../tests/core_test.cpp)

Scenario: FR-CORE-013 Invalid rock sizes are rejected by size helpers
GIVEN a value outside the `RockSize` enumeration
WHEN radius or points are requested
THEN the helper shall throw `std::invalid_argument`.
Verification: [Rocks.InvalidSizeIsRejected](../tests/core_test.cpp)

Scenario: FR-CORE-028 Rocks translate, rotate, and wrap
GIVEN a rock with velocity and spin near the playfield boundary
WHEN the simulation advances
THEN the rock shall translate, rotate, and wrap into the playfield.
Verification: [Rocks.MoveRotateAndWrap](../tests/core_test.cpp)

Feature: Lives, waves, particles, and saucers.

Scenario: FR-CORE-014 Ship protection prevents collisions and a hit respawns once
GIVEN a protected ship overlapping a rock and later an unprotected ship
WHEN collision updates occur
THEN protection shall prevent damage, and a valid hit shall remove one life and respawn with protection.
Verification: [Lives.ProtectionPreventsCollisionsThenOneHitRespawns](../tests/core_test.cpp)

Scenario: FR-CORE-015 Final life loss enters game-over and restart resets the world
GIVEN a ship with one life remaining that collides
WHEN the collision is resolved and the game is restarted
THEN phase shall become game-over, pause shall not revive it, and restart shall restore initial state.
Verification: [Lives.LastHitEndsGamePauseCannotReviveAndRestartResets](../tests/core_test.cpp)

Scenario: FR-CORE-016 Crossing an extra-life threshold grants a life once
GIVEN a score immediately below an extra-life threshold
WHEN points carry the score across that threshold
THEN one life shall be awarded and the next threshold advanced.
Verification: [Lives.AwardsExtraLifeOncePerTenThousandPoints](../tests/core_test.cpp)

Scenario: FR-CORE-017 Empty waves wait before spawning and rock count is capped
GIVEN an empty field with a wave timer or a high wave number
WHEN the timer expires
THEN the next wave shall spawn after the delay and contain no more than twelve rocks.
Verification: [Waves.EmptyFieldWaitsThenSpawnsNextWave](../tests/core_test.cpp), [Waves.DifficultyCapsRockCount](../tests/core_test.cpp)

Scenario: FR-CORE-018 Particles move, wrap, and expire
GIVEN a particle near a boundary with a short lifetime
WHEN it is updated
THEN it shall wrap and be removed after expiration.
Verification: [Particles.MoveWrapAndExpire](../tests/core_test.cpp)

Scenario: FR-CORE-019 Saucers spawn on schedule, weave, fire, and expire
GIVEN a due saucer timer or an active saucer
WHEN the simulation advances
THEN a saucer shall spawn, weave within its amplitude, fire hostile shots on schedule, and expire at its lifetime.
Verification: [Saucers.TimerSpawnsSaucerAndItFiresAndExpires](../tests/core_test.cpp), [Saucers.WeavesSinusoidallyAroundItsBaseline](../tests/core_test.cpp)

Scenario: FR-CORE-020 Player interaction with saucers resolves damage and rewards
GIVEN a player bullet, ship, or hostile bullet overlapping a saucer/ship
WHEN collision handling runs
THEN saucers shall be destroyed for points, ship contact/hostile fire shall affect lives, and protection shall be respected.
Verification: [Saucers.PlayerShotDestroysSaucerAndAwardsPoints](../tests/core_test.cpp), [Saucers.ContactDamagesShip](../tests/core_test.cpp), [Saucers.HostileShotsDamageShipButNotRocksAndRespectProtection](../tests/core_test.cpp)

Scenario: FR-CORE-021 Saucer aim uses shortest wrapped distance
GIVEN a ship and saucer separated across one or both playfield edges
WHEN the saucer fires
THEN its shot shall aim along the shortest wrapped direction.
Verification: [Saucers.AimUsesShortestWrappedDistanceOnBothAxes](../tests/core_test.cpp)

Feature: Pure input mapping.

Scenario: FR-INPUT-001 Joystick axes and buttons map to gameplay input
GIVEN a joystick value sample
WHEN `mapJoystickInput` is called
THEN it shall apply the 20% stick dead zone, prefer D-pad POV X at magnitude 50 or above, map upward stick/POV Y to thrust, and map buttons 0/1 to fire.
Verification: [InputMapping.MapsStickDeadZoneAndDpadPriority](../tests/core_test.cpp)

Scenario: FR-INPUT-002 Keyboard and joystick inputs combine without source mutation
GIVEN independent keyboard and joystick `Input` values
WHEN `combineInputs` is called
THEN it shall return their combined digital/analog controls without changing either input value.
Verification: [InputMapping.MapsStickDeadZoneAndDpadPriority](../tests/core_test.cpp), [InputMapping.CombinesSourcesWithoutMutatingThem](../tests/core_test.cpp)

Feature: Keyboard and application orchestration.

Scenario: FR-KEY-001 Keyboard commands map to activate, pause, and restart
GIVEN Enter, P/Escape, R, or an unrelated event
WHEN `keyboardCommand` processes the event
THEN it shall return the matching command or `none` for unrelated events.
Verification: Not automated; inspect `src/keyboard_input.cpp` and manually exercise the game.

Scenario: FR-APP-001 Start button is edge-triggered and controls application state
GIVEN a title, playing, paused, or game-over state and joystick button 7
WHEN button 7 transitions from released to pressed
THEN the application shall start, pause, resume, or restart as appropriate without repeating while held.
Verification: Not automated; manual application/controller check.

Scenario: FR-APP-002 Focus loss pauses gameplay and gates sampled controls
GIVEN an active game window
WHEN the window loses focus
THEN active play shall pause and keyboard/gamepad gameplay input shall not advance the simulation until focus returns and play is resumed.
Verification: Not automated; manual window-focus check.

Feature: Rendering and diagnostics.

Scenario: FR-SMOKE-001 Simulation smoke verifies movement and firing without rendering
GIVEN a seeded game
WHEN `runSimulationSmokeTest` advances the simulation with turn, thrust, and fire
THEN the ship shall move and at least one player bullet shall exist without requiring a graphics context.
Verification: [runSimulationSmokeTest](../src/smoke_test.cpp), run with `--simulation-smoke-test`.

Scenario: FR-RENDER-001 Render smoke produces visible images for all screens and sizes
GIVEN a seeded simulation state and each supported smoke size
WHEN `runRenderSmokeTest` draws title, game, pause, and game-over
THEN it shall save eight images and reject blank output.
Verification: [runRenderSmokeTest](../src/smoke_test.cpp), run with `./run.ps1 smoke`.

Scenario: FR-DIAG-001 Gamepad diagnostic displays connected device state and mapped controls
GIVEN a connected joystick or no connected joystick
WHEN `//:gamepad_test` runs
THEN its window shall identify the first connected joystick, show detected axis/button state, and highlight mapped controls or report no controller.
Verification: Not automated; manual run with `bazel run //:gamepad_test`.

## Non-Functional Requirements

Feature: Modularity, purity, and verification quality.

Scenario: NFR-ARCH-001 Core simulation remains independent of SFML
GIVEN the `//:core_test` target
WHEN Bazel resolves its dependency graph
THEN the core test shall depend on the SFML-free core and input mapping libraries, not SFML.
Verification: `BUILD.bazel` dependencies and successful headless `//:core_test` run.

Scenario: NFR-PURE-001 Input transformations do not mutate their arguments
GIVEN joystick samples and keyboard/joystick input values
WHEN pure mapping/composition functions are called
THEN the output shall be returned by value and the source values shall remain unchanged.
Verification: [InputMapping.CombinesSourcesWithoutMutatingThem](../tests/core_test.cpp)

Scenario: NFR-COVERAGE-001 Core and pure mapping coverage meet the configured quality gate
GIVEN the core and input-mapping test binary
WHEN `./run.ps1 coverage` is run
THEN line and branch coverage shall each be at least 95%.
Verification: [`tools/coverage.ps1`](../tools/coverage.ps1); current baseline after mapping extraction: 100% lines, 99.35% branches.

Scenario: NFR-TRACE-001 Automated tests maintain bidirectional requirement traceability
GIVEN a requirement verified by an automated test
WHEN the requirement or test implementation is reviewed
THEN the requirement shall name the suite/case and the test declaration shall carry its `REQ-*` ID comment.
Verification: This document and `tests/core_test.cpp` requirement comments.

Scenario: NFR-DOC-001 Project-owned C++ files have Doxygen file headers
GIVEN a project-owned `.cpp` or `.hpp` file
WHEN its file-level documentation is reviewed
THEN its Doxygen comment shall include a matching `@file` tag and a concise `@brief` description, and a `.hpp` comment shall immediately follow `#pragma once`.
Verification: Not automated; inspect all project-owned `.cpp` and `.hpp` files under `src/`, `include/`, and `tests/`.

## Requirement Coverage KPI

Calculate automated requirement coverage as:

`100 * requirement IDs with at least one passing linked automated test / all requirement IDs`

Count each requirement once, even if several cases verify it. Requirements marked
`Not automated` remain in the denominator and identify automation gaps; report
manual verification separately. Test source declarations use comments of the
form `// Requirements: FR-...` to provide the reverse trace.
