/**
 * @file logging/console.h
 */
#ifndef SHIFT_CONSOLE_H_
#define SHIFT_CONSOLE_H_ 1

#include "shift_config.h"

#include <iostream>

 /// Console color manipulation defines
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_RED					"\x1B[31m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_GREEN				"\x1B[32m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_YELLOW				"\x1B[33m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BLUE					"\x1B[34m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_MAGENTA				"\x1B[35m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_CYAN					"\x1B[36m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_WHITE				"\x1B[37m"

#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLACK			"\x1B[90m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_RED			"\x1B[91m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_GREEN			"\x1B[92m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BLACK				"\x1B[30m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_YELLOW		"\x1B[93m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLUE			"\x1B[94m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_MAGENTA		"\x1B[95m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_CYAN			"\x1B[96m"
#define SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_WHITE			"\x1B[97m"

#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BLACK				"\033[3;40;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_RED					"\033[3;41;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_GREEN				"\033[3;42;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_YELLOW				"\033[3;43;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BLUE					"\033[3;44;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_MAGENTA				"\033[3;45;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_CYAN					"\033[3;46;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_WHITE				"\033[3;47;30m"

#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLACK			"\033[3;100;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_RED			"\033[3;101;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_GREEN			"\033[3;102;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_YELLOW		"\033[3;103;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLUE			"\033[3;104;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_MAGENTA		"\033[3;105;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_CYAN			"\033[3;106;30m"
#define SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_WHITE			"\033[3;107;30m"

#define SHIFT_CONSOLE_RESET									"\033[0m"
/** namespace shift */
namespace shift::logging {

	/**
	 * Allows changing of color in the console.
	 * @return True if coloured console text was successfully enabled, false otherwise.
	 */
	SHIFT_API bool enable_colored_console(void) noexcept;

	/**
	 * Disables change of color in the console.
	 */
	SHIFT_API void disable_colored_console(void) noexcept;

	/**
	 * Checks whether colored console has been enabled (by enable_colored_console)
	 * @return True if colored console has been enabled, false otherwise.
	 */
	SHIFT_API bool has_colored_console(void) noexcept;

	/** Utility functions for standard output streams that permit colored console text. */

	/// Resets all color formatting back to the default (usually white text with black background on normal cmd.exe).
	inline std::ostream& creset(std::ostream& os) {
		return os << SHIFT_CONSOLE_RESET;
	}

	/// Black text
	inline std::ostream& black(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BLACK;
	}

	/// Red text
	inline std::ostream& red(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_RED;
	}

	/// Green text
	inline std::ostream& green(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_GREEN;
	}

	/// Yellow text
	inline std::ostream& yellow(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_YELLOW;
	}

	/// Blue text
	inline std::ostream& blue(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BLUE;
	}

	/// Magenta text
	inline std::ostream& magenta(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_MAGENTA;
	}

	/// Cyan text
	inline std::ostream& cyan(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_CYAN;
	}

	/// White text
	inline std::ostream& white(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_WHITE;
	}

	/// Bright black text
	inline std::ostream& lblack(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLACK;
	}

	/// Light red text
	inline std::ostream& lred(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_RED;
	}

	/// Light green text
	inline std::ostream& lgreen(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_GREEN;
	}

	/// Light yellow text
	inline std::ostream& lyellow(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_YELLOW;
	}

	/// Light blue text
	inline std::ostream& lblue(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLUE;
	}

	/// Light magenta text
	inline std::ostream& lmagenta(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_MAGENTA;
	}

	/// Light cyan text
	inline std::ostream& lcyan(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_CYAN;
	}

	/// Bright white text
	inline std::ostream& lwhite(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_WHITE;
	}

	/// Black background
	inline std::ostream& bblack(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BLACK;
	}

	/// Red background
	inline std::ostream& bred(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_RED;
	}

	/// Green background
	inline std::ostream& bgreen(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_GREEN;
	}

	/// Yellow background
	inline std::ostream& byellow(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_YELLOW;
	}

	/// Blue background
	inline std::ostream& bblue(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BLUE;
	}

	/// Magenta background
	inline std::ostream& bmagenta(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_MAGENTA;
	}

	/// Cyan background
	inline std::ostream& bcyan(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_CYAN;
	}

	/// White background
	inline std::ostream& bwhite(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_WHITE;
	}

	/// Bright black background
	inline std::ostream& blblack(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLACK;
	}

	/// Light red background
	inline std::ostream& blred(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_RED;
	}

	/// Light green background
	inline std::ostream& blgreen(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_GREEN;
	}

	/// Light yellow background
	inline std::ostream& blyellow(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_YELLOW;
	}

	/// Light blue background
	inline std::ostream& blblue(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLUE;
	}

	/// Light magenta background
	inline std::ostream& blmagenta(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_MAGENTA;
	}

	/// Light cyan background
	inline std::ostream& blcyan(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_CYAN;
	}

	/// Bright white background
	inline std::ostream& blwhite(std::ostream& os) {
		return os << SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_WHITE;
	}
}

#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BLACK
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_RED
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_GREEN
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_YELLOW
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BLUE
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_MAGENTA
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_CYAN
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_WHITE

#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLACK
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_RED
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_GREEN
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_YELLOW
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLUE
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_MAGENTA
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_CYAN
#undef SHIFT_CONSOLE_BEGIN_FOREGROUND_BRIGHT_WHITE

#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BLACK
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_RED
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_GREEN
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_YELLOW
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BLUE
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_MAGENTA
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_CYAN
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_WHITE

#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLACK
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_RED
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_GREEN
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_YELLOW
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLUE
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_MAGENTA
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_CYAN
#undef SHIFT_CONSOLE_BEGIN_BACKGROUND_BRIGHT_WHITE

#undef SHIFT_CONSOLE_RESET

#endif /* SHIFT_CONSOLE_H_ */
