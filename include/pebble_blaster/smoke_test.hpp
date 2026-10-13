#pragma once
/**
 * @file smoke_test.hpp
 * @brief Declares simulation and rendering smoke-test entry points.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */

#include <filesystem>

namespace pebble_blaster {

int runSimulationSmokeTest();
int runRenderSmokeTest(const std::filesystem::path &directory);

} // namespace pebble_blaster