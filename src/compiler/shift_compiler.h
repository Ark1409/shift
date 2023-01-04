/**
 * @file include/shift_compiler.h
 */

#ifndef SHIFT_COMPILER_H_
#define SHIFT_COMPILER_H_ 1

#include "../shift_config.h"

#include "tokenizer.h"
#include "argument_parser.h"
#include <vector>

/** Namespace shift */
namespace shift {
	/** Namespace compiler */
	namespace compiler {

		class SHIFT_COMPILER_API shift_compiler {
			private:
				shift_error_handler* m_error_handler;
				argument_parser m_args;
				bool m_del_error_handler = false;
				std::vector<tokenizer> m_tokenizers;
			public:
				shift_compiler(const int argc, const char* const* const argv, shift_error_handler* const error_handler = nullptr);
				~shift_compiler(void) noexcept;
				shift_compiler(const shift_compiler& other) = delete;
				shift_compiler(shift_compiler&& other) = default;
				shift_compiler& operator=(const shift_compiler& other) = delete;
				shift_compiler& operator=(shift_compiler&& other) = default;

				void parse_arguments(void);
				void tokenize(void);
				void parse(void);

				inline void execute(void) {
					parse_arguments();
					tokenize();
					parse();
				}
		};

	}
}

#endif /* SHIFT_COMPILER_H_ */
