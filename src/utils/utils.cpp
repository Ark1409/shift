#include <iostream>
#include <cstdlib>
#include "logging/console.h"

namespace shift {
    namespace utils {
        [[noreturn]] SHIFT_API void exit(int status) noexcept {
            std::cout.flush();
            std::cerr.flush();
            shift::logging::disable_colored_console();
            std::exit(status);
        }
    }
}