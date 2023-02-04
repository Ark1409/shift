/**
 * @file compiler/argument_parser.h
 */
#ifndef SHIFT_ARGUMENT_PARSER_H_
#define SHIFT_ARGUMENT_PARSER_H_ 1

#include "shift_config.h"
#include "compiler/shift_error_handler.h"
#include "filesystem/directory.h"
#include "filesystem/file.h"

#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <memory>

 // Command-line flag utility
#define SHIFT_FLAG_PREFIX "-"
#define SHIFT_FLAG(FLAG) (SHIFT_FLAG_PREFIX FLAG)

/// Command-line flags
#define SHIFT_FLAG_WARNING 				SHIFT_FLAG("warnings")
#define SHIFT_FLAG_WERROR 				SHIFT_FLAG("warnings-as-errors")
#define SHIFT_FLAG_CPP_OUTPUT 			SHIFT_FLAG("cpp")
#define SHIFT_FLAG_CPP_OUTPUT_2 		SHIFT_FLAG("c++") // same as SHIFT_FLAG_CPP_OUTPUT
#define SHIFT_FLAG_LIB_PATH 			SHIFT_FLAG("lib-path")
#define SHIFT_FLAG_LIB 					SHIFT_FLAG("lib")
#define SHIFT_FLAG_HELP					SHIFT_FLAG("help")
#define SHIFT_FLAG_NO_STD_LIB 			SHIFT_FLAG("no-std") // Not yet implemented

namespace shift {
	namespace compiler {
		class argument_parser {
		public:
			/**
			 * Enum representing the flags which can be passed to the compiler.
			 */
			enum flags {
				/**
				 * Allows the error handler to report warnings.
				 */
				FLAG_WARNINGS = 0x1, /**< FLAG_WARNINGS */

				/**
				 * Tells the error handler to treat warnings as errors.
				 * Implicitly enables @a FLAG_WARNINGS.
				 */
				 FLAG_WERROR = FLAG_WARNINGS | 0x2, /**< FLAG_WERROR */

				 /**
				  * Tells the compiler to output a .cpp file which can then be compiled to the final executable.
				  */
				  FLAG_CPP_OUTPUT = 0x4, /**< FLAG_CPP_OUTPUT */

				  /**
				   * Tells the compiler to avoid linking the Shift standard library.
				   * This flag is not currently supported.
				   */
				   FLAG_NO_STD = 0x8, /**< FLAG_NO_STD */

				   /**
					* Indicates that the user requested to also print the compiler's help page.
					* If this flag is used in conjunction with others, the compiler will print out the help page then finish its other tasks before closing.
					* If this flag is used by itself, the compiler will print out the help page and immediately close.
					*/
					FLAG_HELP = 0x10, /**< FLAG_HELP */

					/**
					 * Indicates a value of no flags.
					 * This is usually never used, as FLAG_HELP is used whenever a user passes in no parameters.
					 * This flag can be used to prevent the help page from being printed out.
					 */
					 FLAG_NO_FLAGS = 0x0 /**< FLAG_NO_FLAGS */
			};
		public:
			/// Constructs argument parser from command-line argc and argv
			argument_parser(error_handler* const, const size_t argc = 0, const char* const* const argv = nullptr) noexcept;

			/// Constructs argument parser from array of arguments
			inline argument_parser(error_handler* const, const std::vector<std::string_view>& args) noexcept;

			/// Constructs argument parser from array of arguments
			inline argument_parser(error_handler* const, std::vector<std::string_view>&& args) noexcept;

			/// Copy constructor
			argument_parser(const argument_parser&) noexcept = default;

			/// Move constructor
			argument_parser(argument_parser&&) noexcept = default;

			/// Destructor
			~argument_parser() noexcept = default;

			/**
			 * Parses the given command-line arguments.
			 */
			void parse(void);

			/**
			 * Retrieves the argument at the given index
			 * @param[in] index The index of the argument
			 * @return A std::string_view representing the argument
			 */
			std::string_view get_argument(typename std::vector<std::string_view>::size_type index) const;

			/**
			 * Checks whether the argument below is within the argument list.
			 * An argument is said to exist within the list if there is an argument exactly equal to it or if there is an argument equal to it without the SHIFT_FLAG_PREFIX.
			 * i.e. the argument list "-warnings -lib-path libs" contains "-warnings" and "warnings", but not "-warning".
			 *
			 * @param[in] arg The argument to check against
			 * @return True if the argument exist within the argument list, false otherwise.
			 */
			bool contains_argument(const std::string_view arg) const noexcept;

			inline bool contains_argument(const std::string& arg) const noexcept { return contains_argument(std::string_view(arg.data(), arg.length())); }

			/**
			 * Transforms the arguments into a single string, separating each argument with a space.
			 * @return The argument list as a std::string.
			 */
			std::string to_string(void) const;

			argument_parser& operator=(const argument_parser&) noexcept = default;
			argument_parser& operator=(argument_parser&&) noexcept = default;
			inline bool operator==(const std::string& str) const { return to_string() == str; }
			inline bool operator==(const std::string_view str) const { return to_string() == str; }
			inline bool operator!=(const std::string& str) const { return !operator==(str); }
			inline bool operator!=(const std::string_view str) const { return !operator==(str); }

			inline std::string_view operator[](const typename std::vector<std::string_view>::size_type index) const { return get_argument(index); }
			inline bool operator[](flags const flag) const noexcept { return this->has_flag(flag); }
			inline std::vector<std::string_view>& get_arguments(void) noexcept { return this->m_args; }
			inline const std::vector<std::string_view>& get_arguments(void) const noexcept { return this->m_args; }
			inline size_t get_size(void) const noexcept { return this->m_args.size(); }
			inline flags get_flags(void) const noexcept { return this->m_flags; }
			inline void set_flags(flags const flags_) noexcept { this->m_flags = flags_; }
			inline std::list<filesystem::file>& get_source_files(void) noexcept { return this->m_compile_files; }
			inline std::list<filesystem::directory>& get_library_paths(void) noexcept { return this->m_library_paths; }
			inline std::list<filesystem::file>& get_libraries(void) noexcept { return this->m_libraries; }
			inline const std::list<filesystem::file>& get_source_files(void) const noexcept { return this->m_compile_files; }
			inline const std::list<filesystem::directory>& get_library_paths(void) const noexcept { return this->m_library_paths; }
			inline const std::list<filesystem::file>& get_libraries(void) const noexcept { return this->m_libraries; }

			inline bool is_warnings(void) const noexcept { return this->has_flag(FLAG_WARNINGS); }
			inline bool is_werrors(void) const noexcept { return this->has_flag(FLAG_WERROR); }
			inline bool is_cpp_out(void) const noexcept { return this->has_flag(FLAG_CPP_OUTPUT); }
			inline bool is_help(void) const noexcept { return this->has_flag(FLAG_HELP); }
			inline bool is_no_std(void) const noexcept { return this->has_flag(FLAG_NO_STD); }
			inline bool has_flag(flags const flag) const noexcept { return (this->m_flags & flag) == flag; }

			inline error_handler* get_error_handler() noexcept { return m_error_handler; }
			inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }
			inline void set_error_handler(error_handler* const error_handler) noexcept { m_error_handler = error_handler; }
		protected:
			/// Used by the argument parser for error handling
			error_handler* m_error_handler = nullptr;

			/// Compiler flags set by the user
			flags m_flags = FLAG_NO_FLAGS;

			/// The array of command-line arguments
			std::vector<std::string_view> m_args;

			/// Sources files, library paths, and library files
			std::list<filesystem::file> m_compile_files, m_libraries;
			std::list<filesystem::directory> m_library_paths;
		private:
			void resolve_libraries_and_sources(void);
		};

		inline argument_parser::argument_parser(error_handler* const error_handler, const std::vector<std::string_view>& args) noexcept
			: m_error_handler(error_handler), m_args(args) {}

		inline argument_parser::argument_parser(error_handler* const error_handler, std::vector<std::string_view>&& args) noexcept
			: m_error_handler(error_handler), m_args(std::move(args)) {}
	}
}

inline std::ostream& operator<<(std::ostream& os, const shift::compiler::argument_parser& parser) { return os << parser.to_string(); }
constexpr inline shift::compiler::argument_parser::flags operator^(const shift::compiler::argument_parser::flags f, const shift::compiler::argument_parser::flags other) noexcept { return shift::compiler::argument_parser::flags(std::underlying_type_t<shift::compiler::argument_parser::flags>(f) ^ std::underlying_type_t<shift::compiler::argument_parser::flags>(other)); }
constexpr inline shift::compiler::argument_parser::flags& operator^=(shift::compiler::argument_parser::flags& f, const shift::compiler::argument_parser::flags other) noexcept { return f = operator^(f, other); }
constexpr inline shift::compiler::argument_parser::flags operator|(const shift::compiler::argument_parser::flags f, const shift::compiler::argument_parser::flags other) noexcept { return shift::compiler::argument_parser::flags(std::underlying_type_t<shift::compiler::argument_parser::flags>(f) | std::underlying_type_t<shift::compiler::argument_parser::flags>(other)); }
constexpr inline shift::compiler::argument_parser::flags& operator|=(shift::compiler::argument_parser::flags& f, const shift::compiler::argument_parser::flags other) noexcept { return f = operator|(f, other); }
constexpr inline shift::compiler::argument_parser::flags operator&(const shift::compiler::argument_parser::flags f, const shift::compiler::argument_parser::flags other) noexcept { return shift::compiler::argument_parser::flags(std::underlying_type_t<shift::compiler::argument_parser::flags>(f) & std::underlying_type_t<shift::compiler::argument_parser::flags>(other)); }
constexpr inline shift::compiler::argument_parser::flags& operator&=(shift::compiler::argument_parser::flags& f, const shift::compiler::argument_parser::flags other) noexcept { return f = operator&(f, other); }
constexpr inline shift::compiler::argument_parser::flags operator~(const shift::compiler::argument_parser::flags f) noexcept { return shift::compiler::argument_parser::flags(~std::underlying_type_t<shift::compiler::argument_parser::flags>(f)); }

#endif /* SHIFT_ARGUMENT_PARSER_H_ */
