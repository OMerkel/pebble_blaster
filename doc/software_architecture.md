# Software Architecture

This document describes the implementation in `src/`, `include/pebble_blaster/`,
`tests/`, and the Bazel/PowerShell build entry points. It is intended as a
developer reference. The diagrams use Mermaid syntax and describe the current
code, including the separation between the simulation and the SFML presentation
layer.

## System Overview

Pebble Blaster is a single-process C++23 desktop game. The gameplay core is a
deterministic, SFML-independent simulation; the application layer translates
SFML events and held keys into core inputs; the renderer reads the core's
snapshot and draws procedural geometry. Bazel owns dependency resolution and
build/test execution.

```mermaid
flowchart LR
    User([Player]) -->|keyboard and mouse| Main["src/main.cpp<br/>window, events, fixed-step loop"]
    Main -->|Input + elapsed tick| Core["pebble_blaster::Game<br/>src/core.cpp"]
    Core -->|const World snapshot| Main
    Main -->|Game + Screen + presentation state| Renderer["pebble_blaster::Renderer<br/>src/renderer.cpp"]
    Renderer -->|draw calls| SFML["SFML Graphics"]
    SFML --> Display[(Window / display)]
    Build["Bazel 8.4.2"] --> Main
    Build --> Core
    Build --> Renderer
    Build --> Tests["core_test / GoogleTest"]
    Build -. fetches pinned dependencies .-> Deps["SFML 3.1.0, GoogleTest 1.17.0"]
```

The core does not depend on SFML and can be tested without a display or graphics
context. Rendering and event handling are not part of the core coverage
measurement; the executable's smoke mode separately exercises simulation and
render output.

## Components and Dependencies

| Component | Responsibility | Depends on |
| --- | --- | --- |
| `core` | World state, rules, deterministic time-step simulation, collision and scoring | C++ standard library |
| `renderer` | Procedural scene, HUD, menu screens, viewport and clickable action bounds | `core`, SFML Graphics |
| `pebble_blaster` application | Window/event loop, key sampling, focus, accumulator, title and best-score session state | `core`, `renderer`, SFML Graphics |
| `core_test` | Headless unit tests for simulation and geometry | `core`, GoogleTest |
| `run.ps1` and `tools/*.ps1` | Windows build/test/coverage/smoke entry points and Bazelisk setup | PowerShell, Visual Studio C++ tools; LLVM for coverage |

```mermaid
classDiagram
    class Vec2 {
      +float x
      +float y
      +operator+(Vec2) Vec2
      +operator-(Vec2) Vec2
      +operator*(float) Vec2
    }
    class Input {
      +bool left
      +bool right
      +bool thrust
      +bool fire
    }
    class Game {
      -World world_
      -mt19937 random_
      -Vec2 bounds_
      +Game(seed, bounds)
      +Game(world, seed, bounds)
      +restart()
      +togglePause()
      +update(seconds, input)
      +world() const World&
      +bounds() const Vec2
    }
    class World {
      +Ship ship
      +vector~Rock~ rocks
      +vector~Bullet~ bullets
      +vector~Particle~ particles
      +optional~Saucer~ saucer
      +Phase phase
      +int score
      +int lives
      +int wave
      +float elapsed
    }
    class Ship {
      +Vec2 position
      +Vec2 velocity
      +float angle
      +float protection
    }
    class Rock {
      +Vec2 position
      +Vec2 velocity
      +RockSize size
      +float angle
      +float spin
      +uint32 shape
    }
    class Bullet {
      +Vec2 position
      +Vec2 velocity
      +float lifetime
      +bool hostile
    }
    class Particle {
      +Vec2 position
      +Vec2 velocity
      +float lifetime
    }
    class Saucer {
      +Vec2 position
      +Vec2 velocity
      +float lifetime
      +float shotTimer
      +float oscillationTime
      +float oscillationPhase
    }
    class Renderer {
      +draw(target, game, screen, thrust, bestScore)
      +actionContains(point, bounds) bool
      +view(pixels, bounds) sf::View
    }
    class Screen {
      <<enumeration>>
      title
      game
      pause
      gameOver
    }
    class Phase {
      <<enumeration>>
      playing
      paused
      gameOver
    }
    Game *-- World : owns by value
    World *-- Ship
    World *-- Rock : zero or more
    World *-- Bullet : zero or more
    World *-- Particle : zero or more
    World o-- Saucer : optional
    Game ..> Input : consumes per tick
    Renderer ..> Game : reads const state
    Renderer ..> Screen
    World --> Phase
```

`World` and its entity types are value data, not active objects with their own
update methods. `Game` owns the mutable world and performs all simulation
mutations. `Game::world()` exposes a const reference, so callers can inspect but
cannot mutate live state through that accessor. The second `Game` constructor
accepts an already-created `World` for controlled scenarios; unlike the
seed-only constructor, it does not call `restart()` or create a starting wave.

## Runtime States

The application has a title flag in `main` in addition to `World::phase`.
Consequently, the title screen is a presentation/application state and is not a
`Phase` value. The best score is also held by the application and lasts only for
the current process.

```mermaid
stateDiagram-v2
    [*] --> Title
    Title --> Playing: Enter or menu click / restart; clear title flag
    Playing --> Paused: P, Escape, or focus lost
    Paused --> Playing: P, Escape, Enter, or Resume click
    Playing --> GameOver: ship loses final life
    GameOver --> Playing: Enter or Play Again click / restart
    Playing --> Playing: R / retain best score, restart
    Paused --> Playing: R / retain best score, restart
    GameOver --> Playing: R / retain best score, restart
    GameOver --> GameOver: pause toggle is ignored
```

Pausing freezes simulation time because the application stops invoking
`Game::update`; `Game::update` also returns immediately when the phase is not
`playing`. Focus regain alone does not resume play. The title and game-over
action buttons are activated by a left click inside the renderer's action
rectangle; pause's action is Resume. Restart (`R`) is accepted on every
non-title screen and updates the process-local best score before resetting the
game.

## Use Cases

```mermaid
flowchart LR
    Player([Player])
    Developer([Developer / CI])
    Play((Play or resume))
    Controls((Turn, thrust, fire))
    Pause((Pause / resume))
    Restart((Restart / play again))
    Observe((View HUD and game state))
    Quit((Close window))
    Build((Build executable))
    Unit((Run core tests))
    Cov((Measure core coverage))
    Smoke((Render smoke check))
    Player --> Play
    Player --> Controls
    Player --> Pause
    Player --> Restart
    Player --> Observe
    Player --> Quit
    Developer --> Build
    Developer --> Unit
    Developer --> Cov
    Developer --> Smoke
```

## Input and Public Contracts

### Construction and state

| API | Contract |
| --- | --- |
| `Game(seed, bounds)` | Defaults to bounds `1200 x 800`; rejects non-finite dimensions or dimensions below `400 x 300` with `std::invalid_argument`. Initializes a fresh world via `restart()`, placing the ship at center and spawning wave 1. |
| `Game(world, seed, bounds)` | Applies the same bounds validation and retains the supplied world as-is. It does not automatically restart, position the ship, or populate rocks. The seed controls subsequent random operations. |
| `restart()` | Replaces world state with default `World` values, places the ship at the playfield center, and spawns the current initial wave (wave 1). The `Game`'s random engine is not reconstructed, so a restart advances the existing random stream. |
| `togglePause()` | Switches `playing` and `paused`; has no effect in `gameOver`. |
| `world()` | Returns a const reference to the current world. Its lifetime is tied to the `Game`; callers must not retain it past the owning game. |

### Simulation tick

`Input` is a sampled set of held controls. A true `left` and `right` value
cancel rotational input; `thrust` accelerates while held; `fire` attempts to
fire while held, subject to cooldown and shot limits. The core does not know
about keyboard keys or mouse events.

`Game::update(seconds, input)` follows these rules:

- Does nothing unless the phase is `playing` and `seconds` is finite and
  greater than zero.
- Caps an accepted tick to `0.05` seconds. The application normally supplies
  `1/120` second ticks, so this is a defensive core-level upper bound.
- Advances elapsed time and ship, rock, bullet, particle, and saucer state;
  expires transient objects; resolves collisions; then advances the wave timer
  when the rock field is empty.
- The coordinate space is toroidal: entities wrap across both edges, and
  collision distances use the shorter wrapped distance.

The application caps an individual rendered frame contribution to `0.1`
seconds and uses an accumulator to call `update(1/120, input)` zero or more
times per rendered frame. It clears the accumulator while in a menu, paused, or
unfocused, avoiding a large catch-up burst after a pause.

### Input mapping

| Game action | Application input |
| --- | --- |
| Turn left / right | Left or A / Right or D |
| Thrust | Up or W |
| Fire | Space, held for repeated shots |
| Pause / resume | P or Escape |
| Start / resume / play again | Enter or click the menu action |
| Restart | R, outside the title screen |
| Quit | Close window |

On focus loss, the application clears the focused-input flag and pauses an
active game. On focus regain it resumes sampling controls, but a paused game
remains paused until an explicit resume action.

## Gameplay Rules

- A new game starts with 3 lives, score 0, wave 1, a centered ship, and
  `min(3 + wave, 12)` large rocks. At wave 1 this is 4 rocks.
- Ship turning is `4.2` radians/second. Thrust acceleration is `310` world
  units/second squared; speed is capped at `420` and drag is exponential at
  `0.12` per second.
- The player can have at most 6 simultaneous non-hostile bullets. Fire cooldown
  is `0.16` seconds. Player bullets inherit ship velocity, travel at an
  additional `600` units/second, and live `1.1` seconds.
- Rock radii are 44 (large), 25 (medium), and 13 (small). Destroying a large
  rock creates two medium rocks; a medium creates two small rocks; a small
  creates no fragments. Each destruction creates 18 particles.
- Rock scores are 20 (large), 50 (medium), and 100 (small). A saucer is worth
  200. Every 10,000 points awards one life, with support for crossing multiple
  thresholds in one award.
- Ship collision protection starts at 3 seconds and is restored after a
  non-final hit. Losing the final life changes phase to `gameOver`; pause cannot
  leave that phase.
- A new wave starts after the field remains empty for the 2-second wave timer.
  Wave count increases without a cap; rock count caps at 12. Rock speed
  increases with wave number.
- Saucers are scheduled after a timer, live up to 10 seconds, move across the
  wrapped playfield with a 12-unit sinusoidal vertical weave, and fire hostile
  shots toward the ship with aim variation. Hostile bullets can hit the ship
  but do not destroy rocks.
- Resizing changes the viewport to preserve the playfield aspect ratio and
  letterboxes unused space; it does not resize the simulation bounds.

## Tick and Message Workflows

### Frame-to-tick workflow

```mermaid
flowchart TD
    Frame["Start rendered frame"] --> Events["Poll SFML events"]
    Events --> Resize{"Resized?"}
    Resize -->|yes| View["Recompute letterboxed view"]
    Resize -->|no| Focus
    View --> Focus["Apply focus and state transitions"]
    Focus --> Sample["Sample held keys when focused"]
    Sample --> Active{"Not title, playing, and focused?"}
    Active -->|no| Clear["Clear time accumulator"]
    Active -->|yes| Accumulate["Add min(frame time, 0.1 s)"]
    Accumulate --> Due{"Accumulator >= 1/120 s?"}
    Due -->|yes| Tick["Game.update(1/120 s, Input)"]
    Tick --> Subtract["Subtract one fixed step"]
    Subtract --> Due
    Due -->|no| Best["Update session best score"]
    Clear --> Best
    Best --> Draw["Renderer.draw current snapshot"]
    Draw --> Present["Display frame"]
    Present --> Frame
```

### Object message exchange: one simulation update

The entities below are data objects. The messages show calls between the
application, `Game`, and renderer, plus the mutations `Game` performs on its
owned `World`; they do not imply that `Ship`, `Rock`, or `Bullet` implement
independent methods.

```mermaid
sequenceDiagram
    participant MainLoop as Main loop
    participant Input as Input sample
    participant Game as Game instance
    participant World as Owned World
    participant RNG as Random engine
    participant Renderer
    participant Target as SFML RenderTarget
    MainLoop->>Input: Sample focused held keys
    MainLoop->>Game: update(fixedSeconds, Input)
    Game->>World: Check phase and elapsed time
    Game->>World: Move and wrap ship/entities
    Game->>RNG: Generate scheduled/spawned variation
    Game->>World: Resolve bullet and ship collisions
    Game->>World: Apply score, lives, fragments, effects, phase
    Game-->>MainLoop: Return with mutated World
    MainLoop->>Renderer: draw(Game const&, Screen, thrust, bestScore)
    Renderer->>World: Read snapshot through Game::world()
    Renderer->>Target: Draw scene and interface geometry
    Target-->>MainLoop: Present frame
```

### Collision, destruction, and score exchange

```mermaid
sequenceDiagram
    participant Game
    participant World
    participant PlayerBullet as Player bullet data
    participant Rock as Rock data
    participant RNG as Random engine
    Game->>World: Iterate bullets and test wrapped overlap
    Game->>PlayerBullet: Mark hit bullet expired
    Game->>Game: destroyRock(index)
    Game->>World: Remove rock and add score
    Game->>RNG: Choose fragment spin/shape and particle values
    Game->>World: Add two child rocks when parent is not small
    Game->>World: Add explosion particles
    Game->>World: Award extra life at each crossed 10,000 threshold
    Game-->>Game: Continue collision pass, then erase expired bullets
```

## Deployment and Build

The application has no service tier or network protocol at runtime. Network
access is needed on a clean build to fetch Bazelisk (Windows launcher) and
pinned Bazel dependencies; it is not needed by gameplay after the build.

```mermaid
flowchart TB
    subgraph Dev[Developer or CI host]
      Source["Repository source"]
      Launcher["run.ps1 / invoke-bazel.ps1"]
      Bazelisk["Bazelisk 1.26.0 (Windows launcher)"]
      Bazel["Bazel 8.4.2"]
      Cache[("Bazel-managed repository and build caches")]
      Outputs["Bazel outputs: pebble_blaster, core_test"]
      Source --> Launcher --> Bazelisk --> Bazel
      Bazel <--> Cache
      Bazel --> Outputs
    end
    Internet[("Dependency download on initial build")]
    Bazel -. resolve/fetch .-> Internet
    subgraph Runtime["Desktop runtime process"]
      App["pebble_blaster executable"]
      SFML["SFML Graphics"]
      OS["Window system and graphics driver"]
      App --> SFML --> OS
    end
    Outputs -->|launch| App
    subgraph CI["CI platforms"]
      Win["Windows: unit tests, coverage, build"]
      Linux["Ubuntu: unit tests, Xvfb render smoke"]
    end
    Bazel --> Win
    Bazel --> Linux
```

### Build graph

`BUILD.bazel` defines three targets: `//:core` (`cc_library`),
`//:renderer` (`cc_library`), and `//:pebble_blaster` (`cc_binary`), plus
`//:core_test` (`cc_test`). `renderer` depends on `core` and SFML Graphics;
`pebble_blaster` depends on both libraries and SFML Graphics; `core_test` depends
only on `core` and GoogleTest. `MODULE.bazel` pins SFML 3.1.0 and GoogleTest
1.17.0, while `.bazelversion` selects Bazel 8.4.2.

### Common commands

| Goal | Windows | Linux / macOS with Bazelisk |
| --- | --- | --- |
| Run game | `./run.ps1` | `bazelisk run //:pebble_blaster` |
| Run tests | `./run.ps1 test` | `bazelisk test //...` |
| Coverage | `./run.ps1 coverage` (Windows LLVM required) | Linux LCOV command in `README.md` |
| Render smoke | `./run.ps1 smoke` (display required) | `xvfb-run -a bazelisk run //:pebble_blaster -- --smoke-test /absolute/path` |

On Windows, `tools/invoke-bazel.ps1` discovers Visual Studio C++ tools, downloads
checksum-verified Bazelisk into the local application cache if absent, and
forwards the command to Bazelisk. `tools/coverage.ps1` builds and runs the core
test binary with LLVM instrumentation, emits JSON/HTML, and enforces the
configured line/branch thresholds (95% by default). See `README.md` for
platform prerequisites and full commands.

## Storage and Generated Artifacts

The game does not save scores, settings, replays, or user data. `World`, random
state, and the best score are process-memory state; best score is reset when
the application exits. The renderer generates its visual assets procedurally,
so there are no runtime font or texture files.

```mermaid
flowchart LR
    App["Game process"] --> Memory["RAM: World, entities, RNG, session best score"]
    App -->|smoke-test only| Screens[".artifacts/screenshots/*.png"]
    Tests["Coverage script"] --> Raw[".artifacts/coverage/core.profraw"]
    Raw --> Profile[".artifacts/coverage/core.profdata"]
    Profile --> Reports["summary.json and html/"]
    Build["Bazel"] --> Generated["bazel-bin, bazel-out, bazel-testlogs and Bazel cache"]
    Launcher["Windows Bazelisk bootstrap"] --> LocalTool["%LOCALAPPDATA%/pebble-blaster-tools/bazelisk.exe"]
```

| Location | Contents and lifecycle |
| --- | --- |
| Process memory | Live world, PRNG state, and current-process best score; discarded at exit. |
| `.artifacts/screenshots/` | Eight smoke images by default: four screens at `1200x800` and `600x800`; generated by smoke mode. |
| `.artifacts/coverage/` | Raw/merged LLVM profiles, coverage summary JSON, and HTML report; generated by the Windows coverage script. |
| `bazel-bin`, `bazel-out`, `bazel-testlogs` | Bazel output locations/symlinks for built artifacts and test logs; generated and rebuildable. |
| Bazel-managed cache | Downloaded repositories and build cache outside application-owned game data; location is controlled by Bazel. |
| `%LOCALAPPDATA%/pebble-blaster-tools/bazelisk.exe` | Windows Bazelisk bootstrap binary, verified against the pinned SHA-256 by the launcher. |

Generated build and `.artifacts` paths are ignored by `.gitignore`. No generated
location is read by gameplay as saved state.

## Tests and Coverage

### Test structure

`tests/core_test.cpp` contains GoogleTest cases organized by behavior. A helper
`scenario()` constructs a deliberately small world for isolated cases, while
`advance()` runs repeated deterministic ticks. The parameterized
`RockDestruction` suite is instantiated for all three `RockSize` values. The
current file defines 32 executed cases when those three parameter values are
counted separately.

```mermaid
flowchart TD
    BazelTest["bazel test //... or run.ps1 test"] --> CoreTarget["//:core_test"]
    CoreTarget --> Fixture["scenario() / advance() helpers"]
    Fixture --> Geometry["Geometry"]
    Fixture --> GameSuite["Game"]
    Fixture --> Ship["Ship"]
    Fixture --> Weapons["Weapons"]
    Fixture --> Rocks["RockDestruction parameters + Rocks"]
    Fixture --> Lives["Lives"]
    Fixture --> Waves["Waves"]
    Fixture --> Particles["Particles"]
    Fixture --> Saucers["Saucers"]
    CoreTarget --> Gate["CI: Windows unit tests"]
    CoreTarget --> Coverage["Windows LLVM line/branch gate"]
    Smoke["Executable --smoke-test"] --> SimCheck["movement + player fire"]
    Smoke --> RenderCheck["four screens x two aspect ratios"]
    Smoke --> CI2["CI: Linux under Xvfb"]
```

### Suite and case catalog

| Suite | Cases and intended contract |
| --- | --- |
| `Geometry` | `VectorArithmetic` checks vector operations and direction; `WrapsNegativePositiveAndMultipleScreenCrossings` checks positive/negative multi-span wrapping; `CollidesAcrossEdgesAndIncludesTangency` checks toroidal overlap and inclusive radius tangency. |
| `Game` | `StartsWithThreeLivesAndFourSafeRocks` checks initial state and safe spawn distance; `RejectsInvalidPlayfields` checks dimensions and finiteness; `SeedReproducesWave` checks deterministic seeded wave generation; `PausingFreezesSimulationAndResumes` checks phase/time behavior; `IgnoresInvalidTimeAndCapsLargeSteps` checks invalid deltas and the 50 ms cap. |
| `Ship` | `TurnsBothWaysAndOpposedControlsCancel` checks angular input; `AcceleratesCoastsAndCapsSpeed` checks thrust, drag, and maximum speed; `WrapsAtThePlayfieldBoundary` checks ship position wrapping. |
| `Weapons` | `FiresForwardInheritsVelocityAndHonorsCooldown` checks direction, inherited velocity, and cooldown; `LimitsPlayerShotsButNotHostileShots` checks the six-player-shot limit; `BulletsWrapAndExpire` checks bullet movement and lifetime. |
| `RockDestruction` (parameterized) | `AwardsPointsAndSplitsIntoCorrectSize` runs for large, medium, and small rocks; checks score, bullet removal, particles, fragment count, and fragment size. |
| `Rocks` | `MoveRotateAndWrap` checks rock motion; `OneBulletDestroysOnlyOneRock` checks one-hit/one-rock resolution; `InvalidSizeIsRejected` checks invalid enum handling by `radius` and `points`. |
| `Lives` | `ProtectionPreventsCollisionsThenOneHitRespawns` checks protection, life loss, reset position, and re-protection; `LastHitEndsGamePauseCannotReviveAndRestartResets` checks game-over, ignored pause, and restart reset; `AwardsExtraLifeOncePerTenThousandPoints` checks score threshold behavior. |
| `Waves` | `EmptyFieldWaitsThenSpawnsNextWave` checks the inter-wave delay; `DifficultyCapsRockCount` checks the 12-rock cap. |
| `Particles` | `MoveWrapAndExpire` checks particle motion, wrapping, and lifetime removal. |
| `Saucers` | `TimerSpawnsSaucerAndItFiresAndExpires` checks schedule, hostile fire, and expiry; `WeavesSinusoidallyAroundItsBaseline` checks the vertical weave; `PlayerShotDestroysSaucerAndAwardsPoints` checks hit and reward; `ContactDamagesShip` checks contact damage; `HostileShotsDamageShipButNotRocksAndRespectProtection` checks hostile targeting/collision and protection; `AimUsesShortestWrappedDistanceOnBothAxes` checks toroidal aiming. |

### Verification layers

- **Unit tests:** `bazel test //...` or `./run.ps1 test`. The target is
  display-independent and tests the gameplay core and geometry through public
  contracts and controlled `World` scenarios.
- **Coverage:** `./run.ps1 coverage` instruments the core test binary with
  Windows Clang/LLVM. By default it fails below 95% line or branch coverage.
  SFML and presentation code are excluded from this measurement; the coverage
  script also excludes tests and external code from the report.
- **Render smoke:** `./run.ps1 smoke` (or `pebble_blaster --smoke-test <directory>`)
  advances a seeded game with turn/thrust/fire input, verifies ship movement
  and that bullets exist, then renders title, game, pause, and game-over screens
  at landscape and portrait sizes. It requires an OpenGL-capable display and
  checks both a minimum lit-pixel threshold and successful PNG writes. It is an
  integration sanity check, not a pixel-perfect golden-image test.
- **CI:** `.github/workflows/ci.yml` runs Windows unit tests, coverage, and a
  game build; Ubuntu runs unit tests and a render smoke check under Xvfb. The
  Linux job uploads screenshots; Windows uploads coverage artifacts.

The core tests use injected seeds and hand-built worlds to avoid depending on
uncontrolled game startup conditions. Tests assert gameplay outcomes rather
than implementation-private helper call sequences. Coverage is a separate
quality gate and does not replace assertions or the SFML rendering smoke check.

## Architectural Constraints and Extension Guidance

- Keep gameplay rules in `core` and keep this target free of SFML so simulation
  tests remain headless and deterministic.
- Add user input by mapping it to `Input` in the application; do not introduce
  keyboard or window APIs into `Game`.
- Render from `Game::world()` and presentation arguments. Avoid mutating
  gameplay from `Renderer`.
- When adding persistent scores or settings, introduce and document an explicit
  storage contract; there is no persistence layer today.
- Extend `tests/core_test.cpp` with focused deterministic cases for core rule
  changes. Changes to actual drawing, window events, focus, or viewport behavior
  should also receive smoke/integration coverage.
- Keep Bazel target dependencies narrow: only the renderer and application
  targets need SFML Graphics; the core test target should remain independent of
  SFML and its display/runtime requirements.
