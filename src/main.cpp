/**
 * @file main.cpp
 * @brief Entry point for the Pebble Blaster game application.
 *
 * Entry point for an Arcade style fixed-shooter experience with
 * modern engineering quality.
 *
 * @author Oliver Merkel
 * @date 2026-10-14
 */
#include "pebble_blaster/application.hpp"
#include "pebble_blaster/smoke_test.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

int main(int argumentCount, char *arguments[]) {
    try {
        if (argumentCount == 2 && std::string_view(arguments[1]) == "--simulation-smoke-test") {
            return pebble_blaster::runSimulationSmokeTest();
        }
        if (argumentCount >= 2 && std::string_view(arguments[1]) == "--smoke-test") {
            return pebble_blaster::runRenderSmokeTest(
                argumentCount >= 3 ? arguments[2] : ".artifacts/screenshots");
        }
        if (argumentCount != 1) {
                std::cerr << "Usage: pebble_blaster [--simulation-smoke-test | --smoke-test [output-directory]]\n";
            return 2;
        }
        return pebble_blaster::runGame();
    } catch (const std::exception &error) {
            std::cerr << "Pebble Blaster: " << error.what() << '\n';
        return 1;
    }
}
