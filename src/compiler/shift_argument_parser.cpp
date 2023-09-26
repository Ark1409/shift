/**
 * @file compiler/shift_argument_parser.cpp
 */
#include "shift_argument_parser.h"
#include "utils/utils.h"

#include <stdexcept>
#include <cstring>
#include <algorithm>

#define SHIFT_WARNING_PREFIX 			"warning: "
#define SHIFT_ERROR_PREFIX 				"error: "	

#define SHIFT_PRINT() if(this->m_error_handler) this->m_error_handler->print_clear()
#define SHIFT_FATAL() if(this->m_error_handler) this->m_error_handler->print_exit_clear()

#define SHIFT_WARNING(__WARN__) 		if(this->m_error_handler) this->m_error_handler->stream() << SHIFT_WARNING_PREFIX << __WARN__ << std::endl, this->m_error_handler->flush_stream(error_handler::message_type::warning)
#define SHIFT_WARNING_LOG(__WARN__) 	if(this->m_error_handler) this->m_error_handler->stream() << __WARN__ << std::endl, this->m_error_handler->flush_stream(error_handler::message_type::warning)

#define SHIFT_ERROR(__ERR__) 			if(this->m_error_handler) this->m_error_handler->stream() << SHIFT_ERROR_PREFIX << __ERR__ << std::endl, this->m_error_handler->flush_stream(error_handler::message_type::error)
#define SHIFT_ERROR_LOG(__ERR__) 		if(this->m_error_handler) this->m_error_handler->stream() << __ERR__ << std::endl, this->m_error_handler->flush_stream(error_handler::message_type::error)
#define SHIFT_FATAL_ERROR(__ERR__) 		SHIFT_ERROR(__ERR__); SHIFT_FATAL()
#define SHIFT_FATAL_ERROR_LOG(__ERR__)  SHIFT_ERROR_LOG(__ERR__); SHIFT_FATAL()

namespace shift::compiler {
	SHIFT_API argument_parser::argument_parser(error_handler* const error_handler, const size_t argc, const char* const* const argv)
		noexcept : m_error_handler(error_handler) {
		if (!argv) return;
		m_args.reserve(argc);
		for (size_t i = 0; i < argc; i++) {
			m_args.push_back(argv[i]);
		}
	}

	SHIFT_API bool argument_parser::contains_argument(const std::string_view other) const noexcept {
		return std::find(this->m_args.cbegin(), this->m_args.cend(), other) != this->m_args.cend();
	}

	SHIFT_API std::string_view argument_parser::get_argument(typename std::vector<std::string_view>::size_type id) const {
		if (id >= m_args.size())
			throw std::out_of_range(std::to_string(id) + " is not in range [0," + std::to_string(m_args.size()) + ")");
		return m_args[id];
	}

	SHIFT_API std::string argument_parser::to_string(void) const {
		std::string ret;

		for (size_t i = 0; i < this->m_args.size(); i++) {
			if (i > 0) ret += ' ';
			ret += this->m_args[i];
		}
		return ret;
	}

	SHIFT_API void argument_parser::parse(void) {
		size_t off = 0; // offset needed to print error messages properly

		for (size_t i = 0; i < this->m_args.size(); i++) {
			const std::string_view arg = this->m_args[i];

			if (arg == SHIFT_FLAG_HELP) {
				// The user requested the help page
				// We are not exiting the for loop here because we are allowing the user to specify other arguments with the help page
				// The help page will be printed before anything is compiled.
				this->m_flags |= FLAG_HELP;
			} else if (arg == SHIFT_FLAG_WARNING) {
				// The user requested for warnings to be enabled
				this->m_flags |= FLAG_WARNINGS;
				if (this->m_error_handler)
					this->m_error_handler->set_print_warnings(true);
			} else if (arg == SHIFT_FLAG_WERROR) {
				// The user requested for warnings to be treated as errors
				this->m_flags |= FLAG_WERROR;
				if (this->m_error_handler) {
					this->m_error_handler->set_print_warnings(true);
					this->m_error_handler->set_werror(true);
				}
			} else if (arg == SHIFT_FLAG_CPP_OUTPUT || arg == SHIFT_FLAG_CPP_OUTPUT_2) {
				// The user requested for a .cpp file output
				this->m_flags |= FLAG_CPP_OUTPUT;
			} else if (arg == SHIFT_FLAG_NO_STD_LIB) {
				// The user requested to not link against the Shift standard library
				this->m_flags |= FLAG_NO_STD;
			} else if (arg == SHIFT_FLAG_LIB_PATH) {
				if ((i + 1) >= this->m_args.size()) {
					if (this->m_error_handler) {
						SHIFT_ERROR("Expected library path after flag " << SHIFT_FLAG_LIB_PATH << " (parameter " << (i + 1) << ")");
					}
					std::string out;
					out.reserve(off + arg.size());
					out.append(off, ' ');
					out.append(arg.size(), '^');

					if (this->m_error_handler) {
						SHIFT_ERROR_LOG(this->to_string());
						SHIFT_ERROR_LOG(out);
					}
				} else {
					off += arg.length() + 1; // + 1 to account for space character when printing out
					this->m_library_paths.push_back(filesystem::directory(this->m_args[++i]));
				}
			} else if (arg == SHIFT_FLAG_LIB) {
				if ((i + 1) >= this->m_args.size()) {
					if (this->m_error_handler) {
						SHIFT_ERROR("Expected library after flag " << SHIFT_FLAG_LIB << " (parameter " << (i + 1) << ")");
					}

					std::string out;
					out.reserve(off + arg.size());
					out.append(off, ' ');
					out.append(arg.size(), '^');

					if (this->m_error_handler) {
						SHIFT_ERROR_LOG(this->to_string());
						SHIFT_ERROR_LOG(out);
					}
				} else {
					off += arg.length() + 1; // + 1 to account for space character when printing out
					this->m_libraries.push_back(filesystem::file(this->m_args[++i]));
				}
			} else if (utils::starts_with(arg, std::string_view(SHIFT_FLAG_PREFIX))) {
				// error, unknown setting
				std::string out;
				out.reserve(off + arg.size());
				out.append(off, ' ');
				out.append(arg.size(), '^');

				if (this->m_error_handler) {
					SHIFT_WARNING("Ignored parameter: " << arg << " (parameter " << (i + 1) << ")");
					SHIFT_WARNING_LOG(this->to_string());
					SHIFT_WARNING_LOG(out);
				}
			} else {
				// if it doesn't start with a SHIFT_FLAG_PREFIX, we assume that they are entering a source file
				this->m_compile_files.push_back(filesystem::file(arg));
			}

			off += arg.length() + 1; // + 1 to account for space character when printing out
		}

		if (this->m_compile_files.size() <= 0) {
			// If there are no source files to compile, help page will be displayed since there is no work to do.
			this->m_flags |= FLAG_HELP;
		}

		// Ensures library and source files exist
		return this->resolve_libraries_and_sources();
	}

	void argument_parser::resolve_libraries_and_sources(void) {
		{ // Remove inexistent library paths
			for (auto lib_path = this->get_library_paths().cbegin(); lib_path != this->get_library_paths().cend(); ++lib_path) {
				if (!(*lib_path)) {
					// If the library directory does not exist, issue a warning
					if (this->m_error_handler) {
						SHIFT_WARNING("Ignored inexistent library path: " << lib_path->raw_path());
					}

					// Remove the inexistent library directory
					lib_path = --this->get_library_paths().erase(lib_path);
				}
			}
		}

		{ // Remove inexistent libraries
			for (auto lib_file = this->get_libraries().begin(); lib_file != this->get_libraries().end(); ++lib_file) {
				if (!(*lib_file)) {
					// If it is not in current directory, checks directories from inputed library paths
					for (const filesystem::directory& lib_path : this->get_library_paths()) {
						filesystem::file new_file = lib_path / *lib_file;

						if (new_file) {
							// If the file exists within this directory, use this one as the library file
							// This only finds the first occurence. We may want to add a warning if more than one file with this name exists
							// TODO Keep a list of library candidates, and then either error or warn the user which one you chose.
							// or maybe compile both, and use functions specified from both as needed
							*lib_file = std::move(new_file);
							break;
						}
					}
				}

				if (!(*lib_file)) {
					// If the file was still not founded after checking list of library directories, issue a warning
					if (this->m_error_handler) {
						SHIFT_WARNING("Ignored inexistent library: " << lib_file->get_path());
					}
					// Remove the inexistent library
					lib_file = --this->get_libraries().erase(lib_file);
				}
			}
		}

		{ // Remove inexistent source files
			for (auto source_file = this->get_source_files().cbegin(); source_file != this->get_source_files().cend(); ++source_file) {
				std::string::size_type star_index;
				const std::string str = source_file->get_path();

				if ((star_index = str.find('*')) != std::string::npos) {
					// TODO allow user to enter "mycode/*.shift" to compile all files in a directory.
				}

				if (!(*source_file)) {
					if (this->m_error_handler) {
						SHIFT_WARNING("Ignored inexistent source file: " << str);
					}
					source_file = --this->get_source_files().erase(source_file);
				}
			}
		}
	}

}