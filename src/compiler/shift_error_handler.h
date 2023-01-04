/**
 * @file include/shift_error_handler.h
 */

#ifndef SHIFT_ERROR_HANDLER_H_
#define SHIFT_ERROR_HANDLER_H_

#include "shift_config.h"

#include <list>
#include <stack>
#include <string>
#include <sstream>

#define SHIFT_ERROR_MESSAGE_TYPE 	(0x1)
#define SHIFT_WARNING_MESSAGE_TYPE 	(0x2)


/** Namespace shift */
namespace shift {
	namespace compiler {

		class SHIFT_COMPILER_API shift_error_handler {
			private:
//				 std::list<std::string> m_warnings, m_errors;
//				 std::stack<std::pair<std::list<std::string>::size_type, std::list<std::string>::size_type>> m_marks;

				std::list<std::pair<std::string, uint32_t>> m_messages;
				std::stack<typename std::list<std::pair<std::string, uint32_t>>::size_type> m_marks;
				std::ostringstream m_message_stream;
				bool m_werror = false;
				bool m_warnings = false;
			public:
				 shift_error_handler(void) noexcept;
				 shift_error_handler(const shift_error_handler&);
				 shift_error_handler(shift_error_handler&&) = default;

				 virtual ~shift_error_handler(void) noexcept = default;

				 shift_error_handler& operator=(const shift_error_handler&);
				 shift_error_handler& operator=(shift_error_handler&&) = default;

				 shift_error_handler& warn(const std::string&) noexcept;
				 shift_error_handler& error(const std::string&) noexcept;

				 std::ostringstream& stream(void) noexcept; // Convenient stream for writing warnings and errors
				 void flush_stream(uint32_t type) noexcept; // Flushes the std::stringstream used to write warnings/errors,

				 void print(void); // Print messages
				 void print_and_exit(void); // Print messages and only EXITS IF ERRORS WERE FOUND

				 size_t error_count(void) const;
				 size_t warning_count(void) const;

				 void set_werror(bool werror = true);
				 bool is_werror(void) const noexcept;

				 void set_print_warnings(bool warnings = true);
				 bool is_print_warnings(void) const noexcept;

				/**
				 * Adds a mark to the current list of warnings and errors. Calling this method multiple times
				 * will not replace previous marks. Instead, they are added into a stack, with the most
				 * recent mark being used when rolling back.
				 *
				 * @see rollback()
				 */
				 void mark(void) noexcept; // Mark current warnings and errors

				/**
				 * Rolls back to the most recent mark, popping it off the stack to remove it from further use.
				 */
				 void rollback(void) noexcept; // Rollback to last mark and pop mark off stack

				 void clear_marks(typename std::list<std::pair<std::string, uint32_t>>::size_type count = -1) noexcept;

				 std::stack<typename std::list<std::pair<std::string, uint32_t>>::size_type>& get_marks(void) noexcept;

				 const std::stack<std::list<std::string>::size_type>& get_marks(void) const noexcept;

				 std::list<std::pair<std::string, uint32_t>>& get_messages(void) noexcept;
				 const std::list<std::pair<std::string, uint32_t>>& get_messages(void) const noexcept;
			private:
				void m_set_mark(typename std::list<std::string>::size_type) noexcept; // Sets mark to the two values
		};

	}
}

#endif /* SHIFT_ERROR_HANDLER_H_ */
