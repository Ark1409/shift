/**
 * @file include/argument_parser.h
 */

#ifndef SHIFT_ARGUMENT_PARSER_H_
#define SHIFT_ARGUMENT_PARSER_H_

#include "../shift_config.h"
#include "shift_error_handler.h"
#include "../io/directory.h"
#include "../io/file.h"

#include <string>
#include <string_view>
#include <vector>
#include <list>

// Command-line flag utility
#define SHIFT_FLAG_PREFIX "-"
#define SHIFT_FLAG(__FLAG__) (SHIFT_FLAG_PREFIX __FLAG__)

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
		class SHIFT_COMPILER_API argument_parser {
			public:
				typedef uint8_t flags_t; /// Type representing a set of compiler flags

				/**
				 * Enum representing the flags which were passed to the compiler.
				 */
				enum : flags_t {
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
					FLAG_CPP_OUTPUT = 0x4,/**< FLAG_CPP_OUTPUT */

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
			protected:
				/// The array of command-line arguments
				std::vector<std::string_view> m_args;

				/// Sources files, library paths, and library files
				std::list<io::file> m_compile_files, m_libraries;
				std::list<io::directory> m_library_paths;

				/// Compiler flags set by the usre
				flags_t m_flags = FLAG_NO_FLAGS;

				/// Used by the argument parser for error handling
				shift::compiler::shift_error_handler* m_error_handler;

				/// Whether or not to delete (heap) the error handler after the class is destroyed
				bool m_del_error_handler = false;
			public:
				/// Constructs argument parser from command-line argc and argv
				argument_parser(const size_t argc, const char*const* const argv, shift::compiler::shift_error_handler* const error_handler = nullptr);

				/// Constructs argument parser from array of arguments
				argument_parser(const std::vector<std::string_view>& args, shift::compiler::shift_error_handler* const error_handler = nullptr);

				/// Constructs argument parser from array of arguments
				argument_parser(std::vector<std::string_view>&& args, shift::compiler::shift_error_handler* const error_handler = nullptr);

				/// Copy constructor
				argument_parser(const argument_parser&);

				/// Move constructor
				argument_parser(argument_parser&&);

				/// Destructor
				~argument_parser(void) noexcept;

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

				/**
				 * Transforms the arguments into a single string, separating each argument with a space.
				 * @return The argument list as a std::string.
				 */
				std::string to_string(void) const;

				argument_parser& operator=(const argument_parser&);
				argument_parser& operator=(argument_parser&&);

				inline bool operator==(const std::string& str) const {
					return to_string() == str;
				}

				inline bool operator==(const std::string_view str) const {
					return to_string() == str;
				}

				inline bool operator==(const char* const str) const {
					return to_string() == str;
				}

				inline std::string_view operator[](const typename std::vector<std::string_view>::size_type index) const {
					return get_argument(index);
				}

				inline std::vector<std::string_view>& get_arguments(void) noexcept {
					return this->m_args;
				}

				inline const std::vector<std::string_view>& get_arguments(void) const noexcept {
					return this->m_args;
				}

				inline size_t size(void) const noexcept {
					return this->m_args.size();
				}

				inline flags_t get_flags(void) const noexcept {
					return this->m_flags;
				}

				inline std::list<io::file>& get_source_files(void) noexcept {
					return this->m_compile_files;
				}

				inline std::list<io::directory>& get_library_paths(void) noexcept {
					return this->m_library_paths;
				}

				inline std::list<io::file>& get_libraries(void) noexcept {
					return this->m_libraries;
				}

				inline const std::list<io::file>& get_source_files(void) const noexcept {
					return this->m_compile_files;
				}

				inline const std::list<io::directory>& get_library_paths(void) const noexcept {
					return this->m_library_paths;
				}

				inline const std::list<io::file>& get_libraries(void) const noexcept {
					return this->m_libraries;
				}

				inline bool operator!=(const std::string& string) const {
					return !operator==(string);
				}

				inline bool operator!=(const char* const string) const {
					return !operator==(string);
				}

				inline bool operator!=(const std::string_view string) const {
					return !operator==(string);
				}

				inline bool is_warnings(void) const noexcept {
					return this->has_flag(FLAG_WARNINGS);
				}

				inline bool is_werrors(void) const noexcept {
					return this->has_flag(FLAG_WERROR);
				}

				inline bool is_cpp_out(void) const noexcept {
					return this->has_flag(FLAG_CPP_OUTPUT);
				}

				inline bool is_help(void) const noexcept {
					return this->has_flag(FLAG_HELP);
				}

				inline bool has_flag(const flags_t flag) const noexcept {
					return (this->m_flags & flag) == flag;
				}
			private:
				void resolve_libraries_and_sources(void);
		};
	}
}

SHIFT_COMPILER_API inline std::ostream& operator<<(std::ostream& os, const shift::compiler::argument_parser& parser) {
	return os << parser.to_string();
}

#endif /* SHIFT_ARGUMENT_PARSER_H_ */
