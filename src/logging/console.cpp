/**
 * @file console.cpp
 */

#include "console.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#include <windows.h>
#include <optional>

static HANDLE const hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
static HANDLE const hStdErr = GetStdHandle(STD_ERROR_HANDLE);
static std::optional<DWORD> stdOutOldConsoleMode, stdErrOldConsoleMode;

namespace shift {
	namespace logging {
		bool enable_colored_console(void) noexcept {
			stdOutOldConsoleMode.reset();
			stdErrOldConsoleMode.reset();

			stdOutOldConsoleMode.emplace(0x0);
			if (!GetConsoleMode(hStdOut, &stdOutOldConsoleMode.value()))
				return false;

			if (!SetConsoleMode(hStdOut, stdOutOldConsoleMode.value() | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
				return false;

			stdErrOldConsoleMode.emplace(0x0);
			if (!GetConsoleMode(hStdErr, &stdErrOldConsoleMode.value()))
				return false;

			if (!SetConsoleMode(hStdErr, stdErrOldConsoleMode.value() | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
				return false;

			return true;
		}

		void disable_colored_console(void) noexcept {
			stdOutOldConsoleMode.reset();
			stdErrOldConsoleMode.reset();
		}

		bool has_colored_console(void) noexcept {
			return stdOutOldConsoleMode && stdErrOldConsoleMode;
		}
	}
}

#else
 // linux / unix / mac implementation
namespace shift {
	namespace logging {
		void enable_colored_console(void) noexcept {}
		void disable_colored_console(void) noexcept {}

		bool has_colored_console(void) noexcept {
			return true; // function to determine whether we are writing to console or file? ; better to set to false
		}

		bool is_colored_console(void) noexcept { return has_colored_console(); }
	}
}
#endif

