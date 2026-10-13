#pragma once

#include <filesystem>

namespace pebble_blaster {

int runSimulationSmokeTest();
int runRenderSmokeTest(const std::filesystem::path &directory);

} // namespace pebble_blaster