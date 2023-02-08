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
        compiler comp({std::string_view("../../test/file.shift")});
        comp.get_error_handler().enable_warnings();
        comp.parse_flags();
        comp.tokenize();
        comp.parse();
        comp.get_error_handler().print_clear();
    }
    shift::logging::disable_colored_console();
    return 0;
}