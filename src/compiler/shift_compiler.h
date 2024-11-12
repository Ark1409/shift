/**
 * @file compiler/shift_compiler.h
 */
#ifndef SHIFT_COMPILER_H_
#define SHIFT_COMPILER_H_ 1

#include "compiler/shift_error_handler.h"
#include "compiler/shift_argument_parser.h"
#include "compiler/tokenizer/shift_tokenizer.h"
#include "compiler/parser/parser.h"
#include "compiler/analyzing/analyzer.h"

namespace shift::compiler {
    class compiler {
    public:
        compiler() noexcept = default;

        inline compiler(const int argc, const char* const* const argv) noexcept;

        inline compiler(const std::vector<std::string_view>& args) noexcept;

        inline compiler(std::vector<std::string_view>&& args) noexcept;

        inline void parse_flags() { m_args.parse(); }

        SHIFT_API void tokenize();

        SHIFT_API void parse();

        SHIFT_API void analyze();

        inline void run() {
            parse_flags();
            tokenize();
            parse();
            analyze();
        }

        inline error_handler& get_error_handler() noexcept { return m_error_handler; }

        inline error_handler const& get_error_handler() const noexcept { return m_error_handler; }

    private:
        error_handler m_error_handler;
        argument_parser m_args{ &m_error_handler };
        std::deque<tokenizer> m_tokenizers;
        std::deque<parser> m_parsers;
        analyzer m_analyzer{ &m_error_handler, m_parsers };
    };

    inline compiler::compiler(const int argc, const char* const* const argv) noexcept : m_args(&m_error_handler, argc, argv) {}

    inline compiler::compiler(const std::vector<std::string_view>& args) noexcept : m_args(&m_error_handler, args) {}

    inline compiler::compiler(std::vector<std::string_view>&& args) noexcept : m_args(&m_error_handler, std::move(args)) {}

}
#endif