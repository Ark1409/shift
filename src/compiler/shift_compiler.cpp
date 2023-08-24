/**
 * @file compiler/shift_compiler.cpp
 */
#include "compiler/shift_compiler.h"

namespace shift {
    namespace compiler {
        void compiler::tokenize() {
            for (filesystem::file const& file : m_args.get_source_files()) {
                const auto error_count_begin = m_error_handler.get_error_count();

                tokenizer _tokenizer(&m_error_handler, file);
                _tokenizer.tokenize();

                const auto error_count_end = m_error_handler.get_error_count();

                if (error_count_begin == error_count_end)
                    m_tokenizers.push_back(std::move(_tokenizer));
            }
        }

        void compiler::parse() {
            for (tokenizer& _tokenizer : m_tokenizers) {
                const auto error_count_begin = m_error_handler.get_error_count();

                m_parsers.emplace_back(&m_error_handler, &_tokenizer);
                parser& _parser = m_parsers.back();
                _parser.parse();

                const auto error_count_end = m_error_handler.get_error_count();
                if (error_count_begin != error_count_end)
                    m_parsers.erase(--m_parsers.end());
            }
        }

        void compiler::analyze() {
            this->m_analyzer.analyze();
        }
    }
}
