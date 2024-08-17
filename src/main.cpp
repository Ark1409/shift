/**
 * @file main.cpp
 *
 * Main file
 */
#include "logging/console.h"
#include "compiler/shift_compiler.h"

#include <typeinfo>

int main() {
    int ret_val;

    shift::logging::enable_colored_console();
    {
        using namespace shift::compiler;
        // compiler comp({"../../test/test3.shift", "../../test/test2.shift" });
        //compiler comp({ "../../test/builtin.shift", "../../test/test5.shift", "../../test/file.shift" });
//        compiler comp({ "../../test/builtin.shift", "../../test/test5.shift" });
        compiler comp({ "../../../test/test7.shift", "../../../test/builtin.shift" });
        comp.get_error_handler().enable_warnings();
        comp.run();
        ret_val = comp.get_error_handler().get_error_count() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        comp.get_error_handler().print();
    }
    shift::logging::disable_colored_console();

    return ret_val;
}