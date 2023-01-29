/**
 * @file main.cpp
 *
 * Main file
 */
#include "logging/console.h"
#include "compiler/shift_parser.h"

int main() {
    shift::logging::enable_colored_console();
    std::cout << shift::logging::lred << "Red text" << shift::logging::creset << std::endl;
    shift::logging::disable_colored_console();
    return 0;
}