/**
 * @file console.cpp
 */

#include "console.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#include <windows.h>

static bool gotStdOutOldConsole = false;
static bool gotStdErrOldConsole = false;

static HANDLE const hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
static HANDLE const hStdErr = GetStdHandle(STD_ERROR_HANDLE);
static DWORD stdOutOldConsoleMode = 0x0;
static DWORD stdErrOldConsoleMode = 0x0;

namespace shift {
	void SHIFT_API enable_colored_console(void) noexcept {
		if (!gotStdOutOldConsole) {
			if (GetConsoleMode(hStdOut, &stdOutOldConsoleMode)) {
				SetConsoleMode(hStdOut, stdOutOldConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
				gotStdOutOldConsole = true;
			}
		}

		if (!gotStdErrOldConsole) {
			if (GetConsoleMode(hStdErr, &stdErrOldConsoleMode)) {
				SetConsoleMode(hStdErr, stdErrOldConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
				gotStdErrOldConsole = true;
			}

		}
	}

	void SHIFT_API disable_colored_console(void) noexcept {
		if (gotStdOutOldConsole) {
			SetConsoleMode(hStdOut, stdOutOldConsoleMode);
			gotStdOutOldConsole = false;
		}

		if (gotStdErrOldConsole) {
			SetConsoleMode(hStdErr, stdErrOldConsoleMode);
			gotStdErrOldConsole = false;
		}
	}

	bool SHIFT_API has_colored_console(void) noexcept {
		DWORD temp;
		return GetConsoleMode(hStdOut, &temp) && GetConsoleMode(hStdErr, &temp);
	}

	bool SHIFT_API is_colored_console(void) noexcept {
		return gotStdOutOldConsole && gotStdErrOldConsole && has_colored_console();
	}
}

#else
// linux / unix / mac implementation
namespace shift {
	void SHIFT_API enable_colored_console(void) noexcept {}
	void SHIFT_API disable_colored_console(void) noexcept {}

	bool SHIFT_API has_colored_console(void) noexcept {
		return true; // function to determine whether we are writing to console or file? ; better to set to false
	}

	bool SHIFT_API is_colored_console(void) noexcept {
		return has_colored_console();
	}
}
#endif

