/**
 * @file shift_error_handler.cpp
 */
#include "shift_error_handler.h"

#include <iostream>
#include "stdutils.h"
#include "console.h"

//#define CONSOLE_FOREGROUND_BLACK "\x1B[30m"
//#define CONSOLE_FOREGROUND_RED "\x1B[31m"
//#define CONSOLE_FOREGROUND_GREEN "\x1B[32m"
//#define CONSOLE_FOREGROUND_YELLOW "\x1B[33m"
//#define CONSOLE_FOREGROUND_BLUE "\x1B[34m"
//#define CONSOLE_FOREGROUND_MAGENTA "\x1B[35m"
//#define CONSOLE_FOREGROUND_CYAN "\x1B[36m"
//#define CONSOLE_FOREGROUND_WHITE "\x1B[37m"
//
//#define CONSOLE_FOREGROUND_BRIGHT_BRIGHT_BLACK "\x1B[90m"
//#define CONSOLE_FOREGROUND_BRIGHT_RED "\x1B[91m"
//#define CONSOLE_FOREGROUND_BRIGHT_GREEN "\x1B[92m"
//#define CONSOLE_FOREGROUND_BRIGHT_YELLOW "\x1B[93m"
//#define CONSOLE_FOREGROUND_BRIGHT_BLUE "\x1B[94m"
//#define CONSOLE_FOREGROUND_BRIGHT_MAGENTA "\x1B[95m"
//#define CONSOLE_FOREGROUND_BRIGHT_CYAN "\x1B[96m"
//#define CONSOLE_FOREGROUND_BRIGHT_WHITE "\x1B[97m"
//
//
//
//
//#define CONSOLE_BACKGROUND_BLACK "\033[3;40;30m"
//#define CONSOLE_BACKGROUND_RED   "\033[3;41;30m"
//#define CONSOLE_BACKGROUND_GREEN "\033[3;42;30m"
//#define CONSOLE_BACKGROUND_YELLOW "\033[3;43;30m"
//#define CONSOLE_BACKGROUND_BLUE  "\033[3;44;30m"
//#define CONSOLE_BACKGROUND_MAGENTA "\033[3;45;30m"
//#define CONSOLE_BACKGROUND_CYAN "\033[3;46;30m"
//#define CONSOLE_BACKGROUND_WHITE "\033[3;47;30m"
//
//#define CONSOLE_BACKGROUND_BRIGHT_BRIGHT_BLACK "\033[3;100;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_RED "\033[3;101;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_GREEN "\033[3;102;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_YELLOW "\033[3;103;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_BLUE "\033[3;104;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_MAGENTA  "\033[3;105;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_CYAN "\033[3;106;30m"
//#define CONSOLE_BACKGROUND_BRIGHT_WHITE "\033[3;107;30m"
/*
#define CONSOLE_FOREGROUND_BLACK(TEXT) 				"\x1B[30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_RED(TEXT) 				"\x1B[31m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_GREEN(TEXT) 				"\x1B[32m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_YELLOW(TEXT) 			"\x1B[33m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BLUE(TEXT) 				"\x1B[34m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_MAGENTA(TEXT) 			"\x1B[35m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_CYAN(TEXT) 				"\x1B[36m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_WHITE(TEXT)				"\x1B[37m" + std::string(TEXT) + "\033[0m"

#define CONSOLE_FOREGROUND_BRIGHT_BLACK(TEXT) 		"\x1B[90m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_RED(TEXT) 		"\x1B[91m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_GREEN(TEXT) 		"\x1B[92m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_YELLOW(TEXT) 		"\x1B[93m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_BLUE(TEXT) 		"\x1B[94m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_MAGENTA(TEXT)		"\x1B[95m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_CYAN(TEXT) 		"\x1B[96m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_FOREGROUND_BRIGHT_WHITE(TEXT) 		"\x1B[97m" + std::string(TEXT) + "\033[0m"

#define CONSOLE_BACKGROUND_BLACK(TEXT) 				"\033[3;40;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_RED(TEXT) 				"\033[3;41;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_GREEN(TEXT) 				"\033[3;42;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_YELLOW(TEXT) 			"\033[3;43;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BLUE(TEXT) 				"\033[3;44;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_MAGENTA(TEXT)			"\033[3;45;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_CYAN(TEXT) 				"\033[3;46;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_WHITE(TEXT) 				"\033[3;47;30m" + std::string(TEXT) + "\033[0m"

#define CONSOLE_BACKGROUND_BRIGHT_BLACK(TEXT) 		"\033[3;100;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_RED(TEXT)			"\033[3;101;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_GREEN(TEXT) 		"\033[3;102;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_YELLOW(TEXT) 		"\033[3;103;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_BLUE(TEXT) 		"\033[3;104;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_MAGENTA(TEXT) 	"\033[3;105;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_CYAN(TEXT) 		"\033[3;106;30m" + std::string(TEXT) + "\033[0m"
#define CONSOLE_BACKGROUND_BRIGHT_WHITE(TEXT) 		"\033[3;107;30m" + std::string(TEXT) + "\033[0m"

#define CONSOLE_BEGIN_FOREGROUND_BLACK				"\x1B[30m"
#define CONSOLE_BEGIN_FOREGROUND_RED				"\x1B[31m"
#define CONSOLE_BEGIN_FOREGROUND_GREEN				"\x1B[32m"
#define CONSOLE_BEGIN_FOREGROUND_YELLOW				"\x1B[33m"
#define CONSOLE_BEGIN_FOREGROUND_BLUE 				"\x1B[34m"
#define CONSOLE_BEGIN_FOREGROUND_MAGENTA			"\x1B[35m"
#define CONSOLE_BEGIN_FOREGROUND_CYAN				"\x1B[36m"
#define CONSOLE_BEGIN_FOREGROUND_WHITE				"\x1B[37m"

#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLACK		"\x1B[90m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_RED			"\x1B[91m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_GREEN		"\x1B[92m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_YELLOW		"\x1B[93m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_BLUE		"\x1B[94m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_MAGENTA		"\x1B[95m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_CYAN		"\x1B[96m"
#define CONSOLE_BEGIN_FOREGROUND_BRIGHT_WHITE		"\x1B[97m"

#define CONSOLE_BEGIN_BACKGROUND_BLACK				"\033[3;40;30m"
#define CONSOLE_BEGIN_BACKGROUND_RED				"\033[3;41;30m"
#define CONSOLE_BEGIN_BACKGROUND_GREEN				"\033[3;42;30m"
#define CONSOLE_BEGIN_BACKGROUND_YELLOW				"\033[3;43;30m"
#define CONSOLE_BEGIN_BACKGROUND_BLUE				"\033[3;44;30m"
#define CONSOLE_BEGIN_BACKGROUND_MAGENTA			"\033[3;45;30m"
#define CONSOLE_BEGIN_BACKGROUND_CYAN				"\033[3;46;30m"
#define CONSOLE_BEGIN_BACKGROUND_WHITE				"\033[3;47;30m"

#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLACK		"\033[3;100;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_RED			"\033[3;101;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_GREEN 		"\033[3;102;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_YELLOW		"\033[3;103;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_BLUE		"\033[3;104;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_MAGENTA		"\033[3;105;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_CYAN		"\033[3;106;30m"
#define CONSOLE_BEGIN_BACKGROUND_BRIGHT_WHITE		"\033[3;107;30m"

#define CONSOLE_RESET "\033[0m"
*/
/** Namespace shift */
namespace shift {
	namespace compiler {
		SHIFT_COMPILER_API shift_error_handler::shift_error_handler(void) noexcept {
		}

		SHIFT_COMPILER_API shift_error_handler::shift_error_handler(const shift_error_handler& other) : m_messages(other.m_messages), m_marks(
				other.m_marks), m_message_stream(other.m_message_stream.str()), m_werror(other.m_werror), m_warnings(other.m_warnings) {
		}
//
//		SHIFT_COMPILER_API shift_error_handler::shift_error_handler(shift_error_handler&& other) : m_messages(std::move(other.m_messages)), m_marks(
//				std::move(other.m_marks)) {
//		}

//		SHIFT_COMPILER_API shift_error_handler& shift_error_handler::operator=(shift_error_handler&& other) {
//			this->m_messages = std::move(other.m_messages);
//			this->m_message_stream = std::move(other.m_message_stream);
//			this->m_marks = std::move(other.m_marks);
//			return *this;
//		}
//
		SHIFT_COMPILER_API shift_error_handler& shift_error_handler::operator=(const shift_error_handler& other) {
			this->m_messages = other.m_messages;
			this->m_message_stream.str(other.m_message_stream.str());
			this->m_marks = other.m_marks;
			return *this;
		}

		SHIFT_COMPILER_API shift_error_handler& shift_error_handler::warn(const std::string& w) noexcept {
			if (!this->m_warnings)
				return *this;
			typedef std::pair<std::string, uint32_t> __pair_type;
			this->m_messages.push_back(__pair_type(w, this->m_werror ? SHIFT_ERROR_MESSAGE_TYPE : SHIFT_WARNING_MESSAGE_TYPE));
			return *this;
		}

		SHIFT_COMPILER_API shift_error_handler& shift_error_handler::error(const std::string& e) noexcept {
			typedef std::pair<std::string, uint32_t> __pair_type;
			this->m_messages.push_back(__pair_type(e, SHIFT_ERROR_MESSAGE_TYPE));
			return *this;
		}

		SHIFT_COMPILER_API std::ostringstream& shift_error_handler::stream(void) noexcept {
			return m_message_stream;
		}

		SHIFT_COMPILER_API void shift_error_handler::flush_stream(uint32_t type) noexcept {
			if (((type & SHIFT_WARNING_MESSAGE_TYPE) == SHIFT_WARNING_MESSAGE_TYPE) && !this->m_warnings) {
				m_message_stream.str("");
				return;
			}
			typedef std::pair<std::string, uint32_t> __pair_type;
			this->m_messages.push_back(__pair_type(m_message_stream.str(), this->m_werror ? (type | SHIFT_ERROR_MESSAGE_TYPE) : type));
			m_message_stream.str("");
		}

		SHIFT_COMPILER_API void shift_error_handler::print(void) {
//			HANDLE const hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//
//			constexpr int colorNormal = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
//			constexpr int colorWarning = FOREGROUND_RED | FOREGROUND_GREEN;
//			constexpr int colorError = FOREGROUND_RED;

			for (const auto& pair : this->m_messages) {
				if ((pair.second & SHIFT_ERROR_MESSAGE_TYPE) == SHIFT_ERROR_MESSAGE_TYPE) {
					if (shift::is_colored_console()) {
						//std::cerr << CONSOLE_FOREGROUND_BRIGHT_RED(pair.first);
						std::cerr << shift::lred << pair.first << shift::creset;
					} else {
						std::cerr << pair.first;
					}
				} else if ((pair.second & SHIFT_WARNING_MESSAGE_TYPE) == SHIFT_WARNING_MESSAGE_TYPE) {
					if (shift::is_colored_console()) {
						//std::cout << CONSOLE_FOREGROUND_BRIGHT_YELLOW(pair.first);
						std::cout << shift::lyellow << pair.first << shift::creset;
					} else {
						std::cout << pair.first;
					}
				} else {
					std::cout << pair.first;
				}
			}

			std::cerr.flush();
			std::cout.flush();

			this->m_messages.clear();
		}

		SHIFT_COMPILER_API void shift_error_handler::print_and_exit(void) {
			bool __exit = false;

			for (const auto& pair : this->m_messages) {
				if ((pair.second & SHIFT_ERROR_MESSAGE_TYPE) == SHIFT_ERROR_MESSAGE_TYPE) {
					__exit = true;
					if (shift::is_colored_console()) {
						//std::cerr << CONSOLE_FOREGROUND_BRIGHT_RED(pair.first);
						std::cerr << shift::lred << pair.first << shift::creset;
					} else {
						std::cerr << pair.first;
					}
				} else if ((pair.second & SHIFT_WARNING_MESSAGE_TYPE) == SHIFT_WARNING_MESSAGE_TYPE) {
					if (shift::is_colored_console()) {
						//std::cout << CONSOLE_FOREGROUND_BRIGHT_YELLOW(pair.first);
						std::cout << shift::lyellow << pair.first << shift::creset;
					} else {
						std::cout << pair.first;
					}
				} else {
					//if((pair.second & SHIFT_WARNING_MESSAGE_TYPE) == SHIFT_WARNING_MESSAGE_TYPE && !this->m_warnings)
					std::cout << pair.first;
				}
			}

			std::cerr.flush();
			std::cout.flush();

			this->m_messages.clear();

			if (__exit)
				shift::exit(1);
		}

		SHIFT_COMPILER_API size_t shift_error_handler::error_count(void) const {
			size_t ret = 0;
			for (const auto& pair : this->m_messages) {
				if ((pair.second & SHIFT_ERROR_MESSAGE_TYPE) == SHIFT_ERROR_MESSAGE_TYPE)
					ret++;
			}
			return ret;
		}

		SHIFT_COMPILER_API size_t shift_error_handler::warning_count(void) const {
			size_t ret = 0;
			for (const auto& pair : this->m_messages) {
				if ((pair.second & SHIFT_WARNING_MESSAGE_TYPE) == SHIFT_WARNING_MESSAGE_TYPE)
					ret++;
			}
			return ret;
		}

		SHIFT_COMPILER_API void shift_error_handler::set_werror(bool werror) {
			this->m_werror = werror;
		}

		SHIFT_COMPILER_API bool shift_error_handler::is_werror(void) const noexcept {
			return this->m_werror;
		}

		SHIFT_COMPILER_API void shift_error_handler::set_print_warnings(bool warnings) {
			this->m_warnings = warnings;
		}

		SHIFT_COMPILER_API bool shift_error_handler::is_print_warnings(void) const noexcept {
			return this->m_warnings;
		}

		SHIFT_COMPILER_API void shift_error_handler::mark(void) noexcept {
//			typedef std::pair<std::list<std::string>::size_type, std::list<std::string>::size_type> __pair_type;
//			this->m_marks.push(__pair_type(this->m_warnings.size(), this->m_errors.size()));

			this->m_marks.push(this->m_messages.size());
		}

		SHIFT_COMPILER_API void shift_error_handler::rollback(void) noexcept {
//			if (this->m_marks.empty())
//				return;
//			typedef std::pair<std::list<std::string>::size_type, std::list<std::string>::size_type> __pair_type;
//			const __pair_type& pair = this->m_marks.top();
//			this->m_set_mark(pair.first, pair.second);
//			this->m_marks.pop();

			if (this->m_marks.empty())
				return;
			this->m_set_mark(this->m_marks.top());
			this->m_marks.pop();
		}

		SHIFT_COMPILER_API void shift_error_handler::clear_marks(typename std::list<std::pair<std::string, uint32_t>>::size_type __count)
				noexcept {

			if (typename std::stack<typename std::list<std::string>::size_type>::size_type(__count) > this->m_marks.size()) {
				__count = this->m_marks.size();
			}

//			if (__count > this->m_marks.size())
//				__count = this->m_marks.size();

			while (__count != 0) {
				this->m_marks.pop();
				__count--;
			}
		}

		void shift_error_handler::m_set_mark(typename std::list<std::string>::size_type we) noexcept {
			if (we < this->m_messages.size()) {
				this->m_messages.resize(we);
			}
//			if (w < this->m_warnings.size()) {
//				this->m_warnings.resize(w);
//			}
//
//			if (e < this->m_errors.size()) {
//				this->m_errors.resize(e);
//			}
		}

		SHIFT_COMPILER_API std::stack<std::list<std::pair<std::string, uint32_t>>::size_type>& shift_error_handler::get_marks(void) noexcept {
			return this->m_marks;
		}

		SHIFT_COMPILER_API const std::stack<std::list<std::string>::size_type>& shift_error_handler::get_marks(void) const noexcept {
			return this->m_marks;
		}

		SHIFT_COMPILER_API std::list<std::pair<std::string, uint32_t>>& shift_error_handler::get_messages(void) noexcept {
			return this->m_messages;
		}

		SHIFT_COMPILER_API const std::list<std::pair<std::string, uint32_t>>& shift_error_handler::get_messages(void) const noexcept {
			return this->m_messages;
		}

	}
}
