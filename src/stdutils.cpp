/**
 * @file stdutils.cpp
 */

#include "stdutils.h"
#include "console.h"

namespace shift {
	[[noreturn]] SHIFT_API void exit(int status) {
		std::cout.flush();
		std::cerr.flush();
		shift::disable_colored_console();
		std::exit(status);
	}
}
