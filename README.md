# Pebble Blaster

A playable C++23 vector arcade game using SFML 3.1 and Bazel. No external
fonts, textures, working-directory assumptions, or SFML DLL installation.
Bazel fetches and builds the pinned SFML and GoogleTest dependencies.

## Run on Windows

Prerequisite: Visual Studio 2022 or newer with **Desktop development with C++**
and a Windows SDK. A recent MSVC toolset is needed for C++23. Internet access
is needed on the first build; subsequent builds use Bazel's cache.

From the project directory:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1
```

The launcher detects Visual Studio and downloads checksum-verified Bazelisk
1.26.0 to your local application cache. Bazelisk selects Bazel 8.4.2 using
`.bazelversion`. No administrator privileges are needed by the launcher.
It uses Windows' trusted certificate store, including enterprise certificates,
without turning off TLS verification. Setup errors report the missing tool.

The first source build of SFML and its dependencies takes a few minutes.
The game opens with a **START** action; click it or press Enter.

## Controls

| Action | Key |
| --- | --- |
| Turn | Left/Right or A/D |
| Thrust | Up or W |
| Fire | Space (hold for repeated shots) |
| Pause/resume | P or Escape |
| Start/resume/play again | Enter, or click the menu action |
| Restart | R |
| Quit | Close the window |

Three lives, six simultaneous player shots, large/medium/small asteroid
splitting, hostile saucers, and increasing waves. Rocks award 20/50/100 points;
saucers award 200. Each 10,000 points earns a life. Respawns give three seconds
of blinking protection. Best score is retained for the current session.
Losing focus pauses the game. Resizing letterboxes the same playfield rather
than stretching it or changing the simulation.

## Tests and coverage

### Windows

Run these commands from the repository root. To compile the unit-test binary
without executing it:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\invoke-bazel.ps1 build //:core_test
```

To build and execute all Bazel tests, or only the gameplay core tests:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1 test
powershell -ExecutionPolicy Bypass -File .\tools\invoke-bazel.ps1 test //:core_test
```

The `//:core_test` target compiles `tests/core_test.cpp` and runs its GoogleTest
cases. Bazel handles building, test execution, and test logs; the Windows
executable is available at `bazel-bin\core_test.exe` after a successful build.

Coverage additionally requires **C++ Clang tools for Windows** in Visual Studio
Installer, or a standalone LLVM installation (`BAZEL_LLVM` can select it). Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1 coverage
```

This builds the GTests with Clang-cl instrumentation, executes them, and writes
LLVM JSON and HTML reports under `.artifacts/coverage` (`summary.json` and
`html\index.html`). It fails below **95% line or 95% branch coverage**.
Measurement includes the gameplay core and its header, excluding GTest, SFML,
and presentation code. Rendering and window/input integration are checked
separately by the smoke command, not counted as unit-test coverage.

### Render smoke check

The smoke check advances the simulation and renders eight screenshots; an
OpenGL-capable display is required:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1 smoke
```

The render smoke check saves eight PNGs under `.artifacts/screenshots`, covering
title, gameplay, pause, and game-over screens at 960x640 and 600x800. It fails
if rendering is blank or image saving fails; an OpenGL-capable display is
required. Tests of the simulation need no display or SFML context.

## Linux and macOS

Install Bazelisk, a C++23 compiler (GCC 13+/Clang 17+ recommended), and platform
development libraries. On Debian/Ubuntu:

```sh
sudo apt-get install g++ libgl-dev libudev-dev libx11-dev libxrandr-dev libxcursor-dev libxi-dev
bazelisk run //:pebble_blaster
bazelisk build //:core_test
bazelisk test //:core_test
bazelisk test //...
```

`bazelisk build //:core_test` compiles the test binary without running it.
Use `bazelisk test //:core_test` for just the gameplay core suite, or
`bazelisk test //...` for every test target.

On macOS install Xcode command-line tools and Bazelisk, then use the same
`bazelisk` commands. Windows is locally verified; Linux is configured for CI.
Native Bazel LCOV coverage for the gameplay core on Linux is available with:

```sh
bazelisk coverage //:core_test --combined_report=lcov --instrumentation_filter='//:core$'
```

On a headless Linux host, run the display-free simulation smoke check with:

```sh
bazelisk run //:pebble_blaster -- --simulation-smoke-test
```

The screenshot smoke test uses SFML's GLX-backed render textures and requires an
X server with compatible GLX visuals. Xvfb may not provide a suitable visual;
CI runs the full rendering smoke check on Windows instead.

## Design

- `core`: SFML-free simulation with value types and an explicit world snapshot.
  `Game::update` is the only simulation tick; the renderer gets const state.
- `renderer`: procedural line geometry and bitmap lettering; no file assets.
- `main`: SFML events, input sampling, focus handling, and a bounded accumulator
  stepping the simulation at 120 Hz independently of rendering speed.
- `core_test`: deterministic GTest scenarios covering movement, wrapping,
  collisions, splitting, scoring, lives, cooldowns, timers, and state transitions.

The random seed is injectable; the snapshot constructor supports isolated
scenarios. Gameplay uses circle collision bounds and a toroidal playfield,
including collisions across opposite edges. Wave size caps at twelve rocks.
Long frames are capped to prevent runaway catch-up; pause stops game time.

SFML and GoogleTest retain their upstream licenses. This project uses
procedural graphics and does not bundle copyrighted arcade assets.
