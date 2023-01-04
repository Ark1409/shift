/**
 * @file argument_parser.cpp
 */

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include "argument_parser.h"
#include "io/file.h"
#include "stdutils.h"

#define SHIFT_WARNING_PREFIX 			"warning: "
#define SHIFT_ERROR_PREFIX 				"error: "

#define SHIFT_LOG(__LOG__) 				this->m_error_handler->stream() << __LOG__ << std::endl; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)
#define SHIFT_WARNING_LOG(__WARN__) 	SHIFT_LOG(__WARN__)

#define SHIFT_ERROR_LOG(__ERR__) 		this->m_error_handler->stream() << __ERR__ << std::endl; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_FATAL_ERROR_LOG(__ERR__)  SHIFT_ERROR_LOG(__ERR__); this->m_error_handler->print_and_exit()

#define SHIFT_WARNING(__WARN__) 		this->m_error_handler->stream() << SHIFT_WARNING_PREFIX << __WARN__ << std::endl; this->m_error_handler->flush_stream(SHIFT_WARNING_MESSAGE_TYPE)

#define SHIFT_ERROR(__ERR__) 			this->m_error_handler->stream() << SHIFT_ERROR_PREFIX << __ERR__ << std::endl; this->m_error_handler->flush_stream(SHIFT_ERROR_MESSAGE_TYPE)
#define SHIFT_FATAL_ERROR(__ERR__) 		SHIFT_ERROR(__ERR__); this->m_error_handler->print_and_exit()

#define SHIFT_FATAL() this->m_error_handler->print_and_exit()

#define SHIFT_PRINT() SHIFT_FATAL()

namespace shift {
	namespace compiler {
		SHIFT_COMPILER_API argument_parser::argument_parser(const size_t argc, const char*const* const argv,
				 shift::compiler::shift_error_handler* const error_handler) : m_error_handler(
				error_handler ? error_handler : new shift_error_handler()), m_del_error_handler(!error_handler) {
			m_args.reserve(argc);
			for (size_t i = 0; i < argc; i++) {
				m_args.push_back(argv[i]);
			}
		}

		SHIFT_COMPILER_API argument_parser::argument_parser(const std::vector<std::string_view>& args,
				 shift::compiler::shift_error_handler* const error_handler) : m_args(args), m_error_handler(
				error_handler ? error_handler : new shift_error_handler()), m_del_error_handler(!error_handler) {

		}

		SHIFT_COMPILER_API argument_parser::argument_parser(std::vector<std::string_view>&& args,
				 shift::compiler::shift_error_handler* const error_handler) : m_args(std::move(args)), m_error_handler(
				error_handler ? error_handler : new shift_error_handler()), m_del_error_handler(!error_handler) {
		}

		SHIFT_COMPILER_API argument_parser::argument_parser(const argument_parser& other) : m_args(other.m_args), m_error_handler(
				m_del_error_handler ? new shift_error_handler(*other.m_error_handler) : other.m_error_handler), m_del_error_handler(
				other.m_del_error_handler) {
		}

		SHIFT_COMPILER_API argument_parser::argument_parser(argument_parser&& other) : m_args(std::move(other.m_args)), m_error_handler(
				other.m_error_handler), m_del_error_handler(other.m_del_error_handler) {
			other.m_error_handler = nullptr;
			other.m_del_error_handler = false;
		}

		SHIFT_COMPILER_API argument_parser& argument_parser::operator=(const argument_parser& other) {
			this->m_args = other.m_args;
			this->m_error_handler = m_del_error_handler ? new shift_error_handler(*other.m_error_handler) : other.m_error_handler;
			this->m_del_error_handler = other.m_del_error_handler;
			return *this;
		}

		SHIFT_COMPILER_API argument_parser& argument_parser::operator=(argument_parser&& other) {
			this->m_args = std::move(other.m_args);
			this->m_error_handler = other.m_error_handler;
			this->m_del_error_handler = other.m_del_error_handler;
			other.m_error_handler = nullptr;
			other.m_del_error_handler = false;
			return *this;
		}

		SHIFT_COMPILER_API argument_parser::~argument_parser(void) noexcept {
			if (this->m_del_error_handler)
				delete m_error_handler;
		}

		SHIFT_COMPILER_API bool argument_parser::contains_argument(const std::string_view other) const noexcept {
			const size_t prefix_len = ::strlen(SHIFT_FLAG_PREFIX);
			for (typename std::vector<std::string_view>::size_type i = 0; i < m_args.size(); i++)
				if (m_args[i] == other
						|| ((other.size() > prefix_len) && (other.size() - prefix_len == m_args[i].size())
								&& (string_starts_with(other, SHIFT_FLAG_PREFIX))
								&& (std::memcmp(m_args[i].data(), &other[prefix_len], other.size() - prefix_len) == 0)))
					return true;
			return false;
		}

		SHIFT_COMPILER_API std::string_view argument_parser::get_argument(typename std::vector<std::string_view>::size_type id) const {
			if (id < 0 || id >= m_args.size())
				throw std::out_of_range("Argument ID must be: 0 < " + std::to_string(id) + " < " + std::to_string(m_args.size()));
			return m_args[id];
		}

		SHIFT_COMPILER_API std::string argument_parser::to_string(void) const {
			std::string ret;
			{
				typename std::string::size_type res_size = 0;
				std::for_each(this->m_args.begin(), this->m_args.end(), [&res_size](const std::string_view v) {
					res_size += v.size();
				});

				ret.reserve(res_size + (this->m_args.size() ? this->m_args.size() - 1 : 0));
			}

			for (size_t i = 0; i < this->m_args.size(); i++) {
				if (i > 0)
					ret += ' ';
				ret += this->m_args[i];
			}
			return ret;
		}

		SHIFT_COMPILER_API void argument_parser::parse(void) {
			size_t off = 0; // offset needed to print error messages properly

			for (size_t i = 0; i < this->m_args.size(); i++) {
				const std::string_view arg = this->m_args[i];

				if (arg == SHIFT_FLAG_HELP) {
					// The user requested the help page
					// We are not exiting the for loop here because we are allowing the user to specify other arguments with the help page
					// The help page will just be printed before anything is compiled.
					this->m_flags |= FLAG_HELP;
				} else if (arg == SHIFT_FLAG_WARNING) {
					// The user requested for warnings to be enabled
					this->m_flags |= FLAG_WARNINGS;
					this->m_error_handler->set_print_warnings(true);
				} else if (arg == SHIFT_FLAG_WERROR) {
					// The user requested for warnings to be treated as errors
					this->m_flags |= FLAG_WERROR;
					this->m_error_handler->set_werror(true);
				} else if (arg == SHIFT_FLAG_CPP_OUTPUT || arg == SHIFT_FLAG_CPP_OUTPUT_2) {
					// The user requested for a .cpp file output
					this->m_flags |= FLAG_CPP_OUTPUT;
				} else if (arg == SHIFT_FLAG_NO_STD_LIB) {
					// The user requested to not link against the Shift standard library
					this->m_flags |= FLAG_NO_STD;
				} else if (arg == SHIFT_FLAG_LIB_PATH) {
					if ((i + 1) >= this->m_args.size()) {
						SHIFT_ERROR("Expected library path after flag " << SHIFT_FLAG_LIB_PATH << " (parameter " << (i+1) << ")");
						std::string out;
						out.reserve(arg.size() + off);
						if (off > 0) {
							for (size_t moff = 0; moff < off; moff++) {
								out += ' ';
							}
						}
						for (size_t show_len = 0; show_len < arg.size(); show_len++)
							out += '^';
						SHIFT_ERROR_LOG(this->to_string());
						SHIFT_FATAL_ERROR_LOG(out);
					}
					const std::string_view next_arg = this->m_args[i + 1];

					if (string_starts_with(next_arg, SHIFT_FLAG_PREFIX)) {
						std::string out;
						out.reserve(arg.size() + off);
						if (off > 0) {
							for (size_t moff = 0; moff < off; moff++) {
								out += ' ';
							}
						}
						for (size_t show_len = 0; show_len < arg.size(); show_len++)
							out += '^';
						SHIFT_WARNING("Ignored parameter: " << SHIFT_FLAG_LIB_PATH << " (parameter " << (i+1) << ")");
						SHIFT_WARNING_LOG(this->to_string());
						SHIFT_WARNING_LOG(out);
						SHIFT_FATAL();
					} else {
						this->m_library_paths.push_back(io::directory(std::string(next_arg)));
						i++;
					}
				} else if (arg == SHIFT_FLAG_LIB) {
					if ((i + 1) >= this->m_args.size()) {
						SHIFT_ERROR("Expected library after flag " << SHIFT_FLAG_LIB << " (parameter " << (i+1) << ")");

						std::string out;
						out.reserve(arg.size() + off);
						if (off > 0) {

							for (size_t moff = 0; moff < off; moff++) {
								out += ' ';
							}
						}
						for (size_t show_len = 0; show_len < arg.size(); show_len++)
							out += '^';
						SHIFT_ERROR_LOG(this->to_string());
						SHIFT_FATAL_ERROR_LOG(out);
					}
					const std::string_view next_arg = this->m_args[i + 1];

					if (string_starts_with(next_arg, SHIFT_FLAG_PREFIX)) {
						std::string out;
						out.reserve(off + arg.size());

						if (off > 0) {
							for (size_t moff = 0; moff < off; moff++) {
								out += ' ';
							}
						}
						for (size_t show_len = 0; show_len < arg.size(); show_len++)
							out += '^';
						SHIFT_WARNING("Ignored parameter: " << SHIFT_FLAG_LIB << " (parameter " << (i+1) << ")");
						SHIFT_WARNING_LOG(this->to_string());
						SHIFT_WARNING_LOG(out);
						SHIFT_FATAL();
					} else {
						this->m_libraries.push_back(io::file(std::string(next_arg)));
						i++;
					}
				} else if (string_starts_with(arg, SHIFT_FLAG_PREFIX)) {
					// error, unknown setting
					std::string out;
					out.reserve(off + arg.size());
					if (off > 0) {
						for (size_t moff = 0; moff < off; moff++) {
							out += ' ';
						}
					}
					for (size_t show_len = 0; show_len < arg.size(); show_len++)
						out += '^';
					SHIFT_WARNING("Ignored parameter: " << arg << " (parameter " << (i+1) << ")");
					SHIFT_WARNING_LOG(this->to_string());
					SHIFT_WARNING_LOG(out);
					SHIFT_FATAL();
				} else {
					// if it doesn't start with a SHIFT_FLAG_PREFIX, we assume that they are entering a source file
					this->m_compile_files.push_back(io::file(std::string(arg)));
				}

				off += arg.length() + 1; // + 1 to account for space character when printing out
			}

			if (this->m_compile_files.size() <= 0) {
				// If there are no sources files to compile, help page will be displayed since there is no work to do.
				this->m_flags = FLAG_HELP;
			}

			this->resolve_libraries_and_sources();
		}

		void argument_parser::resolve_libraries_and_sources(void) {
//			std::cout << "Libraries [Before]: " << this->get_libraries() << std::endl;
//			std::cout << "Library paths [Before]: " << this->get_library_paths() << std::endl;
//			std::cout << "Sources [Before]: " << this->get_source_files() << std::endl;

			{ // Remove inexistent library paths
				std::list<const io::directory*> remove_lib_paths;
				for (const io::directory& lib_path : this->get_library_paths()) {
					if (!lib_path) {
						// warning: library file not found: "{lib}"
						// Inform the user whenever a library path doesn't exist
						SHIFT_WARNING("Ignored inexistent library path: " << lib_path.get_path());
						remove_lib_paths.push_back(&lib_path);
					}
				}

				for (const io::directory* lib_path : remove_lib_paths) {
					this->get_library_paths().remove(*lib_path);
				}
			}

			{ // Remove inexistent libraries
				std::list<io::file*> remove_lib;
				for (io::file& lib_file : this->get_libraries()) {
					if (!lib_file) {
						// If it is not in current directory, checks directories from inputed library paths
						for (const io::directory& lib_path : this->get_library_paths()) {
							shift::io::file new_file = lib_path / lib_file;

							new_file.get_path().make_preferred();

							if (new_file) {
								// If the file exists within this directory, use this one as the library file
								lib_file = new_file; // you could also keep a list of library candidates, and then either error or warn the user which one you chose
								break;
							}
						}
					}

					if (!lib_file) {
						// warning: library file not found: "{lib}"
						// If the library was unable to be found, inform the user
						SHIFT_WARNING("Ignored inexistent library: " << lib_file.get_path());
						remove_lib.push_back(&lib_file); // and schedule it for removal
					}
				}

				for (const io::file* lib : remove_lib) {
					this->get_libraries().remove(*lib);
				}
			}

			{ // Remove inexistent source files
				std::list<const io::file*> remove_source;
				for (const io::file& source_file : this->get_source_files()) {
					std::string::size_type star_index;
					const std::string str = source_file.get_path().string();

					if ((star_index = str.find('*')) != std::string::npos) {
						// TODO allow user to enter "mycode/*.shift" to compile all files in a directory.
					}

					if (!source_file) {
						SHIFT_WARNING("Ignored inexistent source file: " << str);
						//this->get_source_files().remove(str);
						remove_source.push_back(&source_file);
					}
				}

				for (const io::file* file : remove_source) {
					this->get_source_files().remove(*file);
				}
			}

			SHIFT_PRINT();

//			std::cout << "Libraries [After]: " << this->get_libraries() << std::endl;
//			std::cout << "Library paths [After]: " << this->get_library_paths() << std::endl;
//			std::cout << "Sources [After]: " << this->get_source_files() << std::endl;
		}
	}
}
