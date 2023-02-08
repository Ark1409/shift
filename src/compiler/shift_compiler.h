/**
 * @file compiler/shift_compiler.h
 */
#include "compiler/shift_error_handler.h"
#include "compiler/shift_argument_parser.h"
#include "compiler/shift_tokenizer.h"
#include "compiler/shift_parser.h"

namespace shift {
    namespace compiler {
        class compiler {
        public:
            inline compiler() noexcept;
            inline compiler(const int argc, const char* const* const argv) noexcept;
            inline compiler(const std::vector<std::string_view>& args) noexcept;
            inline compiler(std::vector<std::string_view>&& args) noexcept;

            inline void parse_flags() { m_args.parse(); }
            void tokenize();
            void parse();

            inline error_handler& get_error_handler() noexcept { return m_error_handler; }
            inline error_handler const& get_error_handler() const noexcept { return m_error_handler; }
        private:
            error_handler m_error_handler;
            argument_parser m_args;
            std::list<tokenizer> m_tokenizers;
            std::list<parser> m_parsers;
        };

        inline compiler::compiler() noexcept: m_args(&m_error_handler) {}
        inline compiler::compiler(const int argc, const char* const* const argv) noexcept: m_args(&m_error_handler, argc, argv) {}
        inline compiler::compiler(const std::vector<std::string_view>& args) noexcept: m_args(&m_error_handler, args) {}
        inline compiler::compiler(std::vector<std::string_view>&& args) noexcept: m_args(&m_error_handler, std::move(args)) {}
    }
}