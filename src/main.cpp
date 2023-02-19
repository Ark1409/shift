/**
 * @file main.cpp
 *
 * Main file
 */
#include "logging/console.h"
#include "compiler/shift_compiler.h"

int main() {
    shift::logging::enable_colored_console();
    {
        using namespace shift::compiler;
        compiler comp({"../../test/file.shift", "../../test/test2.shift"});
        comp.get_error_handler().enable_warnings();
        comp.run();
        comp.get_error_handler().print_clear();
    }
    shift::logging::disable_colored_console();
    return 0;
}