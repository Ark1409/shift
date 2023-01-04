/**
 * @file main.cpp
 */

#include "shift.h"

int main(const int argc, const char*const* const argv) {
	shift::enable_colored_console();

	std::cout << shift::lred << "Hello World in red!" << shift::creset << std::endl;

	std::cout << "parsing \"test.shift\":\n" << std::endl;

	const char* file = "test.shift";
	shift::compiler::shift_compiler compiler(1, &file);
	compiler.execute();

	shift::disable_colored_console();
	return 0;
}
