#ifndef SHIFT_PARSER_H_
#define SHIFT_PARSER_H_ 1

#include "compiler/lexing/lexer.h"
#include "compiler/error_handler.h"

#include "compiler/mods.h"
#include "compiler/parsing/parser_fwd.h"
#include "compiler/parsing/parser_types.h"

#include "utils/ordered_set.h"
#include "utils/ordered_map.h"
#include "utils/range.h"

#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <array>
#include <optional>
#include <memory>
#include <ranges>
#include <numeric>

namespace shift::compiler::analyzing {
    // fwd decl analyzer class for friend
    class analyzer;
}

namespace shift::compiler::parsing {
    class parser {
    public:
        inline parser(error_handler* const eh, const lexing::lexer& lex) noexcept;

        parser(error_handler* const eh, const lexing::lexer&& lex) noexcept = delete;

        parser(const parser&) = delete;

        parser(parser&&) noexcept = default;

        parser& operator=(const parser&) = delete;

        parser& operator=(parser&&) noexcept = default;

    private:
        SHIFT_API void parse();

        struct parse_state;

    public:
        inline const lexing::lexer* get_lexer() const noexcept { return m_lexer; }

        inline error_handler* get_error_handler() noexcept { return m_error_handler; }

        inline const error_handler* get_error_handler() const noexcept { return m_error_handler; }

        inline parser_module& get_module() noexcept { return *this->m_module; }

        inline const parser_module& get_module() const noexcept { return *this->m_module; }

        inline auto& get_classes() noexcept { return m_classes; }

        inline const auto& get_classes() const noexcept { return m_classes; }

        inline auto& get_functions() noexcept { return m_functions; }

        inline const auto& get_functions() const noexcept { return m_functions; }

        inline auto& get_variables() noexcept { return m_variables; }

        inline const auto& get_variables() const noexcept { return m_variables; }

        inline auto& get_global_uses() noexcept { return m_global_uses; }

        inline const auto& get_global_uses() const noexcept { return m_global_uses; }

        SHIFT_API static uint_fast8_t operator_priority(const lexing::token::type type, const bool prefix = false) noexcept;

#ifdef SHIFT_DEBUG

        SHIFT_API void print_tree();

#endif

    private:
        void parse_access_specifier();

        void parse_use(parse_state&);

        void parse_use(parse_state&, utils::ordered_set<parser_module>&);

        void parse_module();

        void parse_class(parser_class* parent_class = nullptr);

        parser_function* parse_function_header(parser_class* parent_class, parser_type& return_type);

        std::optional<parser_variable>
        parse_variable_header(parser_class* parent_class, parser_function* parent_function,
            parser_type& type);

        // void parse_class(parser_class&);
        void parse_function(parser_function&);

        void
        parse_function_block(parser_function&, utils::ideque<parser_statement>&, size_t count = -1);

        void parse_body(parse_state&, parser_class* = nullptr);

        parser_expression
        parse_expression(const utils::predicate<std::vector<token>::const_iterator>& end_func);

        inline parser_expression
        parse_expression(const lexing::token::type end_type = lexing::token::type::SEMICOLON) {
            return parse_expression([end_type](const std::vector<token>::const_iterator it) {
                return it->get_token_type() == end_type;
            });
        }

        parser_name parse_name(std::string_view);

        std::optional<parser_type> parse_type(std::string_view);

        void token_error(const lexing::token& lexing::token_, const std::string_view msg);

        void token_error(const lexing::token& lexing::token_, const std::string& msg);

        void token_error(const lexing::token& lexing::token_, const char* const msg);

        void token_warning(const lexing::token& lexing::token_, const std::string_view msg);

        void token_warning(const lexing::token& lexing::token_, const std::string& msg);

        void token_warning(const lexing::token& lexing::token_, const char* const msg);

        std::string_view get_line(const lexing::token&) const noexcept;

        const lexing::token& skip_until(const std::string_view) noexcept;

        const lexing::token& skip_until(const std::string&) noexcept;

        const lexing::token& skip_until(const char* const) noexcept;

        const lexing::token& skip_until(const typename lexing::token::type) noexcept;

        const lexing::token& skip_after(const std::string_view) noexcept;

        const lexing::token& skip_after(const std::string&) noexcept;

        const lexing::token& skip_after(const char* const) noexcept;

        const lexing::token& skip_after(const typename lexing::token::type) noexcept;

        const lexing::token& skip_before(const std::string_view) noexcept;

        const lexing::token& skip_before(const std::string&) noexcept;

        const lexing::token& skip_before(const char* const) noexcept;

        const lexing::token& skip_before(const typename lexing::token::type) noexcept;

        const lexing::token& skip_until_closing(const typename lexing::token::type) noexcept;

        bool is_module_defined() const noexcept;

    private:
        // Tokenized file. Tokenization must have passed with no errors in order to be usable in the parsing stage
        const lexing::lexer* m_lexer;

        // Error handler (if desired)
        error_handler* m_error_handler{ nullptr };

        // The current module for the file
        std::unique_ptr<parser_module> m_module;

        // Storage for all global use statements in the module. 
        // It simply contains all the 'use' statements in sequential order
        utils::ordered_set<parser_module> m_global_uses;

        // List of classes found inside current module inside current file
        std::deque<parser_class> m_classes;

        // List of functions which are tied solely to the current module inside the current file (not inside a class)
        std::deque<parser_function> m_functions;

        // List of variables which are tied solely to the current module inside the current file (not inside a class)
        std::deque<parser_variable> m_variables;


        friend class analyzing::analyzer;
    };

    inline parser::parser(error_handler* const eh, const lexing::lexer& lex) noexcept : m_lexer(&lex), m_error_handler(eh) {
        parse();
    }
}

#endif