/**
 * @file console.cpp
 */

#include "shift_config.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#include <windows.h>
#include <optional>

static HANDLE hStdOut, hStdErr;
static std::optional<DWORD> stdOutOldConsoleMode, stdErrOldConsoleMode;

namespace shift {
	namespace logging {
		bool enable_colored_console(void) noexcept {
			stdOutOldConsoleMode.reset();
			stdErrOldConsoleMode.reset();

			hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			{
				DWORD temp;

				if (!GetConsoleMode(hStdOut, &temp))
					return false;

				if (!SetConsoleMode(hStdOut, temp | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT))
					return false;

				stdOutOldConsoleMode.emplace(temp);
			}

			hStdErr = GetStdHandle(STD_ERROR_HANDLE);
			{
				DWORD temp;

				if (!GetConsoleMode(hStdErr, &temp))
					return false;

				if (!SetConsoleMode(hStdErr, temp | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT))
					return false;

				stdErrOldConsoleMode.emplace(temp);
			}
			return true;
		}

		void disable_colored_console(void) noexcept {
			if (stdOutOldConsoleMode) {
				SetConsoleMode(hStdOut, stdOutOldConsoleMode.value());
				stdOutOldConsoleMode.reset();
			}

			if (stdErrOldConsoleMode) {
				SetConsoleMode(hStdErr, stdErrOldConsoleMode.value());
				stdErrOldConsoleMode.reset();
			}
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
	}
}
#endif

