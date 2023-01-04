/**
 * @file shift_compiler.cpp
 */
#include "shift_compiler.h"

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {
		SHIFT_COMPILER_API shift_compiler::shift_compiler(const int argc, const char*const* const argv, shift_error_handler* const error_handler) : m_error_handler(
				error_handler ? error_handler : (new shift_error_handler())), m_args(argc, argv, m_error_handler), m_del_error_handler(
				!error_handler) {

		}

//		shift_compiler::shift_compiler(const argument_parser& parser) : m_args(parser), m_error_handler(parser.m_error_handler), m_del_error_handler(
//				false) {
//		}

//		shift_compiler::shift_compiler(argument_parser&& parser) : m_error_handler(parser.m_error_handler), m_args(std::move(parser)), m_del_error_handler(
//				m_args.m_del_error_handler) {
//		}

		SHIFT_COMPILER_API shift_compiler::~shift_compiler(void) noexcept {
			if (m_del_error_handler)
				delete m_error_handler;
		}

		SHIFT_COMPILER_API void shift_compiler::parse_arguments(void) {
			this->m_args.parse();
			this->m_error_handler->print_and_exit();
		}

		SHIFT_COMPILER_API void shift_compiler::tokenize(void) {
			{
				const size_t source_file_count = this->m_args.get_source_files().size();
				this->m_tokenizers.reserve(source_file_count); // reserve correct amount of source files

				for (const io::file& source_file : this->m_args.get_source_files()) {
					// tokenize each file
					tokenizer tokenizer(source_file, this->m_error_handler);
					tokenizer.tokenize();
					this->m_tokenizers.push_back(std::move(tokenizer));
				}
			}

			{ // send errors for all tokenizers
//				bool err = false;
//				for (tokenizer& tokenizer : this->m_tokenizers) {
//					err |= tokenizer.get_error_handler().error_count();
//					tokenizer.get_error_handler().print();
//				}

				this->m_error_handler->print_and_exit();
			}
		}

		SHIFT_COMPILER_API void shift_compiler::parse(void) {

		}
	}
}
